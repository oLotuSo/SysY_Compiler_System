#include "Ast.h"
#include "SymbolTable.h"
#include "Unit.h"
#include "Instruction.h"
#include "IRBuilder.h"
#include <string>
#include <iostream>
#include "Type.h"
#include "Unit.h"
extern Unit unit;

extern FILE *yyout;
int Node::counter = 0;
int isret=0; //判断是否为return语句
IRBuilder* Node::builder = nullptr;

Node::Node()
{
    seq = counter++;
    next = nullptr;
}

void Node::setNext(Node* node) {
    Node* n = this;
    while (n->getNext()) {
        n = n->getNext();
    }
    if (n == this) {
        this->next = node;
    } else {
        n->setNext(node);
    }
}

void Node::backPatch(std::vector<Instruction*> &list, BasicBlock*bb)
{
    for(auto &inst:list)
    {
        if(inst->isCond())
            dynamic_cast<CondBrInstruction*>(inst)->setTrueBranch(bb);
        else if(inst->isUncond())
            dynamic_cast<UncondBrInstruction*>(inst)->setBranch(bb);
    }
}

std::vector<Instruction*> Node::merge(std::vector<Instruction*> &list1, std::vector<Instruction*> &list2)
{
    std::vector<Instruction*> res(list1);
    res.insert(res.end(), list2.begin(), list2.end());
    return res;
}

CallExpr::CallExpr(SymbolEntry* se, ExprNode* param): ExprNode(se), param(param)
{
    //判断 函数调用时 形参以及实参类型和数目
    dst = nullptr;
    SymbolEntry* s = se;
    int paramCnt = 0;
    ExprNode* temp = param;
    while (temp) { //统计实参个数
        paramCnt++;
        temp = (ExprNode*)(temp->getNext());
    }
    while (s) {
        Type* type = s->getType();
        std::vector<Type*> params = ((FunctionType*)type)->getParamsType();
        if ((long unsigned int)paramCnt == params.size()) {
            this->symbolEntry = s;
            break;
        }
        s = s->getNext();
    }
    if (symbolEntry) {
        Type* type = symbolEntry->getType();
        this->type = ((FunctionType*)type)->getRetType();
        if (this->type != TypeSystem::voidType) {
            SymbolEntry* se = new TemporarySymbolEntry(this->type, SymbolTable::getLabel());
            dst = new Operand(se);
        }
        std::vector<Type*> params = ((FunctionType*)type)->getParamsType();
        ExprNode* temp = param;
        for (auto it = params.begin(); it != params.end(); it++) {
            if (temp == nullptr) {
                fprintf(stderr, "too few arguments to function %s %s\n",symbolEntry->toStr().c_str(), type->toStr().c_str());
                break;
            } else if ((*it)->getKind() != temp->getType()->getKind())
                fprintf(stderr, "parameter's type %s can't convert to %s\n",temp->getType()->toStr().c_str(), (*it)->toStr().c_str());
            temp = (ExprNode*)(temp->getNext());
        }
        if (temp != nullptr) {
            fprintf(stderr, "too many arguments to function %s %s\n",symbolEntry->toStr().c_str(), type->toStr().c_str());
        }
    }
    if (((IdentifierSymbolEntry*)se)->isSysy())unit.insertDeclare(se);
}

void Ast::genCode(Unit *unit)
{
    IRBuilder *builder = new IRBuilder(unit);
    Node::setIRBuilder(builder);
    root->genCode();
}

