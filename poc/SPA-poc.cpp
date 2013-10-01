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
public:
  bool VisitStmt(Stmt *S){
    std::cout << S->getStmtClass() << " -- ";
    std::cout << S->getStmtClassName();
    if(S->getStmtClass()==60){
        std::cout << " <" << (static_cast<clang::DeclRefExpr*>(S))->getFoundDecl()->getNameAsString() << ">";
    }
    std::cout << std::endl;
    return true;
  }

  bool VisitNamedDecl(NamedDecl *D) {
    // For debugging, dumping the AST nodes will show which nodes are already
    // being visited.
    std::cout << D->getKind() << " -- " << D->getDeclKindName() << " -- " << D->getNameAsString() << std::endl;

    // The return value indicates whether we want the visitation to proceed.
    // Return false to stop the traversal of the AST.
    return true;
  }
};

class SPA_pocConsumer : public clang::ASTConsumer {
public:
  virtual void HandleTranslationUnit(clang::ASTContext &Context) {
    // Traversing the translation unit decl via a RecursiveASTVisitor
    // will visit all nodes in the AST.
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
