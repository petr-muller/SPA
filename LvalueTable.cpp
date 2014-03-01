#include "clang/AST/AST.h"
#include "clang/Frontend/CompilerInstance.h"
#include <map>
#include <string>
#include <sstream>

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
        clang::CompilerInstance &CI;

    public:
        LvalueTable(clang::CompilerInstance &CI): CI(CI){};
        bool set(clang::Stmt *S, clang::DeclRefExpr *D, bool sideEffect, int lvaluelvl);
        void dump();
        std::string makeConstraints();
};

std::string LvalueTable::printLvl(int lvaluelvl){
    if(lvaluelvl==0){
        return std::string();
    }
    char symbol = lvaluelvl>0 ? '&' : '*';
    lvaluelvl = lvaluelvl>0 ? lvaluelvl : -1*lvaluelvl;
    return std::string(lvaluelvl,symbol);
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

std::string LvalueTable::makeConstraints(){
    std::stringstream ret;
    unsigned sideEffects = 0;
    unsigned row;
    std::map< unsigned, std::map<std::string, std::string> > usedConstraints;
    std::stringstream var1, var2;
    ret << "The following is a set of constraints under which there was NO undefined behavior detected by the SPA:" << std::endl;
    for(std::map<clang::Stmt*, std::vector<Tag> >::iterator i = this->table.begin(); i != this->table.end(); ++i){
                    row = this->CI.getSourceManager().getSpellingLineNumber(i->first->getLocStart());
        //each with each -- for most of the statements (particularly those without sequence points within them)
        for(std::vector<Tag>::iterator j = i->second.begin(); j != i->second.end(); ++j){
            if(!j->sideEffect){
                continue;
            }
            for(std::vector<Tag>::iterator k = i->second.begin(); k != i->second.end(); ++k){
                if(j==k){
                    continue;
                }
                var1 << this->printLvl(j->lvaluelvl) << j->D->getNameAsString();
                var2 << this->printLvl(k->lvaluelvl) << k->D->getNameAsString();
                if((usedConstraints[row][var1.str()] != var2.str()) && (usedConstraints[row][var2.str()] != var1.str())){
                    std::cout << "'" << usedConstraints[row][var1.str()] << "' != '" << var2.str() << "' && '" << usedConstraints[row][var2.str()] << "' != '" << var1.str() << "'" << std::endl;
                    usedConstraints[row][var1.str()] = var2.str();//blbost! prepisuju hodnotu, nepridavam do seznamu, coz bych mel...
                    //usedConstraints[row][var2.str()] = var1.str();
                    if(var1.str() == var2.str()){
                        ret << "row: '" << row << "', undefined='" << var1.str() << "'" << std::endl;
                    }else{
                        ret << "row: '" << row << "', var1: '" << var1.str() << "', var2: '" << var2.str() << "'" << std::endl;
                    }
                }
                var1.str("");
                var2.str("");
            }
        }
        sideEffects = 0;
    }
    return ret.str();
}