void FunctionDef::genCode()
{
    Unit *unit = builder->getUnit();
    Function *func = new Function(unit, se);
    BasicBlock *entry = func->getEntry();
    // set the insert point to the entry basicblock of this function.
    builder->setInsertBB(entry);
    if(decl)decl->genCode();
    if(stmt)stmt->genCode();

    /**
     * Construct control flow graph. You need do set successors and predecessors for each basic block.
     * Todo
    */
    //函数中会有多个BB，我们遍历所有的基本块，查找它的最后一个语句，建立前驱和后继
    for(auto block= func->begin(); block != func->end(); block++)
    {
        Instruction* i = (*block)->begin();
        Instruction* last = (*block)->rbegin(); //双向循环链表，第一条的前一个就是最后一条
        while (i != last) {
            if (i->isCond() || i->isUncond()) {
                (*block)->remove(i);
            }
            i = i->getNext();
        }
        if (last->isCond()) {  //最后一条是条件跳转，那么有两个后继
            BasicBlock *truebranch, *falsebranch;
            truebranch = dynamic_cast<CondBrInstruction*>(last)->getTrueBranch();
            falsebranch = dynamic_cast<CondBrInstruction*>(last)->getFalseBranch();
            if (truebranch->empty()) {
                new RetInstruction(nullptr, truebranch);

            } else if (falsebranch->empty()) {
                new RetInstruction(nullptr, falsebranch);
            }
            (*block)->addSucc(truebranch);
            (*block)->addSucc(falsebranch);
            truebranch->addPred(*block);
            falsebranch->addPred(*block);
        } else if (last->isUncond())  //最后一条是无条件跳转，那么有1个后继
        {
            BasicBlock* dst = dynamic_cast<UncondBrInstruction*>(last)->getBranch();
            (*block)->addSucc(dst);
            dst->addPred(*block);
            if (dst->empty()) {
                if (((FunctionType*)(se->getType()))->getRetType() == TypeSystem::intType)
                    new RetInstruction(new Operand(new ConstantSymbolEntry(TypeSystem::intType, 0)),dst); //int默认return 0
                else if (((FunctionType*)(se->getType()))->getRetType() == TypeSystem::voidType)
                    new RetInstruction(nullptr, dst);//void默认return void
            }
        }
        //返回指令没有后继，不操作
        //不是以上三种情况
        else if (!last->isRet()) {
            if (((FunctionType*)(se->getType()))->getRetType() == TypeSystem::voidType) new RetInstruction(nullptr, *block);
        }
    }
}

void BinaryExpr::genCode()
{
    BasicBlock *bb = builder->getInsertBB();
    Function *func = bb->getParent();
    if (op == AND) //注意短路规则
    {
        BasicBlock *trueBB = new BasicBlock(func);  // if the result of lhs is true, jump to the trueBB.
        expr1->genCode();
        backPatch(expr1->trueList(), trueBB);
        builder->setInsertBB(trueBB);               // set the insert point to the trueBB so that intructions generated by expr2 will be inserted into it.
        expr2->genCode();
        true_list = expr2->trueList();
        false_list = merge(expr1->falseList(), expr2->falseList());
    }
    else if(op == OR) //同样注意短路规则
    {
        BasicBlock *falseBB = new BasicBlock(func); 
        expr1->genCode();
        backPatch(expr1->falseList(), falseBB); 
        builder->setInsertBB(falseBB);              
        expr2->genCode();
        true_list = merge(expr1->trueList(), expr2->trueList());
        false_list =expr2->falseList();
    }
    else if(op >= LESS && op <= NEQ) 
    {
        expr1->genCode();
        expr2->genCode();
        Operand* src1 = expr1->getOperand();
        Operand* src2 = expr2->getOperand();
        
        //例如出现 1<2<3 的语句，需要扩展
        if (src1->getType()->getSize() == 1) { //如果子表达式不是int32则零扩展
            Operand* dst = new Operand(new TemporarySymbolEntry(TypeSystem::intType, SymbolTable::getLabel()));
            new ZextInstruction(dst, src1, bb);
            src1 = dst;
        }
        if (src2->getType()->getSize() == 1) {
            Operand* dst = new Operand(new TemporarySymbolEntry(TypeSystem::intType, SymbolTable::getLabel()));
            new ZextInstruction(dst, src2, bb);
            src2 = dst;
        }

        int opcode;
        switch (op)
        {
            case LESS:
                opcode = CmpInstruction::L;
                break;
            case LESEQ:
                opcode = CmpInstruction::LE;
                break;
            case GRA:
                opcode = CmpInstruction::G;
                break;
            case GRAEQ:
                opcode = CmpInstruction::GE;
                break;
            case EQ:
                opcode = CmpInstruction::E;
                break;
            case NEQ:
                opcode = CmpInstruction::NE;
                break;
        }
        new CmpInstruction(opcode, dst, src1, src2, bb); //生成比较指令
        
        //这些符号会在条件跳转中回填，所以需要在这里设置后继的true_list和false_list
        BasicBlock* trueBB = new BasicBlock(func);
        BasicBlock* tempbb = new BasicBlock(func);
        BasicBlock* falseBB = new BasicBlock(func);
        true_list.push_back(new CondBrInstruction(trueBB, tempbb, dst, bb));
        false_list.push_back(new UncondBrInstruction(falseBB, tempbb));
    }
    else if(op >= ADD && op <= MOD)
    {
        expr1->genCode();
        expr2->genCode();
        Operand *src1 = expr1->getOperand();
        Operand *src2 = expr2->getOperand();
        int opcode;
        switch (op)
        {
            case ADD:
                opcode = BinaryInstruction::ADD;
                break;
            case SUB:
                opcode = BinaryInstruction::SUB;
                break;
            case MUL:
                opcode = BinaryInstruction::MUL;
                break;
            case DIV:
                opcode = BinaryInstruction::DIV;
                break;
            case MOD:
                opcode = BinaryInstruction::MOD;
                break;
        }
        new BinaryInstruction(opcode, dst, src1, src2, bb);
    }
}

