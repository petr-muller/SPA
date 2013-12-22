#include "clang/Frontend/FrontendPluginRegistry.h"
#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/Frontend/CompilerInstance.h"
#include "llvm/Support/raw_ostream.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include <iostream>
#include <string>
#include <fstream>

#include "FunctionWriteTable.cpp"

#define FILENAME "SPA_constraints.txt"

using namespace clang;

namespace {

//visitor
class SPAVisitor : public RecursiveASTVisitor<SPAVisitor> {
    private:
        DiagnosticsEngine &DE;
        unsigned SPA_error;
        unsigned FILE_error;
        NamedDecl *CurrentFunDecl;
        FunctionWriteTable functionWriteTable;

        bool side_effect_race(Stmt *S){
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
        }

    public:
        SPAVisitor(CompilerInstance &CI) : DE(CI.getDiagnostics()){
            SPA_error = DE.getCustomDiagID(DiagnosticsEngine::Warning, "Warning: side effect and sequence point related undefined behavior [SPA]");
        }
        bool VisitStmt(Stmt *S){
            std::cout << S->getStmtClass() << " -- " << S->getStmtClassName() << std::endl;
            if(S->getStmtClassName()==std::string("BinaryOperator")){
                BinaryOperator *BO = static_cast<BinaryOperator *>(S);
                if(BO->isAssignmentOp()){
                    if(side_effect_race(S)){
                        DE.Report(SPA_error);
                    }
                }
            }
            return true;
        }

        bool VisitNamedDecl(NamedDecl *D) {
            //For debugging, dumping the AST nodes will show which nodes are already being visited.
            std::cout << "========" << D->getKind() << " -- " << D->getDeclKindName() << " << " << D->getNameAsString() <<  " >>" << std::endl;
            if(D->isFunctionOrFunctionTemplate()){
                this->CurrentFunDecl = D;
                functionWriteTable.addFunction(D);
            }
            return true;
        }
};

//the translation unit entry
class SPAConsumer : public clang::ASTConsumer {
    private:
        //A RecursiveASTVisitor implementation.
        SPAVisitor Visitor;

    public:
        SPAConsumer(CompilerInstance &CI) : Visitor(SPAVisitor(CI)){}

        virtual void HandleTranslationUnit(clang::ASTContext &Context){
            //Visit all nodes in the AST
            Visitor.TraverseDecl(Context.getTranslationUnitDecl());
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
