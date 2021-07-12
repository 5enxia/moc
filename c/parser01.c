// parser01.c
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

//----------------------------------------------------------------------------//
//                               字句解析                                     //
//----------------------------------------------------------------------------//
typedef struct _Token {
    int  type;
    char *str;
} Token;

#define NTOKEN 1000     // 解析可能な最大トークン数
Token token[NTOKEN];    // トークン配列
int nToken;             // トークン数
int tix = -1;           // カレントトークン添え字

void error(const char *format, ...) {
    vfprintf(stderr, format, (char*)&format + sizeof(char*));
    if (tix >= 0) fprintf(stderr, "\n  token[%d].str = \"%s\"\n", tix, token[tix].str);
    exit(1);
}   // 移植性のために、一般には stdarg.h のマクロを使う方がよい。

enum EnumTokenType { ID, NM, ST, SY };
char *TokenType[] = { "ID", "NM", "ST", "SY" };

// 字句解析　返り値：トークン数
int lexer(char *path, int fToken) {
    char buf[256], *p;
    int  n = 0, ch;
    FILE *fin = fopen(path, "r");
    if (fin == NULL) error("入力ファイル<%s>オープンエラー\n", path);
    while ((ch=fgetc(fin)) != EOF) {
        if (ch <= ' ') continue;                  // 空白文字 [\n\t ]
        p = buf;
        if (isalpha(ch)) {                        // 識別子   [A-Za-z][A-Za-z0-9]*
            token[n].type = ID;
            *p++ = ch;
            while (isalnum(ch=fgetc(fin))) *p++ = ch;
            ungetc(ch, fin);                      // 未処理文字をバッファに戻す
        } else if (isdigit(ch)) {                 // 数値     [0-9]+
            token[n].type = NM;
            *p++ = ch;
            while (isdigit(ch=fgetc(fin))) *p++ = ch;
            ungetc(ch, fin);                      // 未処理文字をバッファに戻す
        } else if (ch == '\"') {                  // 文字列
            token[n].type = ST;
            while ((ch=fgetc(fin)) != EOF && ch != '\"') {
                *p++ = ch;
                if (ch=='\\') *p++ = fgetc(fin);
            }
            if (ch != '\"') error("文字列末尾の二重引用符(\")がない\n");
        } else {                                  // 記号
            token[n].type = SY;
            *p++ = ch;
            if (ch == '!' || ch == '=') {       // ２文字演算子の処理
                ch = fgetc(fin);
                if (ch == '=') *p++ = ch;       // "==" または "!="
                else ungetc(ch, fin);
            }
        }
        *p++ = '\0';
        token[n].str = strdup(buf);
        if (fToken) printf("%2d %s: %s\n", n, TokenType[token[n].type], token[n].str);
        n++;
    }
    fclose(fin);
    return n;
}

//----------------------------------------------------------------------------//
//                               名前管理                                     //
//----------------------------------------------------------------------------//
#define  SIZEGLOBAL     1000    // グローバル名前管理表のサイズ
#define  SIZELOCAL      1000    // ローカル名前管理表のサイズ
enum { NM_VAR = 0x01, NM_FUNC = 0x02 };   // 名前の種別
enum { AD_DATA = 0, AD_STACK, AD_CODE };                    // アドレスの種別
enum { GLOBAL = 0, LOCAL };

typedef struct _Name {
    int  type;                  // NM_VAR: 変数、NM_FUNC: 関数
    char *dataType, *name;      // データ型, 変数名または関数名
    int  addrType, address;     // アドレスの種別, 相対アドレス
} Name;
Name GName[SIZEGLOBAL], LName[SIZELOCAL];    // 名前管理表.
int  nGlobal, nLocal;                          // 同上現在の登録数

// 関数または変数情報を名前管理表に登録する
Name *appendName(int nB, int type, char *dataType, char *name, int addrType, int addr) {
    Name nm = { type, dataType, name, addrType, addr };
    Name *pNew = nB == GLOBAL ? &GName[nGlobal++] : &LName[nLocal++];
    if (nGlobal >= SIZEGLOBAL || nLocal >= SIZELOCAL) error("appendName: 配列オーバーフロー");
    memcpy(pNew, &nm, sizeof(Name)); 
    return pNew;        // 新しいエントリのアドレスを返す
}

// 指定された名前表から指定された名前を探す
Name *getNameFromTable(int nB, int type, char *name) {
    int  n, nEntry = nB==GLOBAL ? nGlobal : nLocal;
    for (n = 0; n < nEntry; n++) {
        Name *e = nB==GLOBAL ? &GName[n] : &LName[n];
        if (strcmp(e->name,name) == 0 && (e->type&type) != 0) return e;
    }
    if (nB == 0) error("getNameFromTable: '%s' undeclared", name);
    return NULL;        // 見つからなかった。
} 

