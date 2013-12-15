#include "clang/Frontend/FrontendPluginRegistry.h"
#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/Frontend/CompilerInstance.h"
#include "llvm/Support/raw_ostream.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include <iostream>
using namespace clang;

namespace {

class SPAVisitor : public RecursiveASTVisitor<SPAVisitor> {
    private:
        bool side_effect_race(Stmt *S){
            std::cout << "TESTING" << std::endl;
            if(S->child_begin()->getStmtClass()!=59){
		        return false;
	        }
	        NamedDecl *target_addr = static_cast<clang::DeclRefExpr*>(*(S->child_begin()))->getFoundDecl();
            if((++(S->child_begin()))->getStmtClass()!=108){
		        return false;
	        }
	        UnaryOperator *inc = static_cast<clang::UnaryOperator*>(*(++(S->child_begin())));
	        if(inc->child_begin()->getStmtClass()!=59){
		        return false;
	        }
	        NamedDecl *origin_addr = static_cast<clang::DeclRefExpr*>(*(inc->child_begin()))->getFoundDecl();
  	        return target_addr==origin_addr;
        }

    public:
        bool VisitStmt(Stmt *S){
            std::cout << S->getStmtClass() << " -- " << S->getStmtClassName() << std::endl;
            /*if(S->getStmtClass()==60){//DeclRefExpr
	            NamedDecl * ND = (static_cast<clang::DeclRefExpr*>(S))->getFoundDecl();
                std::cout << " <" << ND->getQualifiedNameAsString() << ">[" << ND << "]";
            }*/
            if(S->getStmtClass()==20){//BinaryOperator
                BinaryOperator *BO = static_cast<BinaryOperator *>(S);
                if(BO->isAssignmentOp()){
                    /*std::cout << " {Assignment}";
                    std::cout << std::endl;*/
                    if(std::distance(BO->child_begin(),BO->child_end())==2 && side_effect_race(S)){
                        std::cout << "Warning: side effect race [-plugin SPA]" << std::endl;
                    }
                    //std::cout << " // " << std::distance(BO->child_begin(),BO->child_end()) << " " << BO->child_begin()->getStmtClass();
                }
            }/*else{
                std::cout << std::endl;
            }*/
            return true;
        }

        bool VisitNamedDecl(NamedDecl *D) {
            //For debugging, dumping the AST nodes will show which nodes are already being visited.
            std::cout << D->getKind() << " -- " << D->getDeclKindName() << " << " << D->getNameAsString() <<  " >>" << std::endl;
            return true;
        }
};

class SPAConsumer : public clang::ASTConsumer {
    private:
        //A RecursiveASTVisitor implementation.
        SPAVisitor Visitor;

    public:
        virtual void HandleTranslationUnit(clang::ASTContext &Context){
            //Visit all nodes in the AST
            Visitor.TraverseDecl(Context.getTranslationUnitDecl());
        }
};

class SPAAction : public PluginASTAction{
    protected:
        ASTConsumer *CreateASTConsumer(CompilerInstance &CI, llvm::StringRef){
            return new SPAConsumer();
        }

        bool ParseArgs(const CompilerInstance &CI, const std::vector<std::string>& args){
            return args.size()==0;
        }

        void PrintHelp(llvm::raw_ostream& ros) {
            ros << "Help for SPA plugin goes here\n";
        }

};

} //namespace

static FrontendPluginRegistry::Add<SPAAction>
X("SPA", "Sequence point analyzer");
