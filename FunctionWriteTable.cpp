#include "clang/AST/AST.h"
#include <iostream>
#include <vector>
#include <map>

class FunctionWriteTable{
    private:
        std::map<clang::NamedDecl*, std::set<clang::DeclRefExpr*> > functions;

    public:
        FunctionWriteTable(){};
        ~FunctionWriteTable(){};
        bool add(clang::NamedDecl *Func, clang::DeclRefExpr *Var);
        void dump();
};

bool FunctionWriteTable::add(clang::NamedDecl *Func, clang::DeclRefExpr *Var){
    this->functions[Func].insert(Var);
    return true;    
}

void FunctionWriteTable::dump(){
    DEBUG("--------------- FunctionWriteTable dump ---------------");
    for(std::map<clang::NamedDecl*, std::set<clang::DeclRefExpr*> >::iterator i=this->functions.begin(); i!=this->functions.end(); ++i){
        DEBUG(i->first->getNameAsString() << ":");
        for(std::set<clang::DeclRefExpr*>::iterator j=i->second.begin(); j!=i->second.end(); ++j){
            DEBUG("> " << (*j)->getDecl()->getNameAsString());
        }
    }
    DEBUG("-------------------------------------------------------");
}
