#define DBG
#ifdef DBG
    #define DEBUG(str) std::cout << str << std::endl;
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
#include <iostream>
#include <string>
#include <fstream>

#include "LvalueTable.cpp"

#define FILENAME "SPA_constraints.txt"

using namespace clang;

namespace {

//visitor
class SPAVisitor : public RecursiveASTVisitor<SPAVisitor> {
    private:
        enum lvalueResult {None, Use, SideEffect};
        int lvaluelvl;
        DiagnosticsEngine &DE;
        unsigned SPA_seqwarning;
        unsigned SPA_fallback;
        unsigned FILE_error;
        NamedDecl *CurrentFunDecl;
        LvalueTable &lvalueTable;
        ParentMap *parentMap;
        bool updateParentMap;
        enum lvalueResult resolveLvalue(Stmt *tmp, Stmt *parent, DeclRefExpr *S, enum lvalueResult sideEffect){
            if(sideEffect==SideEffect){
                lvalueTable.set(parent,static_cast<DeclRefExpr*>(S),true,this->lvaluelvl);
                return SideEffect;
            }
            switch(parent->getStmtClass()){
            case Stmt::DeclRefExprClass:
                lvalueTable.set(parent,static_cast<DeclRefExpr*>(S),false,this->lvaluelvl);
                return Use;
            break;
            case Stmt::ImplicitCastExprClass:
                lvalueTable.set(parent,static_cast<DeclRefExpr*>(S),false,this->lvaluelvl);
                return Use;
            break;
            case Stmt::CallExprClass: // function implicitly has side effect on everything it can // TODO count de/referrences , this must be at least +1 &
                if(tmp != *(parent->child_begin()/* && tmp->getType()->isPointerType()*/)){
                    lvalueTable.set(parent,static_cast<DeclRefExpr*>(S),true,this->lvaluelvl);
                    return SideEffect;
                }
            break;
            case Stmt::UnaryOperatorClass: //TODO: increase lvaluelvl for &, decrease for *
                if(static_cast<UnaryOperator*>(parent)->getOpcode() == UO_Deref){
                    this->lvaluelvl--;
                }
                if(static_cast<UnaryOperator*>(parent)->getOpcode() == UO_AddrOf){
                    this->lvaluelvl++;
                }
                if(static_cast<UnaryOperator*>(parent)->isIncrementDecrementOp()){
                    lvalueTable.set(parent,static_cast<DeclRefExpr*>(S),true,this->lvaluelvl);
                    return SideEffect;
                }
                lvalueTable.set(parent,static_cast<DeclRefExpr*>(S),false,this->lvaluelvl);
                return Use;
            break;
            case Stmt::BinaryOperatorClass:
                if(static_cast<BinaryOperator*>(parent)->isAssignmentOp() && tmp == *(parent->child_begin())){
                    lvalueTable.set(parent,static_cast<DeclRefExpr*>(S),true,this->lvaluelvl);
                    return SideEffect;
                }
                lvalueTable.set(parent,static_cast<DeclRefExpr*>(S),false,this->lvaluelvl);
                return Use;
            break;
            case Stmt::CompoundAssignOperatorClass:
                if(tmp == *(parent->child_begin())){
                    lvalueTable.set(parent,static_cast<DeclRefExpr*>(S),true,this->lvaluelvl);
                    return SideEffect;
                }
                lvalueTable.set(parent,static_cast<DeclRefExpr*>(S),false,this->lvaluelvl);
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
                return Use;
            break;
            default:
            break;    
            }
            DE.Report(DE.getCustomDiagID(DiagnosticsEngine::Warning, std::string("Warning: fallback action while processing a node of unknown type ") + parent->getStmtClassName()));
            if(sideEffect == Use){//fallback action - this may lead to false positives
                lvalueTable.set(parent,static_cast<DeclRefExpr*>(S),false,this->lvaluelvl);
                return Use;
            }
            return None;
        }

