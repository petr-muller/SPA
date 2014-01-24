#include "clang/AST/AST.h"
#include <map>
#include <string>

class LvalueTable{
    private:
        class Tag{
            public:
                clang::NamedDecl *D;
                bool sideEffect;
                int lvaluelvl;
                Tag(clang::NamedDecl *D, bool sideEffect, int lvaluelvl): D(D), sideEffect(sideEffect), lvaluelvl(lvaluelvl){};
        };

        std::map<clang::Stmt*, std::vector<Tag> > table;
        std::string printLvl(int lvaluelvl);

    public:
        bool set(clang::Stmt *S, clang::DeclRefExpr *D, bool sideEffect, int lvaluelvl);
        void dump();
};

std::string LvalueTable::printLvl(int lvaluelvl){
    return std::string("LVL");
}

bool LvalueTable::set(clang::Stmt *S, clang::DeclRefExpr *D, bool sideEffect, int lvaluelvl){
    DEBUG("Adding Lvalue " << this->printLvl(lvaluelvl) << D->getDecl()->getNameAsString() << " " << (sideEffect ? "WITH" : "WITHOUT") << " side effect to statement " << S << " (" << S->getStmtClassName() << ")");
    //this->table[S][D->getDecl()] = this->table[S][D->getDecl()] || sideEffect;
    Tag tag(D->getDecl(),sideEffect,lvaluelvl);
    this->table[S].push_back(tag);
    return true;
}

void LvalueTable::dump(){
    #ifdef DBG
        DEBUG("--------------- Lvalue Table dump ---------------");
        for(std::map<clang::Stmt*, std::vector<Tag> >::iterator i = this->table.begin(); i != this->table.end(); ++i){
            DEBUG(i->first << " (" << i->first->getStmtClassName() << ")");
            for(std::vector<Tag>::iterator j = i->second.begin(); j != i->second.end(); ++j){
                DEBUG("> " << this->printLvl(j->lvaluelvl) << j->D->getNameAsString() << " (" << j->D << ")" << (j->sideEffect ? " - SIDE EFFECT" : ""));
            }
        }
        DEBUG("-------------------------------------------------");
    #endif
}
