%option noyywrap
%option yylineno
%{
    /*
    * You will need to comment this line in lab5.
    */
    //#define ONLY_FOR_LEX
    
    #ifdef ONLY_FOR_LEX
    
    #include <math.h>
    #include <sstream>
    int cnt=0;

    #else
    #include "parser.h"
    #endif

    #define YY_NO_UNPUT
    #define YY_NO_INPUT
    #include <string>

    #ifdef ONLY_FOR_LEX
    #include <ostream>
    #include <fstream>
    using namespace std;
    extern FILE *yyin; 
    extern FILE *yyout;

    struct SymbolTable{
        int id=0;//作用域编号
        int num=0;
        string s[100];
    }st[100];
    bool state;
    int stnum;//结构体复用后的实际序号
    int stcnt;//为每一个作用域编号

    void DEBUG_FOR_LAB4(std::string s){
        if(s[0]=='I' && s[1]=='D'){
            if(state){
                std::string DEBUG_INFO = s +to_string(yylineno)+"\t" + to_string(cnt) + "\t"+to_string(stcnt)+"\n";
                fputs(DEBUG_INFO.c_str(), yyout);
            }else{
                std::string DEBUG_INFO = s +to_string(yylineno)+"\t" + to_string(cnt) + "\t"+to_string(st[stnum].id)+"\n";
                fputs(DEBUG_INFO.c_str(), yyout);
            }
        }else{
            std::string DEBUG_INFO = s +to_string(yylineno)+"\t" + to_string(cnt) +"\n";
            fputs(DEBUG_INFO.c_str(), yyout);
        }
        
    }
    void addChar(int leng){
        cnt+=leng;
    }
    #endif
%}