void UnaryExpr::genCode()
{
    //思路类似Binary 
    expr->genCode();
    BasicBlock* bb = builder->getInsertBB();
    if (op == NOT) {
        Operand* src = expr->getOperand();
        if (expr->getType()->getSize() == 32) {
            Operand* temp = new Operand(new TemporarySymbolEntry(TypeSystem::boolType, SymbolTable::getLabel()));
            new CmpInstruction(CmpInstruction::NE, temp, src,new Operand(new ConstantSymbolEntry(TypeSystem::intType, 0)),bb);//利用NE指令，与0做对比，输出i1类型
            src = temp;
        }
        new XorInstruction(dst, src, bb); //和1做异或代表 取非
    } else if (op == SUB) {
        Operand* src2;
        Operand* src1 = new Operand(new ConstantSymbolEntry(TypeSystem::intType, 0)); //取负相当于用0减这个数
        if (expr->getType()->getSize() == 1) {
            src2 = new Operand(new TemporarySymbolEntry(TypeSystem::intType, SymbolTable::getLabel()));
            new ZextInstruction(src2, expr->getOperand(), bb); //bool类型零扩展指令，高位补0
        } else
            src2 = expr->getOperand();
        new BinaryInstruction(BinaryInstruction::SUB, dst, src1, src2, bb);
    }
}

void CallExpr::genCode() 
{
    //新增 call指令
    std::vector<Operand*> operands;
    ExprNode* temp = param;
    while (temp) {
        temp->genCode();
        operands.push_back(temp->getOperand());
        temp = (ExprNode*)temp->getNext(); //获取所有参数
    }
    BasicBlock* bb = builder->getInsertBB();
    new CallInstruction(dst, symbolEntry, operands, bb);
}
void Constant::genCode()
{
    // we don't need to generate code.
}

void Id::genCode()
{
    BasicBlock *bb = builder->getInsertBB();
    Operand *addr = dynamic_cast<IdentifierSymbolEntry*>(symbolEntry)->getAddr();
    new LoadInstruction(dst, addr, bb);
}

void ITBExpr::genCode(){
    expr->genCode();
    //与0做NE，输出类型为i1，代表隐式类型转换
    BasicBlock* bb = builder->getInsertBB();
    Function* func = bb->getParent();

    BasicBlock* trueBB = new BasicBlock(func);
    BasicBlock* tempbb = new BasicBlock(func);
    BasicBlock* falseBB = new BasicBlock(func);

    new CmpInstruction(CmpInstruction::NE, this->dst, this->expr->getOperand(),new Operand(new ConstantSymbolEntry(TypeSystem::intType, 0)), bb);

    this->trueList().push_back(new CondBrInstruction(trueBB, tempbb, this->dst, bb));
    this->falseList().push_back(new UncondBrInstruction(falseBB, tempbb));
}

void DeclExpr::genCode(){
    //do nothing
}

void IfStmt::genCode()
{
    Function *func;
    BasicBlock *then_bb, *end_bb;

    func = builder->getInsertBB()->getParent();
    then_bb = new BasicBlock(func);
    end_bb = new BasicBlock(func);

    cond->genCode();
    backPatch(cond->trueList(), then_bb);
    backPatch(cond->falseList(), end_bb);

    builder->setInsertBB(then_bb);
    thenStmt->genCode();
    then_bb = builder->getInsertBB();
    new UncondBrInstruction(end_bb, then_bb);

    builder->setInsertBB(end_bb);
}

