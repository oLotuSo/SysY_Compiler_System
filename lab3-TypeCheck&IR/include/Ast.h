#ifndef __AST_H__
#define __AST_H__

#include <fstream>
#include <iostream>
#include <stack>
#include "Operand.h"
#include "Type.h"


class SymbolEntry;
class Unit;
class Function;
class BasicBlock;
class Instruction;
class IRBuilder;

class Node
{
private:
    static int counter;
    int seq;
    Node* next;
protected:
    std::vector<Instruction*> true_list;
    std::vector<Instruction*> false_list;
    static IRBuilder *builder;
    void backPatch(std::vector<Instruction*> &list, BasicBlock*bb);
    std::vector<Instruction*> merge(std::vector<Instruction*> &list1, std::vector<Instruction*> &list2);
public:
    Node();
    int getSeq() const {return seq;};
    static void setIRBuilder(IRBuilder*ib) {builder = ib;};
    virtual void output(int level) = 0;
    void setNext(Node* node);
    virtual void typeCheck(Type* retType = nullptr) = 0;
    virtual void genCode() = 0;
    Node* getNext() { return next; }
    std::vector<Instruction*>& trueList() {return true_list;}
    std::vector<Instruction*>& falseList() {return false_list;}
};

class ExprNode : public Node
{
private:
    int exprType;
protected:
    enum { EXPR, DECLEXPR, ITBEXPR, UEXPR };
    Type* type;
    SymbolEntry *symbolEntry;
    Operand *dst;   // The result of the subtree is stored into dst.
public:
    ExprNode(SymbolEntry *symbolEntry , int exprType=EXPR) :  exprType(exprType) , symbolEntry(symbolEntry){};
    Operand* getOperand() {return dst;};
    SymbolEntry* getSymPtr() {return symbolEntry;};
    virtual int getValue() { return 0; };
    virtual Type* getType() { return type; };
    bool isExpr() const { return exprType == EXPR; };
    bool isDeclExpr() const { return exprType == DECLEXPR; };
    bool isITBExpr() const { return exprType == ITBEXPR; };
    bool isUnaryExpr() const { return exprType == UEXPR; };
    SymbolEntry* getSymbolEntry() { return symbolEntry; };
};


class ITBExpr : public ExprNode {
   private:
    ExprNode* expr;

   public:
    ITBExpr(ExprNode* expr): ExprNode(nullptr, ITBEXPR), expr(expr) {
        type = TypeSystem::boolType;
        dst = new Operand(new TemporarySymbolEntry(type, SymbolTable::getLabel()));
    };
    void output(int level);
    ExprNode* getExpr() const { return expr; };
    void typeCheck(Type* retType = nullptr);
    void genCode();
};

class BinaryExpr : public ExprNode
{
private:
    int op;
    ExprNode *expr1, *expr2;
public:
    enum {ADD, SUB,MUL,DIV,MOD, AND, OR, LESS,GRA,LESEQ,GRAEQ,EQ,NEQ};
    BinaryExpr(SymbolEntry *se, int op, ExprNode*expr1, ExprNode*expr2) : ExprNode(se), op(op), expr1(expr1), expr2(expr2){
    //二元运算符类型检查
    //这部分需要判断两个case：数值表达式计算中所有运算数类型是否正确 与 int到bool的隐式类型转换
    dst = new Operand(se);
    std::string op_str;
    switch (op) {
        case ADD: 
            op_str = "+";
            break;
        case SUB:
            op_str = "-";
            break;
        case MUL:
            op_str = "*";
            break;
        case DIV:
            op_str = "/";
            break;
        case MOD:
            op_str = "%";
            break;
        case AND:
            op_str = "&&";
            break;
        case OR:
            op_str = "||";
            break;
        case LESS:
            op_str = "<";
            break;
        case LESEQ:
            op_str = "<=";
            break;
        case GRA:
            op_str = ">";
            break;
        case GRAEQ:
            op_str = ">=";
            break;
        case EQ:
            op_str = "==";
            break;
        case NEQ:
            op_str = "!=";
            break;
    }
    if(expr1->getType()->isVoid() || expr2->getType()->isVoid()){ //不能为void类型
        fprintf(stderr, "invalid operand of type \'void\' to binary \'opeartor%s\'\n", op_str.c_str());
    }
    if(op >= BinaryExpr::AND && op <= BinaryExpr::NEQ){ //是逻辑表达式
        type = TypeSystem::boolType; //这种情况是bool类型
        if(op == BinaryExpr::AND || op == BinaryExpr::OR ){//运算符为逻辑与和或时，做int至bool的隐式类型转换
            if (expr1->getType()->isInt() && expr1->getType()->getSize() == 32) {
                ITBExpr* temp = new ITBExpr(expr1);
                this->expr1 = temp;
            }
            if (expr2->getType()->isInt() && expr2->getType()->getSize() == 32) {
                ITBExpr* temp = new ITBExpr(expr2);
                this->expr2 = temp;
            }
        }
    }else{
        type = TypeSystem::intType; //其余情况全是整型
    }
    };
    void output(int level);
    int getValue();
    void typeCheck(Type* retType = nullptr);
    void genCode();
};

