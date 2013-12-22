#include "clang/AST/AST.h"
#include <iostream>

class FunctionWriteTable{
    public:
        FunctionWriteTable(){};
        ~FunctionWriteTable(){};
        bool addFunction(clang::NamedDecl *D){
            std::cout << "ADDING FUNCTION " << D->getNameAsString() << std::endl;
            return true;    
        }
};