void IfElseStmt::genCode()
{
    Function* func;
    BasicBlock *then_bb, *end_bb, *else_bb;

    func = builder->getInsertBB()->getParent();
    then_bb = new BasicBlock(func);
    end_bb = new BasicBlock(func);
    else_bb = new BasicBlock(func);

    cond->genCode();

    backPatch(cond->trueList(), then_bb);
    backPatch(cond->falseList(), else_bb); //表达式为假 将falseList更改为else_bb回填

    builder->setInsertBB(then_bb);
    thenStmt->genCode();
    then_bb = builder->getInsertBB();
    new UncondBrInstruction(end_bb, then_bb);

    builder->setInsertBB(else_bb);
    elseStmt->genCode();
    else_bb = builder->getInsertBB();
    new UncondBrInstruction(end_bb, else_bb); //增加else部分

    builder->setInsertBB(end_bb);
}

void CompoundStmt::genCode()
{
    if (stmt)
        stmt->genCode();
}

void SeqNode::genCode()
{
    stmt1->genCode();
    stmt2->genCode();
}

void DeclStmt::genCode()//声明语句，包括全局、局部以及参数
{
    IdentifierSymbolEntry *se = dynamic_cast<IdentifierSymbolEntry *>(id->getSymPtr());
    if(se->isGlobal())
    {
        Operand *addr;
        SymbolEntry *addr_se;
        addr_se = new IdentifierSymbolEntry(*se);
        addr_se->setType(new PointerType(se->getType()));
        addr = new Operand(addr_se);
        se->setAddr(addr);
        unit.insertGlobal(se);
    }
    else if(se->isLocal() || se->isParam())//增加对于参数的生成
    {
        Function *func = builder->getInsertBB()->getParent();
        BasicBlock *entry = func->getEntry();
        Instruction *alloca;
        Operand *addr, *temp;
        SymbolEntry *addr_se;
        Type *type;
        type = new PointerType(se->getType());
        addr_se = new TemporarySymbolEntry(type, SymbolTable::getLabel());
        addr = new Operand(addr_se);
        alloca = new AllocaInstruction(addr, se);                   // allocate space for local id in function stack.
        entry->insertFront(alloca);                                 // allocate instructions should be inserted into the begin of the entry block.
        if(se->isParam()){
            temp=se->getAddr();
        }
        se->setAddr(addr);                                          // set the addr operand in symbol entry so that we can use it in subsequent code generation.
        if (expr) {
            
                BasicBlock* bb = builder->getInsertBB();
                expr->genCode();
                Operand* src = expr->getOperand();
                new StoreInstruction(addr, src, bb);
            
        }
        if (se->isParam()) {
            BasicBlock* bb = builder->getInsertBB();
            new StoreInstruction(addr, temp, bb);
        }
    }
    if (this->getNext()) 
    {
        this->getNext()->genCode();
    }
}


void BlankStmt::genCode()
{
    //do nothing
}

void BreakStmt::genCode()
{
    BasicBlock *bb=builder->getInsertBB();
    Function *func = bb->getParent();
    new UncondBrInstruction(((WhileStmt*)whileStmt)->get_end_bb(), bb);
    BasicBlock* break_bb = new BasicBlock(func);
    builder->setInsertBB(break_bb); //break下一步跳转至 end_bb
}

void ContinueStmt::genCode()
{
    BasicBlock *bb=builder->getInsertBB();
    Function *func = bb->getParent();
    new UncondBrInstruction(((WhileStmt*)whileStmt)->get_cond_bb(), bb);
    BasicBlock* continue_bb = new BasicBlock(func);
    builder->setInsertBB(continue_bb); //continue下一步跳转至 cond_bb
}

void ExprStmt::genCode()
{
    expr->genCode();
}