// 最初にローカル名前表、なければグローバル名前表から指定された名前を探す
Name *getNameFromAllTable(int type, char *name) {
    Name *pName = getNameFromTable(1, type, name);
    if (pName != NULL) return pName;
    return getNameFromTable(0, type, name);
}

//----------------------------------------------------------------------------//
//                       構文解析～コード生成                                 //
//----------------------------------------------------------------------------//
void Expression(int mode);                      // 式の構文解析
void Statement(int locBreak, int locContinue);  // 文の構文解析

enum { ST_FUNC=0, ST_GVAR };    // ST_FUNC: 関数定義中, ST_GVAR: 変数宣言中
enum { VAL=0, ADDR };           // VAL: 変数の内容, ADDR: 変数のアドレス
enum { ADD, SUB, MUL, DIV, MOD, EQ, NE, GT, LT, INV, NOT,
    RET, STORE, PUSHI, PUSHA, PUSHLA, PUSHGA, LOADA, LOADG, LOADL, ADDSP,
    JUMP, BEQ0, REV, CALL, SYSCALL, ENTRY, LABEL };
char *OPCODE[] = { "add", "sub", "mul", "div", "mod", "eq", "ne", "gt", "lt", "inv", "not",
    "ret", "store", "pushi", "pusha", "pushla", "pushga", "loada", "loadg", "loadl", "addsp",
    "jump", "beq0", "rev", "call", "syscall", "entry", "label" };

typedef struct _INSTRUCT { int opcode, val; } INSTRUCT;
INSTRUCT Inst[1000];    // 命令配列
int  nInst = 0;         // 命令数
int  entryPoint;        // main関数の先頭番地
int  ixLabel = 1;       // ラベルの通し番号(ラベル配列の添え字)
int  status;            // ST_FUNC または ST_GVAR
int  ixData;            // データ(グローバル変数)の登録数
int  baseSpace;         // スタック上の相対アドレス（ローカル変数の相対アドレス）

//==================== 構文解析用共通関数 ======================//
int  is(char *s) { return (tix < nToken && strcmp(token[tix].str,s) == 0); }
int  ispp(char *s) { if (is(s)) { tix++; return 1; } return 0; }
void skip(char *s) { if (!ispp(s)) error("'%s' expected", s); }
int  getLabel() { return ixLabel++; }
int  isTypeSpecifier() { return is("void") || is("int"); }

// 中間語命令コード出力関数
void outInst1(int opcode, int val) {
    Inst[nInst].opcode = opcode;
    Inst[nInst++].val = val;
}
void outInst(int code){ outInst1(code, 0); }

/// TypeSpecifier ::= "void" | "int"
char *TypeSpecifier() {
    if (!isTypeSpecifier()) error("%d: '%s' not typespecfier", tix, token[tix].str);
    return token[tix++].str;        // データ型
}

//==================== 最上位の構文解析 ======================//
/// Program ::= (FunctionDefinition | VariableDeclaration ";")*
void FunctionDefinition();   // 関数定義     後で定義する
void VariableDeclaration();  // 変数宣言     後で定義する

int isFunctionDefinition() { return token[tix+2].str[0] == '('; }
void Program() {
    for (tix = 0; tix < nToken; ) {
        if (isFunctionDefinition()) {   // 関数定義かどうかを調べる
            status = ST_FUNC;
            FunctionDefinition();       // 関数定義
        } else { 
            status = ST_GVAR;
            VariableDeclaration();      // 外部変数宣言
            skip(";");
        }
    }
}

void parser() {
    appendName(GLOBAL, NM_FUNC, "int", "printf", AD_CODE, 0);
    Program();
}

//================================ 変数宣言の構文解析 ================================//
/// VarDeclarator ::= Identifier
char *VarDeclarator() { return token[tix++].str; }      // 変数名、関数名

/// VariableDeclaration ::= TypeSpecifier VarDeclarator ("," VarDeclarator)* 
void VariableDeclaration() {
    char *varType = TypeSpecifier();            // データ型名
    do {
        char *varName = VarDeclarator();        // 変数名
        if (status == ST_FUNC) {
            appendName(LOCAL, NM_VAR, varType, varName, AD_STACK, --baseSpace);
        } else {                // グローバル変数
            appendName(GLOBAL, NM_VAR, varType, varName, AD_DATA, ixData++);
        }
    } while (ispp(","));
}

