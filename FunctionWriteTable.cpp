#include "clang/AST/AST.h"
#include <iostream>
#include <vector>

struct function{
    clang::NamedDecl *D;
    std::vector<clang::NamedDecl*> changes;
};

class FunctionWriteTable{
    private:
        std::vector<struct function> functions;

    public:
        FunctionWriteTable(){};
        ~FunctionWriteTable(){};
        bool addFunction(clang::NamedDecl *D);
        void dump();
};

bool FunctionWriteTable::addFunction(clang::NamedDecl *D){
    DEBUG("ADDING FUNCTION " << D->getNameAsString());
    struct function func = {D};
    this->functions.push_back(func);
    return true;    
}

void FunctionWriteTable::dump(){
    DEBUG("--------------- FunctionWriteTable dump ---------------");
    for(std::vector<struct function>::iterator i=this->functions.begin(); i!=this->functions.end(); ++i){
        DEBUG(i->D->getNameAsString() << ":");
        for(std::vector<clang::NamedDecl*>::iterator j=i->changes.begin(); j!=i->changes.end(); ++j){
            DEBUG("> " << (*j)->getNameAsString());
        }
    }
    DEBUG("-------------------------------------------------------");
}