void WhileStmt::genCode()
{
    //思路非常类似 if
    Function *func;
    BasicBlock *then_bb, *end_bb, *cond_bb; //新建一个condition basic block，在每次执行完循环体后回到cond_bb
    BasicBlock *bb = builder->getInsertBB();
    func = builder->getInsertBB()->getParent();
    then_bb = new BasicBlock(func);
    end_bb = new BasicBlock(func);
    cond_bb = new BasicBlock(func);

    this->cond_bb=cond_bb;
    this->end_bb=end_bb; // 保存bb,用于break和continue
    new UncondBrInstruction(cond_bb,bb); 
    
    builder->setInsertBB(cond_bb);
    cond->genCode();
    backPatch(cond->trueList(), then_bb);
    backPatch(cond->falseList(), end_bb);

    builder->setInsertBB(then_bb);
    whileStmt->genCode();
    then_bb = builder->getInsertBB();
    new UncondBrInstruction(cond_bb, then_bb); //回到cond_bb而非end_bb

    builder->setInsertBB(end_bb);
}

void ReturnStmt::genCode()
{
    BasicBlock *bb = builder->getInsertBB();
    Operand* src = nullptr; //void类型返回是空
    if(retValue){ //非void
        retValue->genCode();
        src = retValue->getOperand();
    }
    new RetInstruction(src, bb);
}

void AssignStmt::genCode()
{
    BasicBlock *bb = builder->getInsertBB();
    expr->genCode();
    Operand *addr = dynamic_cast<IdentifierSymbolEntry*>(lval->getSymPtr())->getAddr();
    Operand *src = expr->getOperand();
    /***
     * We haven't implemented array yet, the lval can only be ID. So we just store the result of the `expr` to the addr of the id.
     * If you want to implement array, you have to caculate the address first and then store the result into it.
     * 数组需要在这里变更
     */
    new StoreInstruction(addr, src, bb);
}

void Ast::typeCheck(Type* retType)
{
    if(root != nullptr)
        root->typeCheck();
}

void FunctionDef::typeCheck(Type* retType)
{
    //判断 非void函数需要有return语句 以及 函数语句块为空时必须是void函数
    SymbolEntry* se = this->getSymbolEntry();
    Type* ret = ((FunctionType*)(se->getType()))->getRetType();
    StmtNode* stmt = this->stmt;
    if (stmt == nullptr) {
        if (ret != TypeSystem::voidType)
            fprintf(stderr, "non-void funtion must return a value\n");
    }
    stmt->typeCheck(ret);
    if(!isret && ret != TypeSystem::voidType){  //不是ret语句
        fprintf(stderr, "function does not have a return statement\n");
    }
    isret=0;
}

void BinaryExpr::typeCheck(Type* retType)
{
    // 在构造函数完成初始化
    return;
}
void UnaryExpr::typeCheck(Type* retType){
    return;
}

void CallExpr::typeCheck(Type* retType){
    return;
}

void Constant::typeCheck(Type* retType)
{
    return;
}

void Id::typeCheck(Type* retType)
{
    return;
}

void ITBExpr::typeCheck(Type* retType){
    //do nothing
}

void DeclExpr::typeCheck(Type* retType){
    //do nothing
}

void IfStmt::typeCheck(Type* retType)
{
    if (thenStmt)
        thenStmt->typeCheck(retType);
}

void IfElseStmt::typeCheck(Type* retType)
{
    if (thenStmt)
        thenStmt->typeCheck(retType);
    if (elseStmt)
        elseStmt->typeCheck(retType);
}

void CompoundStmt::typeCheck(Type* retType)
{
    if (stmt)
        stmt->typeCheck(retType);
}

void SeqNode::typeCheck(Type* retType)
{
    //分别对stmt1和stmt2进行类型检查
    if(stmt1){
         stmt1->typeCheck(retType);
    }
    if(stmt2){
         stmt2->typeCheck(retType);
    }
}

void DeclStmt::typeCheck(Type* retType)
{
    //do nothing
}

void BlankStmt::typeCheck(Type* retType){
    //do nothing
}

void BreakStmt::typeCheck(Type* retType){
    //do nothing
}

void ContinueStmt::typeCheck(Type* retType){
    //do nothing
}

void ExprStmt::typeCheck(Type* retType){
    //do nothing
}

void WhileStmt::typeCheck(Type* retType){
    if (whileStmt)
        whileStmt->typeCheck(retType);
}

