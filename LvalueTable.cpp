#include "clang/AST/AST.h"
#include "clang/AST/Stmt.h"
#include "clang/Frontend/CompilerInstance.h"
#include <map>
#include <string>
#include <sstream>
#include <iterator>

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
    //DEBUG("Adding Lvalue " << this->printLvl(lvaluelvl) << D->getDecl()->getNameAsString() << " " << (sideEffect ? "WITH" : "WITHOUT") << " side effect to statement " << S << " (" << S->getStmtClassName() << ")");
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
    std::map< unsigned, std::map<std::string, std::map<std::string, bool> > > usedConstraints;
    std::stringstream var1, var2;
    bool skipFirstOccurenceOfSameObject = false;
    ret << "The following is a set of constraints under which there was NO undefined behavior detected by the SPA:" << std::endl;
    for(std::map<clang::Stmt*, std::vector<Tag> >::iterator i = this->table.begin(); i != this->table.end(); ++i){
        row = this->CI.getSourceManager().getSpellingLineNumber(i->first->getLocStart());
        switch(i->first->getStmtClass()){

        case clang::Stmt::CompoundStmtClass: // {}
            continue;
        break;

        case clang::Stmt::BinaryOperatorClass:
            if(static_cast<clang::BinaryOperator*>(i->first)->isAssignmentOp()){
                for(unsigned j=0; j<2; ++j){ //child node index
                    clang::Stmt::child_iterator children = i->first->child_begin();
                    std::advance(children,j);
                    std::vector<Tag> child = this->table[*(children)];
                    for(std::vector<Tag>::iterator k=child.begin(); k!=child.end(); ++k){
                        var1.str("");
                        var1 << this->printLvl(k->lvaluelvl) << k->D->getNameAsString();
                        skipFirstOccurenceOfSameObject = true;
                        if(!k->sideEffect){
                            continue;
                        }
                        for(std::vector<Tag>::iterator l=i->second.begin(); l!=i->second.end(); ++l){
                            var2.str("");
                            var2 << this->printLvl(l->lvaluelvl) << l->D->getNameAsString();
                            if(var1.str() == var2.str()){
                                if(skipFirstOccurenceOfSameObject){
                                    skipFirstOccurenceOfSameObject = false;
                                    continue;
                                }
                            }
                            if((!usedConstraints[row][var1.str()][var2.str()]) && (!usedConstraints[row][var2.str()][var1.str()])){
                                usedConstraints[row][var1.str()][var2.str()] = 1;
                                if(var1.str() == var2.str()){
                                    ret << "row: '" << row << "', undefined='" << var1.str() << "'" << std::endl;
                                }else{
                                    ret << "row: '" << row << "', var1: '" << var1.str() << "', var2: '" << var2.str() << "'" << std::endl;
                                }
                            }
                        }
                    }
                }
            }else{
                goto defaultClass;
            }
        break;

        case clang::Stmt::ConditionalOperatorClass: // ? :
            for(unsigned j=0; j<3; ++j){ //child node index
                clang::Stmt::child_iterator children = i->first->child_begin();
                std::vector<Tag> first_child = this->table[*(children)];
                std::advance(children,j);
                std::vector<Tag> other_child = this->table[*(children)];
                for(std::vector<Tag>::iterator k=first_child.begin(); k!=first_child.end(); ++k){
                    if(!k->sideEffect){
                        continue;
                    }
                    for(std::vector<Tag>::iterator l=other_child.begin(); l!=other_child.end(); ++l){
                        if(k==l){// FIXME -- this is fishy - comparing iterators from two different vectors
                            continue;
                        }
                        var1 << this->printLvl(k->lvaluelvl) << k->D->getNameAsString();
                        var2 << this->printLvl(l->lvaluelvl) << l->D->getNameAsString();
                        if((!usedConstraints[row][var1.str()][var2.str()]) && (!usedConstraints[row][var2.str()][var1.str()])){
                            usedConstraints[row][var1.str()][var2.str()] = 1;
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
            }
        break;

        default:
        defaultClass:
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
                    if((!usedConstraints[row][var1.str()][var2.str()]) && (!usedConstraints[row][var2.str()][var1.str()])){
                        usedConstraints[row][var1.str()][var2.str()] = 1;
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
        break;
        }
        sideEffects = 0;
    }
    return ret.str();
}
