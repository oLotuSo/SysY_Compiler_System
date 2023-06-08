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
    void output(int level);
    SymbolEntry* getSymPtr() {return symbolEntry;};
    virtual int getValue() { return -1; };
    virtual Type* getType() { return type; };
    bool isExpr() const { return exprType == EXPR; };
    bool isDeclExpr() const { return exprType == DECLEXPR; };
    bool isITBExpr() const { return exprType == ITBEXPR; };
    bool isUnaryExpr() const { return exprType == UEXPR; };
    void typeCheck(Type* retType = nullptr) { return; };
    SymbolEntry* getSymbolEntry() { return symbolEntry; };
    void genCode();
    Type* getOriginType() { return type; };
};


class ITBExpr : public ExprNode {
   private:
    ExprNode* expr;

   public:
    ITBExpr(ExprNode* expr): ExprNode(nullptr, ITBEXPR), expr(expr) {
        type = TypeSystem::boolType;
        dst = new Operand(
            new TemporarySymbolEntry(type, SymbolTable::getLabel()));
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
    BinaryExpr(SymbolEntry* se, int op, ExprNode* expr1, ExprNode* expr2);
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
            if (expr->isUnaryExpr()) {
            UnaryExpr* ue = (UnaryExpr*)expr;
            if (ue->op == UnaryExpr::NOT) {
                if (ue->getType() == TypeSystem::intType)
                    ue->setType(TypeSystem::boolType);
                // type = TypeSystem::intType;
            }
        }
        }else if(op==UnaryExpr::SUB){
            //－，输出为int类型
            type = TypeSystem::intType;
            dst = new Operand(se);
            if (expr->isUnaryExpr()) {
            UnaryExpr* ue = (UnaryExpr*)expr;
            if (ue->op == UnaryExpr::NOT)
                if (ue->getType() == TypeSystem::intType)
                    ue->setType(TypeSystem::boolType);
        }
        }
    };
    void output(int level);
    int getValue();
    void typeCheck(Type* retType = nullptr);
    void genCode();
    int getOp() const { return op; };
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
private:
    ExprNode* arrIdx;
    bool left = false;
public:
    Id(SymbolEntry *se, ExprNode* arrIdx = nullptr) : ExprNode(se), arrIdx(arrIdx){
        if(se){
            type = se->getType();
            if (type->isInt()) {
                SymbolEntry* temp = new TemporarySymbolEntry(TypeSystem::intType, SymbolTable::getLabel());
                dst = new Operand(temp);
            } else if (type->isArray()) {
                SymbolEntry* temp = new TemporarySymbolEntry(new PointerType(((ArrayType*)type)->getElementType()),SymbolTable::getLabel());
                dst = new Operand(temp);
            }
        }
    };
    void output(int level);
    SymbolEntry* getSymbolEntry() { return symbolEntry; };
    int getValue();
    void typeCheck(Type* retType = nullptr);
    void genCode();
    Type* getType();
    ExprNode* getArrIdx() { return arrIdx; };
    bool isLeft() const { return left; };
    void setLeft() { left = true; }
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

class StmtNode : public Node {
   private:
    int kind;

   protected:
    enum { IF, IFELSE, WHILE, COMPOUND, RETURN };

   public:
    StmtNode(int kind = -1) : kind(kind){};
    bool isIf() const { return kind == IF; };
    //virtual bool typeCheck(Type* retType = nullptr) = 0;
};

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
            if (expr->isDeclExpr())
                ((DeclExpr*)(this->expr))->fill();
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
    AssignStmt(ExprNode* lval, ExprNode* expr);
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