//================================ 関数定義の構文解析 ================================//
/// FunctionDefinition ::= TypeSpecifier  VarDeclarator 
///     "(" [ VariableDeclaration ("," VariableDeclaration)* ] ")" CompoundStatement
void CompoundStatement(int, int);    // 後で定義する
void FunctionDefinition() {
    INSTRUCT *pSub = &Inst[nInst+1];    // ADDSP命令の位置
    int labelFunc = getLabel(); // この関数の識別番号を得る
    char *varType = TypeSpecifier();        // 返り値のデータ型
    char *varName = VarDeclarator();        // 関数名
    int  nInstSave = nInst;

    if (strcmp(varName,"main") == 0) entryPoint = nInst;
    outInst1(ENTRY, labelFunc);
    nLocal = 0;         // ローカル変数表初期化(メモリ解放省略)
    appendName(GLOBAL, NM_FUNC, varType, varName, AD_CODE, labelFunc);
    baseSpace = 2;      // 現在のspは旧bpを指す, その下は戻り番地
    outInst1(ADDSP, 0); // 0の値は後で書き換え
    for (skip("("); !ispp(")"); ispp(",")) {
        varType = TypeSpecifier();
        varName = VarDeclarator();      // 引数（戻り番地よりもさらに下に置かれている）
        appendName(LOCAL, NM_VAR, varType, varName, AD_STACK, baseSpace++); 
    }
    baseSpace = 0;              // ここからローカル変数領域 (旧bpの上。相対番地は負)
    CompoundStatement(0, 0);    // 関数本体
    pSub->val = baseSpace;      // ここで ADDSP の値を変更(0または負数)
    if (Inst[nInst-1].opcode != RET) outInst(RET);
}

//================================ 文の構文解析 ================================//
/// CompoundStatement ::= "{" (VariableDeclaration ";")* (Statements)* "}"
void CompoundStatement(int locBreak, int locContinue) {
    for (skip("{"); tix < nToken && isTypeSpecifier(); ) {
        VariableDeclaration(ST_FUNC);   // ローカル変数の宣言
        skip(";");
    }
    while (!ispp("}")) Statement(locBreak, locContinue); 
}

/// IfStatement ::= "if" "(" Expression ")" Statement1 [ "else" Statement2 ] 
void IfStatement(int locBreak, int locContinue) {
    int labelElse, labelEnd;
    tix++;           // "if" をスキップする
    skip("(");
    Expression(VAL);    // 実行結果はstackのトップに置かれる
    skip(")");
    outInst1(BEQ0, labelElse=getLabel());    // stackからpopして、0だったら,ラベルElseに分岐する。
    Statement(locBreak, locContinue);        // Statement1
    if (is("else")) outInst1(JUMP, labelEnd=getLabel()); 
    outInst1(LABEL, labelElse);
    if (ispp("else")) {
        Statement(locBreak, locContinue);    // Statement2
        outInst1(LABEL, labelEnd);
    }
}

/// ReturnStatement ::= "return" [Expression] ";"
void ReturnStatement() {
    if (!is(";")) Expression(VAL);
    skip(";");
    outInst(RET);
}

/// Statement ::= CompoundStatement | IfStatement | ForStatement | WhileStatement
///     | ReturnStatement | BreakStatement | ContinueStatement | Expression ";" | ";"
void Statement(int locBreak, int locContinue) {
    if (is(";")) tix++;      // 空文（null statement）
    else if (is("{")) CompoundStatement(locBreak, locContinue);
    else if (is("if")) IfStatement(locBreak, locContinue);
//    else if (is("for")) ForStatement(TRUE);
//    else if (is("while")) WhileStatement();
    else if (is("return")) ReturnStatement();
//    else if (is("break")) BreakStatement(locBreak);
//    else if (is("continue")) ContinueStatement(locContinue);
    else { Expression(VAL); skip(";"); } 
}

//================================ 式の構文解析 ================================//
void FunctionCall() {
    int n, nParam;
    char *name  = token[tix++].str;
    Name *pName = getNameFromTable(GLOBAL, NM_FUNC, name);
    skip("(");
    for (nParam = 0; !ispp(")"); nParam++) {
        Expression(VAL);
        ispp(",");
    }
    outInst1(REV, nParam);   // 引数順序反転
    if (pName->address > 0) outInst1(CALL, pName->address);   // ユーザ定義関数
    else outInst1(SYSCALL, (int)pName->name);                 // ライブラリ関数
    outInst1(ADDSP, nParam);               // スタックポインタ回復
}

