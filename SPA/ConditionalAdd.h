/* When a function is called, a special item is added to the lvalueTable. This
 * item specifies that after running the whole scan, values are added as side
 * effects that are actually a side effect in that function (in opposite to
 * previous presumption "a function modifies everything it can access"). This
 * is done iteratively until the lvalueTable doesn't change anymore. This
 * effectively implements side effect inheritance between functions. */

#include "clang/AST/AST.h"
#include "clang/AST/Stmt.h"
#include <vector>

class FunctionConditionalAdd{
  private:
    clang::NamedDecl funDecl;
    std::vector<clang::NamedDecl> functionsToAdd;
  public:
    FunctionConditionalAdd(clang::NamedDecl funDecl):funDecl(funDecl){};
    clang::NamedDecl getDecl();
    std::vector<clang::NamedDecl> getFunctionsToAdd;
    void addFunctionToAdd(clang::NamedDecl function);
};

class ContitionalAdd{
  private:
    std::vector<FunctionConditionalAdd> funcAdds;
  public:
    void addConditionalConstraint(clang::NamedDecl funDecl, clang::NamedDecl constraintFunDecl); // (function to add constraints to, function side effects of which will be added)
    void execute(); // iteratively (over i and over functions) add constraints as long as they change
};
