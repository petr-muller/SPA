#include "clang/AST/AST.h"
#include <map>

class LvalueTable{
    private:
        std::map<clang::Stmt*, std::map<clang::NamedDecl*, bool> > table;

    public:
        bool set(clang::Stmt *S, clang::DeclRefExpr *D, bool sideEffect);
        void dump();
};

bool LvalueTable::set(clang::Stmt *S, clang::DeclRefExpr *D, bool sideEffect){
    DEBUG("Adding Lvalue " << D->getDecl()->getNameAsString() << " " << (sideEffect ? "WITH" : "WITHOUT") << " side effect to statement " << S << " (" << S->getStmtClassName() << ")");
    this->table[S][D->getDecl()] = this->table[S][D->getDecl()] || sideEffect;
    return true;
}

void LvalueTable::dump(){
    #ifdef DBG
        DEBUG("--------------- Lvalue Table dump ---------------");
        for(std::map<clang::Stmt*, std::map<clang::NamedDecl*, bool> >::iterator i = this->table.begin(); i != this->table.end(); ++i){
            DEBUG(i->first << " (" << i->first->getStmtClassName() << ")");
            for(std::map<clang::NamedDecl*, bool>::iterator j = i->second.begin(); j != i->second.end(); ++j){
                DEBUG("> " << j->first->getNameAsString() << " (" << j->first << ")" << (j->second ? " - SIDE EFFECT" : ""));
            }
        }
        DEBUG("-------------------------------------------------");
    #endif
}