class UnaryExpr : public ExprNode {
   private:
    int op;
    ExprNode* expr;

   public:
    enum { NOT, SUB };
    UnaryExpr(SymbolEntry* se, int op, ExprNode* expr) : ExprNode(se,UEXPR), op(op), expr(expr){
        //判断同上
        std::string op_str = (op == UnaryExpr::SUB ) ? "-" : "!";
        if (expr->getType()->isVoid()) {
            fprintf(stderr,"invalid operand of type \'void\' to unary \'opeartor%s\'\n",op_str.c_str());
        }
        if(op==UnaryExpr::NOT){
            //!，输出为bool类型
            type = TypeSystem::boolType;
            dst = new Operand(se);
        }else if(op==UnaryExpr::SUB){
            //－，输出为int类型
            type = TypeSystem::intType;
            dst = new Operand(se);
        }
    };
    void output(int level);
    int getValue();
    void typeCheck(Type* retType = nullptr);
    void genCode();
    void setType(Type* type) { this->type = type; }
};

class CallExpr : public ExprNode {
   private:
    ExprNode* param;

   public:
    CallExpr(SymbolEntry* se, ExprNode* param = nullptr);
    void output(int level);
    void typeCheck(Type* retType = nullptr);
    void genCode();
};

class Constant : public ExprNode
{
public:
    Constant(SymbolEntry *se) : ExprNode(se){
        dst = new Operand(se);
        type = TypeSystem::intType;
    };
    void output(int level);
    int getValue();
    void typeCheck(Type* retType = nullptr);
    void genCode();
};

class Id : public ExprNode
{
public:
    Id(SymbolEntry *se) : ExprNode(se){
        if(se){
            type = se->getType();
            SymbolEntry *temp = new TemporarySymbolEntry(se->getType(), SymbolTable::getLabel()); 
            st = new Operand(temp);
        }
    };
    void output(int level);
    SymbolEntry* getSymbolEntry() { return symbolEntry; };
    int getValue();
    void typeCheck(Type* retType = nullptr);
    void genCode();
    Type* getType();
};


class DeclExpr : public ExprNode { //构建声明链
   private:
    ExprNode* expr;
    int dcnt;

   public:
    DeclExpr(SymbolEntry* se, ExprNode* expr = nullptr): ExprNode(se, DECLEXPR), expr(expr) {
        type = se->getType();
        dcnt = 0;
    };
    void output(int level);
    ExprNode* getExpr() const { return expr; };
    void addExpr(ExprNode* expr);
    bool isEmpty() { return dcnt == 0; };
    bool isFull();
    void fill();
    void typeCheck(Type* retType = nullptr);
    void genCode();
};

class StmtNode : public Node
{};

class CompoundStmt : public StmtNode
{
private:
    StmtNode *stmt;
public:
    CompoundStmt(StmtNode* stmt = nullptr) : stmt(stmt){};
    void output(int level);
    void typeCheck(Type* retType = nullptr);
    void genCode();
};

class SeqNode : public StmtNode
{
private:
    StmtNode *stmt1, *stmt2;
public:
    SeqNode(StmtNode *stmt1, StmtNode *stmt2) : stmt1(stmt1), stmt2(stmt2){};
    void output(int level);
    void typeCheck(Type* retType = nullptr);
    void genCode();
};

class DeclStmt : public StmtNode
{
private:
    Id *id;
    ExprNode* expr;
public:
    DeclStmt(Id *id , ExprNode* expr = nullptr) : id(id),expr(expr){
        if (expr) {
            this->expr = expr;
        }
    };
    void output(int level);
    Id* getId() { return id; };
    void typeCheck(Type* retType = nullptr);
    void genCode();
};

class BlankStmt : public StmtNode {
   public:
    BlankStmt(){};
    void output(int level);
    void typeCheck(Type* retType = nullptr);
    void genCode();
};

class IfStmt : public StmtNode
{
private:
    ExprNode *cond;
    StmtNode *thenStmt;
public:
    IfStmt(ExprNode *cond, StmtNode *thenStmt) : cond(cond), thenStmt(thenStmt){
        //cond中int到bool的隐式类型转换
        if (cond->getType()->isInt() && cond->getType()->getSize() == 32) {
            ITBExpr* temp = new ITBExpr(cond);
            this->cond = temp;
        }
    };
    void output(int level);
    void typeCheck(Type* retType = nullptr);
    void genCode();
};

