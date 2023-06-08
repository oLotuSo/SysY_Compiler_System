#include "Unit.h"
#include <algorithm>
#include <stack>
#include <string>
#include "Ast.h"
#include "SymbolTable.h"
#include "Type.h"
extern FILE* yyout;

void Unit::insertFunc(Function *f)
{
    func_list.push_back(f);
}

void Unit::removeFunc(Function *func)
{
    func_list.erase(std::find(func_list.begin(), func_list.end(), func));
}

void Unit::insertGlobal(SymbolEntry* se) {
    global_list.push_back(se);
}

void Unit::insertDeclare(SymbolEntry* se) {
    auto it = std::find(declare_list.begin(), declare_list.end(), se);
    if (it == declare_list.end()) {
        declare_list.push_back(se);
    }
}

void Unit::output() const
{
    for (auto se : global_list) 
        fprintf(yyout, "%s = global %s %d, align 4\n", se->toStr().c_str(),se->getType()->toStr().c_str(),((IdentifierSymbolEntry*)se)->getValue());
    for (auto &func : func_list)
        func->output();
    for (auto se : declare_list) {
        FunctionType* type = (FunctionType*)(se->getType());
        std::string str = type->toStr();
        std::string name = str.substr(0, str.find('('));
        std::string param = str.substr(str.find('('));
        fprintf(yyout, "declare %s %s%s\n", type->getRetType()->toStr().c_str(),se->toStr().c_str(), param.c_str());
    }
}

Unit::~Unit()
{
    for(auto &func:func_list)
        delete func;
    for (auto& se : global_list)
        delete se;
}
