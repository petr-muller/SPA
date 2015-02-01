/* Sequence point analyzer
 * Lukas Hellebrandt <xhelle04@stud.fit.vutbr.cz>
 */

//#define DBG
#ifdef DBG
    #define DEBUG(str) std::cout << str << std::endl;
#else
    #define DEBUG(str)
#endif

#include "clang/Frontend/FrontendPluginRegistry.h"
#include "clang/AST/AST.h"
#include "clang/AST/Stmt.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/ParentMap.h"
#include "clang/Frontend/CompilerInstance.h"
#include "llvm/Support/raw_ostream.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CodeGenOptions.h"
#include <iostream>
#include <sstream>
#include <string>
#include <fstream>
#include <iterator>

#include "LvalueTable.cpp"

#define FILENAME "SPA_constraints.txt"


namespace {

using namespace clang;

std::stringstream indexAsString;

//visitor
class SPAVisitor : public RecursiveASTVisitor<SPAVisitor> {
    private:
        CompilerInstance &CI;
        enum lvalueResult {None, Use, SideEffect};
        int lvaluelvl;
        DiagnosticsEngine &DE;
        unsigned SPA_fallback;
        unsigned FILE_error;
        NamedDecl *currentFunDecl;
        LvalueTable &lvalueTable;
        ParentMap *parentMap;
        bool updateParentMap;
        enum lvalueResult resolveLvalue(Stmt *tmp, Stmt *parent, DeclRefExpr *S, enum lvalueResult sideEffect){
            int childIndex = 0;
            Stmt::child_iterator child = parent->child_begin();
            while(child!=parent->child_end() && *child != tmp){
                std::advance(child, 1);
                ++childIndex;
            }
            if(sideEffect==SideEffect){
                lvalueTable.set(this->currentFunDecl, parent,static_cast<DeclRefExpr*>(S),true,this->lvaluelvl, childIndex);
                return SideEffect;
            }
            switch(parent->getStmtClass()){
            case Stmt::DeclRefExprClass:
                lvalueTable.set(this->currentFunDecl, parent,static_cast<DeclRefExpr*>(S),false,this->lvaluelvl, childIndex);
                return Use;
            break;
            case Stmt::ImplicitCastExprClass:
                lvalueTable.set(this->currentFunDecl, parent,static_cast<DeclRefExpr*>(S),false,this->lvaluelvl,childIndex);
                return Use;
            break;
            case Stmt::CallExprClass: // function implicitly has side effect on everything it can
                if(/*this->lvaluelvl > 0 && */tmp != *(parent->child_begin())){ //FIXME: int i; int j = &i; works for f(&j) but not for f(j) (which should tag side effect for i)
                    this->lvaluelvl--;
                    lvalueTable.set(this->currentFunDecl, parent,static_cast<DeclRefExpr*>(S),true,this->lvaluelvl, childIndex);
                    return SideEffect;
                }
                this->lvaluelvl--;
                return None;
            break;
            case Stmt::ConditionalOperatorClass:
                if(tmp !=  *(parent->child_begin())){
                    return Use;
                }
                return None;
            break;
            case Stmt::UnaryOperatorClass:
                if(static_cast<UnaryOperator*>(parent)->getOpcode() == UO_Deref){
                    this->lvaluelvl--;
                }
                if(static_cast<UnaryOperator*>(parent)->getOpcode() == UO_AddrOf){
                    this->lvaluelvl++;
                }
                if(static_cast<UnaryOperator*>(parent)->isIncrementDecrementOp()){
                    lvalueTable.set(this->currentFunDecl, parent,static_cast<DeclRefExpr*>(S),true,this->lvaluelvl, childIndex);
                    return SideEffect;
                }
                lvalueTable.set(this->currentFunDecl, parent,static_cast<DeclRefExpr*>(S),false,this->lvaluelvl, childIndex);
                return Use;
            break;
            case Stmt::BinaryOperatorClass:
                if(static_cast<BinaryOperator*>(parent)->isAssignmentOp() && tmp == *(parent->child_begin())){
                    lvalueTable.set(this->currentFunDecl, parent,static_cast<DeclRefExpr*>(S),true,this->lvaluelvl, childIndex);
                    return SideEffect;
                }
                lvalueTable.set(this->currentFunDecl, parent,static_cast<DeclRefExpr*>(S),false,this->lvaluelvl, childIndex);
                return Use;
            break;
            case Stmt::CompoundAssignOperatorClass:
                if(tmp == *(parent->child_begin())){
                    lvalueTable.set(this->currentFunDecl, parent,static_cast<DeclRefExpr*>(S),true,this->lvaluelvl, childIndex);
                    return SideEffect;
                }
                lvalueTable.set(this->currentFunDecl, parent,static_cast<DeclRefExpr*>(S),false,this->lvaluelvl, childIndex);
                return Use;
            break;
            case Stmt::CompoundStmtClass:
                return None;
            case Stmt::ReturnStmtClass:
                return None;
            break;
            case Stmt::DeclStmtClass:
                return None;
            break;
            case Stmt::ParenExprClass:
                lvalueTable.set(this->currentFunDecl, parent,static_cast<DeclRefExpr*>(S),false,this->lvaluelvl, childIndex);
                return Use;
            break;
            case Stmt::ArraySubscriptExprClass:
                indexAsString << "[" << this->CI.getSourceManager().getExpansionLineNumber(S->getLocStart()) << "," << this->CI.getSourceManager().getExpansionColumnNumber(S->getLocStart()) << "]";
                std::cout << "index: " << indexAsString.str() << std::endl;
                lvalueTable.set(this->currentFunDecl, parent,static_cast<DeclRefExpr*>(S),false,this->lvaluelvl, childIndex);
                /*if(tmp != *(parent->child_begin())){ // FIXME: this might make sense for subscription as the value actually is dereferrenced, but it is also read and the * makes things harder (this is addressing the case arr[i] for i as it can be vice versa)
                    this->lvaluelvl--;
                }*/
                return Use;
            break;
            case Stmt::ForStmtClass:
            case Stmt::CaseStmtClass:
            case Stmt::InitListExprClass:
            case Stmt::UnaryExprOrTypeTraitExprClass:
            case Stmt::IfStmtClass:
            case Stmt::WhileStmtClass:
            case Stmt::SwitchStmtClass:
            case Stmt::BreakStmtClass:
            case Stmt::DoStmtClass:
            case Stmt::GotoStmtClass:
            case Stmt::CStyleCastExprClass:
            case Stmt::MemberExprClass:
                lvalueTable.set(this->currentFunDecl, parent,static_cast<DeclRefExpr*>(S),false,this->lvaluelvl, childIndex);
                return Use;
            break;
            default:
            break;    
            }
            DE.Report(this->SPA_fallback) << parent->getStmtClassName();
            if(sideEffect == Use){//fallback action - this may lead to false positives
                lvalueTable.set(this->currentFunDecl, parent,static_cast<DeclRefExpr*>(S),false,this->lvaluelvl, childIndex);
                return Use;
            }
            return None;
        }