void ReturnStmt::typeCheck(Type* retType)
{
    // Todo
    //判断 return语句返回类型与函数声明类型是否匹配
    //函数类型retType，return语句retValue
    bool err=false;
    if (!retType) {
        fprintf(stderr, "expected unqualified-id\n");
        err=true;
    }
    else if (!retValue && !retType->isVoid()) {//函数要求返回，但未return
        fprintf(stderr,"return-statement with no value, in function returning \'%s\'\n",retType->toStr().c_str());
        err=true;
    }
    else if (retValue && retType->isVoid()) {//函数是void，却有返回值
        fprintf(stderr,"return-statement with a value, in function returning \'void\'\n");
        err=true;
    }
    if(retValue){
        Type* type = retValue->getType();
        if (!err && type != retType) { //返回类型不匹配,最后再判定，保证二者有效
            fprintf(stderr,"cannot initialize return object of type \'%s\' with an rvalue of type \'%s\'\n",retType->toStr().c_str(), type->toStr().c_str());
        }
    }
    err = false;
    isret=1;
}

void AssignStmt::typeCheck(Type* retType)
{
    return;
}

Type* Id::getType() {
    SymbolEntry* se = this->getSymbolEntry();
    if (!se)
        return TypeSystem::voidType;
    Type* type = se->getType();
    return type;
}

void Ast::output()
{
    fprintf(yyout, "program\n");
    if(root != nullptr)
        root->output(4);
}

void BinaryExpr::output(int level)
{
    std::string op_str;
    switch(op)
    {
        case ADD:
            op_str = "add";
            break;
        case SUB:
            op_str = "sub";
            break;
        case MUL:
            op_str = "mul";
            break;
        case DIV:
            op_str = "div";
            break;
        case MOD:
            op_str = "mod";
            break;
        case AND:
            op_str = "and";
            break;
        case OR:
            op_str = "or";
            break;
        case LESS:
            op_str = "less";
            break;
        case GRA:
            op_str = "gra";
            break;
        case LESEQ:
            op_str = "leseq";
            break;
        case GRAEQ:
            op_str = "graeq";
            break;
        case EQ:
            op_str = "eq";
            break;
        case NEQ:
            op_str = "neq";
            break;
    }
    fprintf(yyout, "%*cBinaryExpr\top: %s\n", level, ' ', op_str.c_str());
    expr1->output(level + 4);
    expr2->output(level + 4);
}

int BinaryExpr::getValue() {
    int value;
    switch (op) {
        case ADD:
            value = expr1->getValue() + expr2->getValue();
            break;
        case SUB:
            value = expr1->getValue() - expr2->getValue();
            break;
        case MUL:
            value = expr1->getValue() * expr2->getValue();
            break;
        case DIV:
            value = expr1->getValue() / expr2->getValue();
            break;
        case MOD:
            value = expr1->getValue() % expr2->getValue();
            break;
        case AND:
            value = expr1->getValue() && expr2->getValue();
            break;
        case OR:
            value = expr1->getValue() || expr2->getValue();
            break;
        case LESS:
            value = expr1->getValue() < expr2->getValue();
            break;
        case LESEQ:
            value = expr1->getValue() <= expr2->getValue();
            break;
        case GRA:
            value = expr1->getValue() > expr2->getValue();
            break;
        case GRAEQ:
            value = expr1->getValue() >= expr2->getValue();
            break;
        case EQ:
            value = expr1->getValue() == expr2->getValue();
            break;
        case NEQ:
            value = expr1->getValue() != expr2->getValue();
            break;
    }
    return value;
}

void UnaryExpr::output(int level) {
    std::string op_str;
    switch (op) {
        case NOT:
            op_str = "not";
            break;
        case SUB:
            op_str = "minus";
            break;
    }
    fprintf(yyout, "%*cUnaryExpr\top: %s\n", level, ' ', op_str.c_str());
    expr->output(level + 4);
}

int UnaryExpr::getValue() {
    int value;
    switch (op) {
        case NOT:
            value = !(expr->getValue());
            break;
        case SUB:
            value = -(expr->getValue());
            break;
    }
    return value;
}

void CallExpr::output(int level) {
    std::string name, type;
    int scope;
    name = symbolEntry->toStr();
    type = symbolEntry->getType()->toStr();
    scope = dynamic_cast<IdentifierSymbolEntry*>(symbolEntry)->getScope();
    fprintf(yyout, "%*cCallExpr\tfunction name: %s\tscope: %d\ttype: %s\n",
            level, ' ', name.c_str(), scope, type.c_str());
    Node* temp = param;
    while (temp) {
        temp->output(level + 4);
        temp = temp->getNext();
    }
}