        /*bool side_effect_race(Stmt *S){
            std::cout << "TESTING" << std::endl;
            if(S->child_begin()->getStmtClassName()!=std::string("DeclRefExpr")){
		        return false;
	        }
	        NamedDecl *target_addr = static_cast<clang::DeclRefExpr*>(*(S->child_begin()))->getFoundDecl();
            if((++(S->child_begin()))->getStmtClassName()!=std::string("UnaryOperator")){
		        return false;
	        }
	        UnaryOperator *inc = static_cast<clang::UnaryOperator*>(*(++(S->child_begin())));
	        if(inc->child_begin()->getStmtClassName()!=std::string("DeclRefExpr")){
		        return false;
	        }
	        NamedDecl *origin_addr = static_cast<clang::DeclRefExpr*>(*(inc->child_begin()))->getFoundDecl();
  	        return target_addr==origin_addr;
        }*/

    public:
        SPAVisitor(CompilerInstance &CI, LvalueTable &lvalueTable) : lvaluelvl(0), DE(CI.getDiagnostics()), lvalueTable(lvalueTable), parentMap(0), updateParentMap(false)/*, parentMap(static_cast<Decl*>(CI.getASTContext().getTranslationUnitDecl()))*/{
            SPA_seqwarning = DE.getCustomDiagID(DiagnosticsEngine::Warning, "Warning: side effect and sequence point related undefined behavior [SPA]");
        }
        ~SPAVisitor(){
            if(this->parentMap != 0){
                delete this->parentMap;
            }
        }

        bool VisitStmt(Stmt *S){
            /*std::cout << S->getStmtClass() << " -- " << S->getStmtClassName() << std::endl;*/
            /*if(S->getStmtClassName()==std::string("BinaryOperator")){
                BinaryOperator *BO = static_cast<BinaryOperator *>(S);
                if(BO->isAssignmentOp()){
                    if(side_effect_race(S)){
                        DE.Report(SPA_seqwarning);
                    }
                }
            }*/
            //Function write table
            /*BinaryOperator *BO;
            UnaryOperator *UO;
            CallExpr *CE;
            switch(S->getStmtClass()){
            case Stmt::BinaryOperatorClass:
                DEBUG("BinaryOperator");
                BO = static_cast<BinaryOperator*>(S);
                if(BO->isAssignmentOp() && (*(BO->child_begin()))->getStmtClass()==Expr::DeclRefExprClass){
                    lvalueTable.add(CurrentFunDecl,static_cast<DeclRefExpr*>(*(BO->child_begin())));
                }
            break;
            case Stmt::UnaryOperatorClass:
                DEBUG("UnaryOperator");
            break;
            case Stmt::CallExprClass:
                DEBUG("CallExpr");
            break;
            case Stmt::ArraySubscriptExprClass:
                DEBUG("ArraySubscriptExpr");
            break;
            default: DEBUG("Something else");
            break;    
            }*/
            //change the root node if processing new function
            
            if(this->updateParentMap){
                this->updateParentMap = false;
                if(this->parentMap){
                    delete this->parentMap;
                }
                this->parentMap = new ParentMap(S);
            }
            if(S->getStmtClass() == Stmt::DeclRefExprClass){
                DEBUG("DeclRefExpr: " << static_cast<DeclRefExpr*>(S)->getDecl()->getNameAsString());
                this->lvaluelvl = 0;
                Stmt *parent = S;
                Stmt *tmp = S;
                enum lvalueResult sideEffect = None;
                while(parent != 0){
                    DEBUG("> " << parent->getStmtClassName());
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
            //For debugging, dumping the AST nodes will show which nodes are already being visited.
            /*std::cout << "========" << D->getKind() << " -- " << D->getDeclKindName() << " << " << D->getNameAsString() <<  " >>" << std::endl;*/
            if(D->isFunctionOrFunctionTemplate()){
                this->CurrentFunDecl = D;
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
        SPAConsumer(CompilerInstance &CI) : Visitor(SPAVisitor(CI, lvalueTable)){}

        virtual void HandleTranslationUnit(clang::ASTContext &Context){
            //Visit all nodes in the AST
            Visitor.TraverseDecl(Context.getTranslationUnitDecl());
            lvalueTable.dump();
        }
};

//plugin basic behavior
class SPAAction : public PluginASTAction{
    protected:
        ASTConsumer *CreateASTConsumer(CompilerInstance &CI, llvm::StringRef){
            return new SPAConsumer(CI);
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
