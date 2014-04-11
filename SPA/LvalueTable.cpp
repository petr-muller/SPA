#include "clang/AST/AST.h"
#include "clang/AST/Stmt.h"
#include "clang/Frontend/CompilerInstance.h"
#include <map>
#include <string>
#include <sstream>
#include <iterator>

class LvalueTable{
    private:
        typedef struct coords{
          unsigned row;
          unsigned col;
        } coords;
        class Tag{
            public:
                clang::NamedDecl *D;
                bool sideEffect;
                bool isRestrict;
                int lvaluelvl;
                int childIndex;
                Tag(clang::NamedDecl *D, bool sideEffect, bool isRestrict, int lvaluelvl, int childIndex): D(D), sideEffect(sideEffect), isRestrict(isRestrict), lvaluelvl(lvaluelvl), childIndex(childIndex){};
        };

        std::map<clang::Stmt*, std::vector<Tag> > table;
        std::string printLvl(int lvaluelvl);
        clang::CompilerInstance &CI;
        bool addConstraint(std::stringstream &stream, coords start, coords end, std::stringstream &var1, std::stringstream &var2, std::map<unsigned, std::map<std::string, std::map<std::string, bool> > > usedConstraints);

    public:
        LvalueTable(clang::CompilerInstance &CI): CI(CI){};
        bool set(clang::Stmt *S, clang::DeclRefExpr *D, bool sideEffect, int lvaluelvl, int childIndex);
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

bool LvalueTable::set(clang::Stmt *S, clang::DeclRefExpr *D, bool sideEffect, int lvaluelvl, int childIndex){
    //DEBUG("Adding Lvalue " << this->printLvl(lvaluelvl) << D->getDecl()->getNameAsString() << " " << (sideEffect ? "WITH" : "WITHOUT") << " side effect to statement " << S << " (" << S->getStmtClassName() << ")");
    //this->table[S][D->getDecl()] = this->table[S][D->getDecl()] || sideEffect;
    Tag tag(D->getDecl(),sideEffect,D->getType().getQualifiers().hasRestrict(),lvaluelvl, childIndex);
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

bool LvalueTable::addConstraint(std::stringstream &stream, coords start, coords end, std::stringstream &var1, std::stringstream &var2, std::map< unsigned, std::map<std::string, std::map<std::string, bool> > > usedConstraints){
    if((!usedConstraints[start.row][var1.str()][var2.str()]) && (!usedConstraints[start.row][var2.str()][var1.str()])){
        usedConstraints[start.row][var1.str()][var2.str()] = 1;
        if(var1.str() == var2.str()){
            #ifdef JSON
                stream << "{row: '" << start.row << "', col: '" << start.col << "',  undefined:'" << var1.str() << "'}";
            #else
                stream << start.row << " " << start.col << " " << end.row << " " << end.col << " " << var1.str();
            #endif
        }else{
            #ifdef JSON
                stream << "{row: '" << start.row << "', col: '" << start.col << "', var1: '" << var1.str() << "', var2: '" << var2.str() << "'}";
            #else
                stream << start.row << " " << start.col << " " << end.row << " " << end.col << " " << var1.str() << " " << var2.str();
            #endif
        }
        return true;
    }
    return false;
}

std::string LvalueTable::makeConstraints(){
    std::stringstream ret;
    coords start, end;
    std::map< unsigned, std::map<std::string, std::map<std::string, bool> > > usedConstraints;
    std::stringstream var1, var2;
    //bool skipFirstOccurenceOfSameObject = false;
    #ifdef JSON
        ret << "The following is a set of constraints under which there was undefined behavior detected by the SPA:" << std::endl << "{constraints: [" << std::endl;
    #endif
    for(std::map<clang::Stmt*, std::vector<Tag> >::iterator i = this->table.begin(); i != this->table.end(); ++i){
        start.row = this->CI.getSourceManager().getExpansionLineNumber(i->first->getLocStart());
        start.col = this->CI.getSourceManager().getExpansionColumnNumber(i->first->getLocStart());
        end.row = this->CI.getSourceManager().getExpansionLineNumber(i->first->getLocEnd());
        end.col = this->CI.getSourceManager().getExpansionColumnNumber(i->first->getLocEnd());

        switch(i->first->getStmtClass()){

        case clang::Stmt::CompoundStmtClass: // {}
        case clang::Stmt::ParenExprClass: // ()
        case clang::Stmt::ConditionalOperatorClass: // ? :
        break;

        case clang::Stmt::BinaryOperatorClass:
            if(static_cast<clang::BinaryOperator*>(i->first)->isAssignmentOp()){
                for(int j=0; j<2; ++j){ //child node index
                    clang::Stmt::child_iterator children = i->first->child_begin();
                    std::advance(children,j);
                    std::vector<Tag> child = this->table[*(children)];
                    for(std::vector<Tag>::iterator k=child.begin(); k!=child.end(); ++k){
                        var1.str("");
                        var1 << this->printLvl(k->lvaluelvl) << k->D->getNameAsString();
                        //skipFirstOccurenceOfSameObject = true;
                        if(!k->sideEffect){
                            continue;
                        }
                        for(std::vector<Tag>::iterator l=i->second.begin(); l!=i->second.end(); ++l){
                            if(j==l->childIndex){//do not iterate over self
                                continue;
                            }
                            var2.str("");
                            var2 << this->printLvl(l->lvaluelvl) << l->D->getNameAsString();
                            /*if(var1.str() == var2.str()){
                                if(skipFirstOccurenceOfSameObject){
                                    skipFirstOccurenceOfSameObject = false;
                                    continue;
                                }
                            }*/
                            if(k->lvaluelvl == 0 && l->lvaluelvl == 0 && var1.str() != var2.str()){//two non-pointers do not alias for sure
                                continue;
                            }
                            if(k->isRestrict && l->isRestrict && var1.str()!=var2.str()){//two different "restrict" variables do not alias for sure
                                continue;
                            }
                            if(this->addConstraint(ret, start, end, var1, var2, usedConstraints)){
                                #ifdef JSON
                                    ret << "," << std::endl;
                                #else
                                    ret << std::endl;
                                #endif
                            }
                        }
                    }
                }
            }else{
                goto defaultClass;
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
                    if(j->childIndex==k->childIndex){//do not iterate over self
                        continue;
                    }
                    if(j==k){//FIXME is this necessary?
                        continue;
                    }
                    if(j->lvaluelvl == 0 && k->lvaluelvl == 0 && var1.str() != var2.str()){//two non-pointers do not alias for sure
                        continue;
                    }
                    if(j->isRestrict && k->isRestrict && var1.str()!=var2.str()){//two different "restrict" variables do not alias for sure
                        continue;
                    }
                    var1.str("");
                    var2.str("");
                    var1 << this->printLvl(j->lvaluelvl) << j->D->getNameAsString();
                    var2 << this->printLvl(k->lvaluelvl) << k->D->getNameAsString();
                    if(this->addConstraint(ret, start, end, var1, var2, usedConstraints)){
                        #ifdef JSON
                            ret << "," << std::endl;
                        #else
                            ret << std::endl;
                        #endif
                    }
                }
            }
        break;
        }
    }
    #ifdef JSON
        ret << "]}" << std::endl;
    #endif
    return ret.str();
}