DIGIT [0-9]
DECI ([1-9][0-9]*|0)
OCT (0[0-7]+)
HEX (0[Xx][[0-9a-fA-F]+)
FNUM  {DECI}*(\.{DIGIT}+)?((e|E)[+\-]?{DECI}+)?
ID [[:alpha:]_][[:alpha:][:digit:]_]*
EOL (\r\n|\n)
WHITE [\t ]

LPAREN "("
RPAREN ")"
LBRACK "["
RBRACK "]"
LBRACE "{"
RBRACE "}"
SEMI ";"
COMMA ","

EQ "=="
GRAEQ ">="
LESEQ "<="
NEQ "!="
ASSIGN "="
PLUSASSIGN "+="
MINUSASSIGN "-="
MULASSIGN "*="
DIVASSIGN "/="
GRA ">"
LESS "<"
ADD "+"
SUB "-"
MUL "*"
DIV "/"
INCRE "++"
DECRE "--"
MOD "%"
AND "&&"
OR "||"
NOT "!"

CONST "const"
VOID "void"
IF "if"
ELSE "else"
DO "do"
WHILE "while"
FOR "for"
BREAK "break"
CONTINUE "continue"
RETURN "return"
INT "int"
FLOAT "float"


commentbegin "/*"
commentelement .|\n
commentend "*/"
%x COMMENT
commentlinebegin "//"
commentlineelement .
commentlineend \n
%x COMMENTLINE


%%

"int" {
    /*
    * Questions: 
    *   Q1: Why we need to return INT in further labs?
    *   Q2: What is "INT" actually?
    */
    #ifdef ONLY_FOR_LEX
        DEBUG_FOR_LAB4("INT\t\tint\t\t");
        addChar(yyleng);
    #else
        return INT;
    #endif
}
"float" {
    #ifdef ONLY_FOR_LEX
        DEBUG_FOR_LAB4("FLOAT\t\tfloat\t\t");
        addChar(yyleng);
    #else
        return FLOAT;
    #endif
}
"void" {
    #ifdef ONLY_FOR_LEX
        DEBUG_FOR_LAB4("VOID\t\tvoid\t\t");
        addChar(yyleng);
    #else
        return VOID;
    #endif 
}
"const" {
    #ifdef ONLY_FOR_LEX
        DEBUG_FOR_LAB4("CONST\t\tconst\t\t");
        addChar(yyleng);
    #else
        return CONST;
    #endif 
}
"if" {
    #ifdef ONLY_FOR_LEX
        DEBUG_FOR_LAB4("IF\t\tif\t\t");
        addChar(yyleng);
    #else
        return IF;
    #endif
};
"else" {
    #ifdef ONLY_FOR_LEX
        DEBUG_FOR_LAB4("ELSE\t\telse\t\t");
        addChar(yyleng);
    #else
        return ELSE;
    #endif
};
"do" {
    #ifdef ONLY_FOR_LEX
        DEBUG_FOR_LAB4("DO\t\tdo\t\t");
        addChar(yyleng);
    #else
        return DO;
    #endif
};
"while" {
    #ifdef ONLY_FOR_LEX
        DEBUG_FOR_LAB4("WHILE\t\twhile\t\t");
        addChar(yyleng);
    #else
        return WHILE;
    #endif
};
"for" {
    #ifdef ONLY_FOR_LEX
        DEBUG_FOR_LAB4("FOR\t\tfor\t\t");
        addChar(yyleng);
    #else
        return FOR;
    #endif
};
"break" {
    #ifdef ONLY_FOR_LEX
        DEBUG_FOR_LAB4("BREAK\t\tbreak\t\t");
        addChar(yyleng);
    #else
        return BREAK;
    #endif 
}
"continue" {
    #ifdef ONLY_FOR_LEX
        DEBUG_FOR_LAB4("CONTINUE\tcontinue\t");
        addChar(yyleng);
    #else
        return CONTINUE;
    #endif 
}
"return" {
    #ifdef ONLY_FOR_LEX
        DEBUG_FOR_LAB4("RETURN\t\treturn\t\t");
        addChar(yyleng);
    #else
        return RETURN;
    #endif
}

"==" {
    #ifdef ONLY_FOR_LEX
        DEBUG_FOR_LAB4("EQ\t\t==\t\t");
        addChar(yyleng);
    #else
        return EQ;
    #endif
}
">=" {
    #ifdef ONLY_FOR_LEX
        DEBUG_FOR_LAB4("GRAEQ\t\t>=\t\t");
        addChar(yyleng);
    #else
        return GRAEQ;
    #endif
}
"<=" {
    #ifdef ONLY_FOR_LEX
        DEBUG_FOR_LAB4("LESEQ\t\t<=\t\t");
        addChar(yyleng);
    #else
        return LESEQ;
    #endif
}
"!=" {
    #ifdef ONLY_FOR_LEX
        DEBUG_FOR_LAB4("NEQ\t\t!=\t\t");
        addChar(yyleng);
    #else
        return NEQ;
    #endif
}
"+=" {
    #ifdef ONLY_FOR_LEX
        DEBUG_FOR_LAB4("PLUSASSIGN\t+=\t\t");
        addChar(yyleng);
    #else
        return PLUSASSIGN;
    #endif
}
"-=" {
    #ifdef ONLY_FOR_LEX
        DEBUG_FOR_LAB4("MINUSASSIGN\t-=\t\t");
        addChar(yyleng);
    #else
        return MINUSASSIGN;
    #endif
}
"*=" {
    #ifdef ONLY_FOR_LEX
        DEBUG_FOR_LAB4("MULASSIGN\t*=\t\t");
        addChar(yyleng);
    #else
        return MULASSIGN;
    #endif
}
"/=" {
    #ifdef ONLY_FOR_LEX
        DEBUG_FOR_LAB4("DIVASSIGN\t/=\t\t");
        addChar(yyleng);
    #else
        return DIVASSIGN;
    #endif
}

"=" {
    #ifdef ONLY_FOR_LEX
        DEBUG_FOR_LAB4("ASSIGN\t\t=\t\t");
        addChar(yyleng);
    #else
        return ASSIGN;
    #endif
}
"<" {
    #ifdef ONLY_FOR_LEX
        DEBUG_FOR_LAB4("LESS\t\t<\t\t");
        addChar(yyleng);
    #else
        return LESS;
    #endif
}
">" {
    #ifdef ONLY_FOR_LEX
        DEBUG_FOR_LAB4("GRA\t\t>\t\t");
        addChar(yyleng);
    #else
        return GRA;
    #endif
}
"+" {
    #ifdef ONLY_FOR_LEX
        DEBUG_FOR_LAB4("ADD\t\t+\t\t");
        addChar(yyleng);
    #else
        return ADD;
    #endif
}
"-" {
    #ifdef ONLY_FOR_LEX
        DEBUG_FOR_LAB4("SUB\t\t-\t\t");
        addChar(yyleng);
    #else
        return SUB;
    #endif
}
"*" {
    #ifdef ONLY_FOR_LEX
        DEBUG_FOR_LAB4("MUL\t\t*\t\t");
        addChar(yyleng);
    #else
        return MUL;
    #endif
}
"/" {
    #ifdef ONLY_FOR_LEX
        DEBUG_FOR_LAB4("DIV\t\t/\t\t");
        addChar(yyleng);
    #else
        return DIV;
    #endif
}
"++" {
    #ifdef ONLY_FOR_LEX
        DEBUG_FOR_LAB4("INCRE\t\t++\t\t");
        addChar(yyleng);
    #else
        return INCRE;
    #endif
}
"--" {
    #ifdef ONLY_FOR_LEX
        DEBUG_FOR_LAB4("DECRE\t\t--\t\t");
        addChar(yyleng);
    #else
        return DECRE;
    #endif
}
"%" {
    #ifdef ONLY_FOR_LEX
        DEBUG_FOR_LAB4("MOD\t\t%\t\t");
        addChar(yyleng);
    #else
        return MOD;
    #endif
}
"&&" {
    #ifdef ONLY_FOR_LEX
        DEBUG_FOR_LAB4("AND\t\t&&\t\t");
        addChar(yyleng);
    #else
        return AND;
    #endif
}
"||" {
    #ifdef ONLY_FOR_LEX
        DEBUG_FOR_LAB4("OR\t\t||\t\t");
        addChar(yyleng);
    #else
        return OR;
    #endif
}
"!" {
    #ifdef ONLY_FOR_LEX
        DEBUG_FOR_LAB4("NOT\t\t!\t\t");
        addChar(yyleng);
    #else
        return NOT;
    #endif
}

";" {
    #ifdef ONLY_FOR_LEX
        DEBUG_FOR_LAB4("SEMI\t\t;\t\t");
        addChar(yyleng);
    #else
        return SEMI;
    #endif
}
"(" {
    #ifdef ONLY_FOR_LEX
        DEBUG_FOR_LAB4("LPAREN\t\t(\t\t");
        addChar(yyleng);
    #else
        return LPAREN;
    #endif
}
")" {
    #ifdef ONLY_FOR_LEX
        DEBUG_FOR_LAB4("RPAREN\t\t)\t\t");
        addChar(yyleng);
    #else
    return RPAREN;
    #endif
}
"{" {
    #ifdef ONLY_FOR_LEX
        if(stnum+1<100)stnum++;
        else stnum=0;
        stcnt++;
        st[stnum].id=stcnt;
        state=true;
        DEBUG_FOR_LAB4("LBRACE\t\t{\t\t");
        addChar(yyleng);
    #else
        return LBRACE;
    #endif
}
"}" {
    #ifdef ONLY_FOR_LEX
        st[stnum].num=0;
        stnum--;
        state=false;
        DEBUG_FOR_LAB4("RBRACE\t\t}\t\t");
        addChar(yyleng);
    #else
        return RBRACE;
    #endif
}
"[" {
    #ifdef ONLY_FOR_LEX
        DEBUG_FOR_LAB4("LBRACK\t\t[\t\t");
        addChar(yyleng);
    #else
        return LBRACK;
    #endif
}
"]" {
    #ifdef ONLY_FOR_LEX
        DEBUG_FOR_LAB4("RBRACK\t\t]\t\t");
        addChar(yyleng);
    #else
        return RBRACK;
    #endif
}
"," {
    #ifdef ONLY_FOR_LEX
        DEBUG_FOR_LAB4("COMMA\t\t,\t\t");
        addChar(yyleng);
    #else
        return COMMA;
    #endif
}

{DECI} {
    #ifdef ONLY_FOR_LEX
        string str=yytext,str1="DECI\t\t";
        str1+=str;
        if(str.length()%8==0)str1+="\t";
        else str1+="\t\t";
        DEBUG_FOR_LAB4(str1);
        addChar(yyleng);
    #else
        yylval.itype = atoi(yytext);
        return DECI;
    #endif
}
{OCT} {
    #ifdef ONLY_FOR_LEX
        string str=yytext,str1="OCT\t\t",str2;
        int dec;
        str=str.substr(1);
        stringstream ss;
        ss<<oct<<str;    
        ss>>dec;
        str2=to_string(dec);
        str1+=str2;
        if(str2.length()%8==0)str1+="\t";
        else str1+="\t\t";
        DEBUG_FOR_LAB4(str1);
        addChar(yyleng);
    #else
        int temp;
        sscanf(yytext, "%o", &temp);
        yylval.itype = temp;
        return OCT;
    #endif
}
{HEX} {
    #ifdef ONLY_FOR_LEX
        string str=yytext,str1="HEX\t\t",str2;
        int dec;
        str=str.substr(2);
        stringstream ss;
        ss<<hex<<str;    
        ss>>dec;
        str2=to_string(dec);
        str1+=str2;
        if(str2.length()%8==0)str1+="\t";
        else str1+="\t\t";
        DEBUG_FOR_LAB4(str1);
        addChar(yyleng);
    #else
        int temp;
        sscanf(yytext, "%x", &temp);
        yylval.itype = temp;
        return HEX;
    #endif
}
{FNUM} {
    #ifdef ONLY_FOR_LEX
        string str=yytext,str1="FNUM\t\t";
        str1+=str;
        if(str.length()%8==0)str1+="\t";
        else str1+="\t\t";
        DEBUG_FOR_LAB4(str1);
        addChar(yyleng);
    #else
        return FNUM;
    #endif
}

"putint" {
    char *lexeme;
    lexeme = new char[strlen(yytext) + 1];
    strcpy(lexeme, yytext);
    yylval.strtype = lexeme;
    std::vector<Type*> vec;
    vec.push_back(TypeSystem::intType);
    Type* funcType = new FunctionType(TypeSystem::voidType, vec);
    SymbolTable* st = identifiers;
    while(st->getPrev())
        st = st->getPrev();
    SymbolEntry* se = new IdentifierSymbolEntry(funcType, yytext, st->getLevel());
    st->install(yytext, se);
    return ID;
}

"getint" {
    char *lexeme;
    lexeme = new char[strlen(yytext) + 1];
    strcpy(lexeme, yytext);
    yylval.strtype = lexeme;
    std::vector<Type*> vec;
    Type* funcType = new FunctionType(TypeSystem::intType, vec);
    SymbolTable* st = identifiers;
    while(st->getPrev())
        st = st->getPrev();
    SymbolEntry* se = new IdentifierSymbolEntry(funcType, yytext, st->getLevel());
    st->install(yytext, se);
    return ID;
}
"putch" {
    char *lexeme;
    lexeme = new char[strlen(yytext) + 1];
    strcpy(lexeme, yytext);
    yylval.strtype = lexeme;
    std::vector<Type*> vec;
    vec.push_back(TypeSystem::intType);
    Type* funcType = new FunctionType(TypeSystem::voidType, vec);
    SymbolTable* st = identifiers;
    while(st->getPrev())
        st = st->getPrev();
    SymbolEntry* se = new IdentifierSymbolEntry(funcType, yytext, st->getLevel());
    st->install(yytext, se);
    return ID;
}
"getch" {
    char *lexeme;
    lexeme = new char[strlen(yytext) + 1];
    strcpy(lexeme, yytext);
    yylval.strtype = lexeme;
    std::vector<Type*> vec;
    Type* funcType = new FunctionType(TypeSystem::intType, vec);
    SymbolTable* st = identifiers;
    while(st->getPrev())
        st = st->getPrev();
    SymbolEntry* se = new IdentifierSymbolEntry(funcType, yytext, st->getLevel());
    st->install(yytext, se);
    return ID;
}

"starttime" {
    char *lexeme;
    lexeme = new char[strlen(yytext) + 1];
    strcpy(lexeme, yytext);
    yylval.strtype = lexeme;
    std::vector<Type*> vec;
    Type* funcType = new FunctionType(TypeSystem::voidType, vec);
    SymbolTable* st = identifiers;
    while(st->getPrev())
        st = st->getPrev();
    SymbolEntry* se = new IdentifierSymbolEntry(funcType, yytext, st->getLevel());
    st->install(yytext, se);
    return ID;
}
"stoptime" {
    char *lexeme;
    lexeme = new char[strlen(yytext) + 1];
    strcpy(lexeme, yytext);
    yylval.strtype = lexeme;
    std::vector<Type*> vec;
    Type* funcType = new FunctionType(TypeSystem::voidType, vec);
    SymbolTable* st = identifiers;
    while(st->getPrev())
        st = st->getPrev();
    SymbolEntry* se = new IdentifierSymbolEntry(funcType, yytext, st->getLevel());
    st->install(yytext, se);
    return ID;
}

{ID} {
    #ifdef ONLY_FOR_LEX
        string str=yytext,str1="ID\t\t";
        str1+=str;
        if(str.length()%8==0)str1+="\t";
        else str1+="\t\t";
        
        bool isIn=false;
        for(int i=0;i<=st[stnum].num;i++){
            if(st[stnum].s[i].compare(str)==0){
                isIn=true;
                break;
            }
        }
        if(!isIn){
            st[stnum].s[st[stnum].num]=str;
            st[stnum].num++;
        }
        DEBUG_FOR_LAB4(str1);
        addChar(yyleng);
    #else
        char *lexeme;
        lexeme = new char[strlen(yytext) + 1];
        strcpy(lexeme, yytext);
        yylval.strtype = lexeme;
        return ID;
    #endif
}
{EOL} {
    #ifdef ONLY_FOR_LEX
    cnt=0;
    #endif
}
{WHITE} {
    #ifdef ONLY_FOR_LEX
    addChar(1);
    #endif
}

{commentbegin} {BEGIN COMMENT;}
<COMMENT>{commentelement} {}
<COMMENT>{commentend} {BEGIN INITIAL;}
{commentlinebegin} {BEGIN COMMENTLINE;}
<COMMENTLINE>{commentlineelement} {}
<COMMENTLINE>{commentlineend} {BEGIN INITIAL;}

%%


#ifdef ONLY_FOR_LEX
int main(int argc, char **argv){
    if(argc != 5){
        fprintf(stderr, "Argument Not Enough");
        exit(EXIT_FAILURE);
    }

    if(!(yyin = fopen(argv[1], "r"))){
        fprintf(stderr, "No such file or directory: %s", argv[1]);
        exit(EXIT_FAILURE);
    }

    if(!(yyout = fopen(argv[3], "w"))){
        fprintf(stderr, "No such file or directory: %s", argv[3]);
        exit(EXIT_FAILURE);
    }
    fputs("token\t\tlexeme\t\tlineno\toffset\tpointer_to_scope\n",yyout);
    yylex();
    return 0;
}
#endif
