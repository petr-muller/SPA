/*SPA_poc.cpp
  */
#include "clang/Frontend/FrontendPluginRegistry.h"
#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/Frontend/CompilerInstance.h"
#include "llvm/Support/raw_ostream.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include <iostream>
using namespace clang;

namespace {

class SPA_pocVisitor
  : public RecursiveASTVisitor<SPA_pocVisitor> {
private:
  bool side_effect_race(Stmt *S){
	if(S->child_begin()->getStmtClass()!=60){
		return false;
	}
	NamedDecl *target_addr = static_cast<clang::DeclRefExpr*>(*(S->child_begin()))->getFoundDecl();
	if((++(S->child_begin()))->getStmtClass()!=109){
		return false;
	}
	UnaryOperator *inc = static_cast<clang::UnaryOperator*>(*(++(S->child_begin())));
	if(inc->child_begin()->getStmtClass()!=60){
		return false;
	}
	NamedDecl *origin_addr = static_cast<clang::DeclRefExpr*>(*(inc->child_begin()))->getFoundDecl();
  	return target_addr==origin_addr;
  }

public:
  bool VisitStmt(Stmt *S){
    /*std::cout << S->getStmtClass() << " -- " << S->getStmtClassName();
    if(S->getStmtClass()==60){//DeclRefExpr
	NamedDecl * ND = (static_cast<clang::DeclRefExpr*>(S))->getFoundDecl();
        std::cout << " <" << ND->getQualifiedNameAsString() << ">[" << ND << "]";
    }*/
    if(S->getStmtClass()==20){//BinaryOperator
        BinaryOperator *BO = static_cast<BinaryOperator *>(S);
	if(BO->isAssignmentOp()){
	    /*std::cout << " {Assignment}";
    	    std::cout << std::endl;*/
	    if(std::distance(BO->child_begin(),BO->child_end())==2 && side_effect_race(S)){
	    	std::cout << "Warning: side effect race [-plugin SPA-poc]" << std::endl;
	    }
            //std::cout << " // " << std::distance(BO->child_begin(),BO->child_end()) << " " << BO->child_begin()->getStmtClass();
	}
    }
    /*else{
        std::cout << std::endl;
    }*/
    return true;
  }

  bool VisitNamedDecl(NamedDecl *D) {
    // For debugging, dumping the AST nodes will show which nodes are already
    // being visited.
    //std::cout << D->getKind() << " -- " << D->getDeclKindName() << " -- " << D->getNameAsString() << std::endl;

    // The return value indicates whether we want the visitation to proceed.
    // Return false to stop the traversal of the AST.
    return true;
  }
};

class SPA_pocConsumer : public clang::ASTConsumer {
public:
  virtual void HandleTranslationUnit(clang::ASTContext &Context) {
    // Traversing the translation unit decl via a RecursiveASTVisitor
    // will visit all nodes in the AST.clang -Xclang -load -Xclang lib/SPA-poc.so -Xclang -add-plugin -Xclang SPA-poc ~/Desktop/bc/test.c -o test; echo "-----"; ./test; result=$?; echo "-----"; echo $result;
    Visitor.TraverseDecl(Context.getTranslationUnitDecl());
  }
private:
  // A RecursiveASTVisitor implementation.
  SPA_pocVisitor Visitor;
};

class SPA_pocAction : public PluginASTAction {
protected:
  ASTConsumer *CreateASTConsumer(CompilerInstance &CI, llvm::StringRef) {
    return new SPA_pocConsumer();
  }

  bool ParseArgs(const CompilerInstance &CI,
                 const std::vector<std::string>& args) {
    for (unsigned i = 0, e = args.size(); i != e; ++i) {
      llvm::errs() << "SPA_poc arg = " << args[i] << "\n";

      // Example error handling.
      if (args[i] == "-an-error") {
        DiagnosticsEngine &D = CI.getDiagnostics();
        unsigned DiagID = D.getCustomDiagID(
          DiagnosticsEngine::Error, "invalid argument '" + args[i] + "'");
        D.Report(DiagID);
        return false;
      }
    }
    if (args.size() && args[0] == "help")
      PrintHelp(llvm::errs());

    return true;
  }
  void PrintHelp(llvm::raw_ostream& ros) {
    ros << "Help for SPA_poc plugin goes here\n";
  }

};

}

static FrontendPluginRegistry::Add<SPA_pocAction>
X("SPA-poc", "Sequence points analyzer - proof of concept");