class IfElseStmt : public StmtNode
{
private:
    ExprNode *cond;
    StmtNode *thenStmt;
    StmtNode *elseStmt;
public:
    IfElseStmt(ExprNode *cond, StmtNode *thenStmt, StmtNode *elseStmt) : cond(cond), thenStmt(thenStmt), elseStmt(elseStmt) {
        //同上
        if (cond->getType()->isInt() && cond->getType()->getSize() == 32) {
            ITBExpr* temp = new ITBExpr(cond);
            this->cond = temp;
        }
    };
    void output(int level);
    void typeCheck(Type* retType = nullptr);
    void genCode();
};

class ForStmt : public StmtNode
{
private:
    ExprNode *cond1;
    ExprNode *cond2;
    ExprNode *cond3;
    StmtNode *forStmt;
public:
    ForStmt(ExprNode *cond1,ExprNode *cond2,ExprNode *cond3, StmtNode *forStmt) : cond1(cond1),cond2(cond2),cond3(cond3), forStmt(forStmt){};
    void output(int level);
};

class WhileStmt : public StmtNode
{
private:
    ExprNode *cond;
    StmtNode *whileStmt;
    BasicBlock *cond_bb, *end_bb; //在break和continue中会用到
public:
    WhileStmt(ExprNode *cond, StmtNode *whileStmt=nullptr) : cond(cond), whileStmt(whileStmt){
        //同if
        if (cond->getType()->isInt() && cond->getType()->getSize() == 32) {
            ITBExpr* temp = new ITBExpr(cond);
            this->cond = temp;
        }
    };
    void output(int level);
    void setStmt(StmtNode* whileStmt){this->whileStmt = whileStmt;};
    void typeCheck(Type* retType = nullptr);
    void genCode();
    BasicBlock* get_cond_bb(){return this->cond_bb;};
    BasicBlock* get_end_bb(){return this->end_bb;};
};

class ReturnStmt : public StmtNode
{
private:
    ExprNode *retValue;
public:
    ReturnStmt(ExprNode* retValue=nullptr) : retValue(retValue) {};
    void output(int level);
    void typeCheck(Type* retType = nullptr);
    void genCode();
};

class BreakStmt : public StmtNode {
    private:
    StmtNode * whileStmt;
   public:
    BreakStmt(StmtNode* whileStmt){this->whileStmt=whileStmt;};
    void output(int level);
    void typeCheck(Type* retType = nullptr);
    void genCode();
};

class ContinueStmt : public StmtNode {
    private:
    StmtNode * whileStmt;
   public:
    ContinueStmt(StmtNode* whileStmt){this->whileStmt=whileStmt;};
    void output(int level);
    void typeCheck(Type* retType = nullptr);
    void genCode();
};

class AssignStmt : public StmtNode
{
private:
    ExprNode *lval;
    ExprNode *expr;
public:
    AssignStmt(ExprNode *lval, ExprNode *expr) : lval(lval), expr(expr) {
        //错误的赋值，包括对常量赋值，以及对int型变量赋void值
        Type* type = ((Id*)lval)->getType();
        if(type->isInt()){//对常量赋值
            if (((IntType*)type)->isConst()) {
                fprintf(stderr,"Expression must be a modifiable lvalue \n");
            }
        }
        else if(type->isInt() && !expr->getType()->isInt()){ //对int型变量赋void值
            fprintf(stderr,"cannot initialize a variable of type \'int\' with an rvalue of type \'%s\'\n", expr->getType()->toStr().c_str());
        }
    };
    void output(int level);
    void typeCheck(Type* retType = nullptr);
    void genCode();
};

class ExprStmt : public StmtNode {
   private:
    ExprNode* expr;

   public:
    ExprStmt(ExprNode* expr) : expr(expr){};
    void output(int level);
    void typeCheck(Type* retType = nullptr);
    void genCode();
};

class FunctionDef : public StmtNode
{
private:
    SymbolEntry *se;
    DeclStmt* decl;
    StmtNode *stmt;
public:
    FunctionDef(SymbolEntry* se, DeclStmt* decl, StmtNode* stmt): se(se), decl(decl), stmt(stmt){};
    void output(int level);
    void typeCheck(Type* retType = nullptr);
    void genCode();
    SymbolEntry* getSymbolEntry() { return se; };
};

class Ast
{
private:
    Node* root;
public:
    Ast() {root = nullptr;}
    void setRoot(Node*n) {root = n;}
    void output();
    void typeCheck(Type* retType = nullptr);
    void genCode(Unit *unit);
};

#endif