void Constant::output(int level)
{
    std::string type, value;
    type = symbolEntry->getType()->toStr();
    value = symbolEntry->toStr();
    fprintf(yyout, "%*cIntegerLiteral\tvalue: %s\ttype: %s\n", level, ' ',
            value.c_str(), type.c_str());
}


int Constant::getValue() {
    return ((ConstantSymbolEntry*)symbolEntry)->getValue();
}

int Id::getValue() {
    return ((IdentifierSymbolEntry*)symbolEntry)->getValue();
}

void Id::output(int level)
{
    std::string name, type;
    int scope;
    name = symbolEntry->toStr();
    type = symbolEntry->getType()->toStr();
    scope = dynamic_cast<IdentifierSymbolEntry*>(symbolEntry)->getScope();
    fprintf(yyout, "%*cId\tname: %s\tscope: %d\ttype: %s\n", level, ' ',
            name.c_str(), scope, type.c_str());
}

void ITBExpr::output(int level) {
    fprintf(yyout, "%*cITBExpr\ttype: %s to %s\n", level, ' ',expr->getType()->toStr().c_str(), type->toStr().c_str());
    this->expr->output(level + 4);
}

void DeclExpr::output(int level) {
    std::string type;
    if (symbolEntry->getType())
        type = symbolEntry->getType()->toStr();
    fprintf(yyout, "%*cDeclExpr\ttype: %s\n", level, ' ', type.c_str());
    Node* temp = expr;
    while (temp) {
        temp->output(level + 4);
        temp = temp->getNext();
    }
}

void CompoundStmt::output(int level)
{
    fprintf(yyout, "%*cCompoundStmt\n", level, ' ');
    if (stmt)
        stmt->output(level + 4);
}

void SeqNode::output(int level)
{
    stmt1->output(level);
    stmt2->output(level);
}

void DeclStmt::output(int level)
{
    fprintf(yyout, "%*cDeclStmt\n", level, ' ');
    id->output(level + 4);
    if (expr)
        expr->output(level + 4);
    if (this->getNext()) {
        this->getNext()->output(level);
    }
}

void BlankStmt::output(int level) {
    fprintf(yyout, "%*cBlankStmt\n", level, ' ');
}

void IfStmt::output(int level)
{
    fprintf(yyout, "%*cIfStmt\n", level, ' ');
    cond->output(level + 4);
    thenStmt->output(level + 4);
}

void IfElseStmt::output(int level)
{
    fprintf(yyout, "%*cIfElseStmt\n", level, ' ');
    cond->output(level + 4);
    thenStmt->output(level + 4);
    elseStmt->output(level + 4);
}

void ForStmt::output(int level)
{
    fprintf(yyout, "%*cForStmt\n", level, ' ');
    cond1->output(level + 4);
    cond2->output(level + 4);
    cond3->output(level + 4);
    forStmt->output(level + 4);
}

void WhileStmt::output(int level)
{
    fprintf(yyout, "%*cWhileStmt\n", level, ' ');
    cond->output(level + 4);
    whileStmt->output(level + 4);
}

void ReturnStmt::output(int level)
{
    fprintf(yyout, "%*cReturnStmt\n", level, ' ');
    if (retValue != nullptr)
        retValue->output(level + 4);
}


void BreakStmt::output(int level) {
    fprintf(yyout, "%*cBreakStmt\n", level, ' ');
}

void ContinueStmt::output(int level) {
    fprintf(yyout, "%*cContinueStmt\n", level, ' ');
}

void AssignStmt::output(int level)
{
    fprintf(yyout, "%*cAssignStmt\n", level, ' ');
    lval->output(level + 4);
    expr->output(level + 4);
}


void ExprStmt::output(int level) {
    fprintf(yyout, "%*cExprStmt\n", level, ' ');
    expr->output(level + 4);
}

void FunctionDef::output(int level)
{
    std::string name, type;
    name = se->toStr();
    type = se->getType()->toStr();
    fprintf(yyout, "%*cFunctionDefine\tfunction name: %s\ttype: %s\n", level, ' ', 
            name.c_str(), type.c_str());
    if (decl) {
        decl->output(level + 4);
    }
    stmt->output(level + 4);
}