    public:
        SPAVisitor(CompilerInstance &CI, LvalueTable &lvalueTable) : CI(CI), lvaluelvl(0), DE(CI.getDiagnostics()), lvalueTable(lvalueTable), parentMap(0), updateParentMap(false)/*, parentMap(static_cast<Decl*>(CI.getASTContext().getTranslationUnitDecl()))*/{
            this->SPA_fallback = DE.getCustomDiagID(DiagnosticsEngine::Warning, "SPA: fallback action while processing a node of unknown type '%0'");
        }
        ~SPAVisitor(){
            if(this->parentMap != 0){
                delete this->parentMap;
            }
        }

        bool VisitStmt(Stmt *S){
            //change the root node if processing new function
            if(this->updateParentMap){
                this->updateParentMap = false;
                if(this->parentMap){
                    delete this->parentMap;
                }
                this->parentMap = new ParentMap(S);
            }
            if(S->getStmtClass() == Stmt::DeclRefExprClass){
                //DEBUG("DeclRefExpr: " << static_cast<DeclRefExpr*>(S)->getDecl()->getNameAsString());
                this->lvaluelvl = 0;
                indexAsString.str("");
                indexAsString.clear();
                Stmt *parent = S;
                Stmt *tmp = S;
                enum lvalueResult sideEffect = None;
                while(parent != 0){
                    //DEBUG("> " << parent->getStmtClassName());
                    if((sideEffect=this->resolveLvalue(tmp,parent,static_cast<DeclRefExpr*>(S),sideEffect))==None){
                        break;
                    }
                    tmp = parent;
                    parent = this->parentMap->getParent(parent);
                }
            }
            return true;
        }

        bool VisitNamedDecl(NamedDecl *D) {
            if(D->isFunctionOrFunctionTemplate()){
                this->currentFunDecl = D;
                DEBUG("In function " << this->currentFunDecl->getNameAsString());
                this->updateParentMap = true;
            }
            return true;
        }
};

//the translation unit entry
class SPAConsumer : public clang::ASTConsumer {
    private:
        //A RecursiveASTVisitor implementation.
        LvalueTable lvalueTable;
        SPAVisitor Visitor;

    public:
        SPAConsumer(CompilerInstance &CI): lvalueTable(LvalueTable(CI)) , Visitor(SPAVisitor(CI, lvalueTable)){}

        virtual void HandleTranslationUnit(clang::ASTContext &Context){
            //Visit all nodes in the AST
            Visitor.TraverseDecl(Context.getTranslationUnitDecl());
            #ifdef DBG
                lvalueTable.dump();
            #endif
            std::cout << lvalueTable.makeConstraints();
        }
};

//plugin basic behavior
class SPAAction : public PluginASTAction{
    protected:
        std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI, llvm::StringRef){
            return std::unique_ptr<clang::ASTConsumer>(new SPAConsumer(CI));
        }

        bool ParseArgs(const CompilerInstance &CI, const std::vector<std::string>& args){
            return args.size()==0;
        }

        void PrintHelp(llvm::raw_ostream& ros) {
            ros << "Help for SPA plugin goes here\n";
        }

};

} //namespace

//register the plugin
static FrontendPluginRegistry::Add<SPAAction>
X("SPA", "Sequence point analyzer");