/// PrimaryExpression ::= IDENTIFIER | CONSTANT | '(' Expression ')' | FunctionCall
void PrimaryExpression(int mode) {      // 優先順位 #1
    if (token[tix+1].str[0] == '(') {          // FuctionCall
        FunctionCall(); 
    } else if (token[tix].type == ID) { // Identifier
        Name *pName = getNameFromAllTable(NM_VAR, token[tix++].str);
        if (mode == VAL) { 
            if (pName->addrType == AD_STACK) outInst1(LOADL, pName->address);
            else outInst1(LOADG, pName->address);
        } else {
            if (pName->addrType == AD_STACK) outInst1(PUSHLA, pName->address);
            else outInst1(PUSHGA, pName->address);
        }
    } else if (token[tix].type == NM) {
        outInst1(PUSHI, atoi(token[tix++].str));    // 整数
    } else if (token[tix].type == ST) {
        outInst1(PUSHA, (int)token[tix++].str);     // 文字列
    } else if (ispp("(")) {                     // "(" Expr ")"
        Expression(VAL);
        skip(")");
    } else {
        error("PrimExpr: <%s>", token[tix].str);
    }
}

/// UnaryExpression ::= ["-" | "!"] PrimaryExpression 
void UnaryExpression(int mode) {      // 優先順位 #2
    int op = 0;
    if (token[tix].type == SY) { op = token[tix++].str[0];  }
    PrimaryExpression(mode);
    if (op != 0) outInst(op == '-' ? INV : NOT);
}

/// MulExpression ::= UnaryExpression ( ("*" | "/" | "%") UnaryExpression )*
void MulExpression(int mode) {  // #4
    int fMul=0, fDiv=0;
    PrimaryExpression(mode);    // [左辺の値]
    while ((fMul=ispp("*")) || (fDiv=ispp("/")) || ispp("%")) {
        PrimaryExpression(mode); // [右辺の値,左辺の値]
        outInst(fMul ? MUL : fDiv ? DIV : MOD);
    }
}

/// AddExpression ::= MulExpression ( ("+" | "-") MulExpression )*
void AddExpression(int mode) {  // #5
    int fAdd=0;
    MulExpression(mode);    // [左辺の値]
    while ((fAdd=ispp("+")) || ispp("-")) {
        MulExpression(mode); // [右辺の値,左辺の値]
        outInst(fAdd ? ADD : SUB);
    }
}

/// RelationalExpression ::= AddExpression [ (">" | "<") AddExpression ]
void RelationalExpression(int mode) {  // #7
    int fGT=0;
    AddExpression(mode);    // [左辺の値]
    if ((fGT=ispp(">")) || ispp("<")) {
        AddExpression(mode); // [右辺の値,左辺の値]
        outInst(fGT ? GT : LT);
    }
}

/// EqualityExpression ::= RelationalExpression [ ("==" | "!=") RelationalExpression ]
void EqualityExpression(int mode) {  // #8
    int fEq=0;
    RelationalExpression(mode);    // [左辺の値]
    if ((fEq=ispp("==")) || ispp("!=")) {
        RelationalExpression(mode); // [右辺の値,左辺の値]
        outInst(fEq ? EQ : NE);
    }
}

/// AssignExpression ::= IDENTIFIER "=" EqualityExpression
void AssignExpression() {
    PrimaryExpression(ADDR);    // [左辺変数アドレス]
    skip("=");
    EqualityExpression(VAL);    // [右辺の値,左辺変数アドレス]
    outInst(STORE);               // Mem[st1] = st0
}

/// Expression ::= AssignExpression | EqualityExpression
void Expression(int mode) {
    if (token[tix].type==ID && token[tix+1].type==SY && token[tix+1].str[0]=='=') {
        AssignExpression();
    } else {
        EqualityExpression(mode);
    }
}

void printInst(int n) {         // デバッグ用
    printf("%2d: %s", n, OPCODE[Inst[n].opcode]);
    if (Inst[n].opcode < PUSHI) printf("\n");
    else if (Inst[n].opcode==PUSHA || Inst[n].opcode==SYSCALL) printf(" '%s'\n", (char*)Inst[n].val);
    else printf(" %d\n", Inst[n].val);
}

//-----------------------------------------------------------------------------//
//                   　　　　　　メインプログラム                              //
//-----------------------------------------------------------------------------//
void main(int argc, char* argv[]) {
    int  n, fToken=0;
    char *srcfile = NULL;
    for (n = 1; n < argc; n++) {
        if (strcmp(argv[n],"-token") == 0) fToken = 1;
        else srcfile = argv[n];
    }
    nToken = lexer(srcfile, fToken);
    tix = 0;
    parser();
    for (n = 0; n < nInst; n++) {
        printInst(n);
    }
}
