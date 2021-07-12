// mc00.c       mini C コンパイラ
// c:\mcc>tcc src\mc00.c
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lib0.c"

int  fToken, fCTrace, fCode, fTrace, fAST, fJIT;        // コマンドパラメータ・フラグ

typedef struct { int nLine, tix, nInst, nData, nText; } Index; // 配列添え字等
Index ix;

#define  MAXTEXT   1000000
char text[MAXTEXT];     // 文字列および浮動小数点定数格納領域. tix.nText: 登録サイズ

//--------------------------------------------------------------------------------------//
//                                     字句解析                                         //
//--------------------------------------------------------------------------------------//
#define LINESIZE        1024    // 1行の最大サイズ(バイト)
#define NTOKEN          100000  // 解析可能な最大トークン数

typedef struct {
    int     type;       // トークン種別
    char    *str;       // トークン文字列
    int     ival;       // 定数のときの値,文字列アドレスを格納することもある。
    double  dval;       // 定数のときの値
    int     nLine;      // ソースコードの行番号
} Token;
Token token[NTOKEN];            // トークン配列
int   nToken;                   // トークン数

void printToken() { if (ix.tix >= 0) printf("\n %d: %s\n", ix.nLine, token[ix.tix].str); }

char *outText(char *buf) {
    char *dst = &text[ix.nText+4];      // 前の４バイトは将来使う領域。
    ix.nText += strlen(buf) + 5;        // +4 は将来使う領域のサイズ, +1 は ヌル文字('\0')
    return strcpy(dst, buf);
}

// 字句解析（行単位）
void lex(char *p, int nLine) {
    char *OPERATOR2 = "== != <= >= >> << && || += -= *= /= ++ -- [] ";  //末尾スペース必須
    char buf[LINESIZE], *pBgn, op2[4];
    int  type, ival, lenToken;
    double dval;

    while (*p != '\0') {
        if (*p <= ' ') { p++; continue; }       // 空白文字 [\n\t ]
        if (*p == '/' && p[1] == '/') return;   // 行末までコメント
        // ここから字句解析
        pBgn = p;
        ival = 0;
        if (isalpha(*p) || *p=='_') {           // 予約語,識別子 [_A-Za-z][_A-Za-z0-9]*
            for (p++; isalnum(*p) || *p=='_'; ) p++;
            type = ID;
        } else if (isdigit(*p)) {                       // 数値 10進のみ
            dval = strtod(p, &p);                       // 10進
            ival = (int)dval;
            type = CDBL;                                // 整数の場合後で CINT に置換
        } else if (*p == '\"' || *p == '\'') {          // 文字列か文字
            for (p++ ; *p != '\0' && *p != *pBgn; p++) {
                if (*p == '\\' || isJp(p[0]&0xff)) p++;
            }
            if (*p++ != *pBgn) error("文字列末尾の引用符(%c)がない", *pBgn);
            type = *pBgn++ == '"' ? CSTR : CCHR;
        } else {                                        // 記号
            sprintf(op2, "%c%c ", p[0], p[1]);          // ３文字目のスペース必須
            p += strstr(OPERATOR2,op2) != NULL ? 2 : 1;
            type = SY;
        }
        lenToken = p - pBgn;
        if (type==CSTR || type==CCHR) lenToken--;  // 引用符は除外
        memcpy(buf, pBgn, lenToken);
        buf[lenToken] = '\0';
        if (type==CDBL && strchr(buf,'.')==NULL) type = CINT;

        // 構文解析以降の処理を助けるための処理(本来の字句解析処理ではない)
        if (type == CDBL) error("double型未実装");
        else if (type == CSTR) ival = (int)outText(buf);

        if (fToken) printf("%3d[%d]: %s\n", ix.tix, nLine, buf);
        { // 上でセットしたデータを Token構造体配列の要素にコピーする
            Token tk = { type, strdup(buf), ival, dval, nLine }; 
            token[ix.tix++] = tk; 
        }
    }
}

void lexer(char *path) {
    char buf[LINESIZE];
    FILE *fp = fopen(path,"r");
    if (fp == NULL) error("ファイル<%s>オープンエラー", path);
    for (ix.nLine = 1; fgets(buf,LINESIZE,fp) != NULL; ix.nLine++) {
        if (buf[0] != '#') lex(buf, ix.nLine);                // この行の字句解析
    }
    nToken = ix.tix;
    fclose(fp);
}

//--------------------------------------------------------------------------------------//
//                         構文解析～中間語コード生成                                   //
//--------------------------------------------------------------------------------------//
#define  MAXINST   1000000
#define  MAXMEM    1000000
enum { PRINTF=1,  };
enum { DT_VOID=NMSIZE+'v', DT_INT=NMSIZE+'i', DT_STR=NMSIZE+'s', DT_DBL=NMSIZE+'d' };
enum { TOP=0, VAL=1, ADDR=2 };    // VAL: 変数の内容, ADDR: 変数のアドレス
enum { ST_FUNC=0, ST_GVAR, ST_STRUCT };    // ST_FUNC: 関数定義中, ST_GVAR: G変数宣言中

typedef struct { char *rtnType, *funcName; } SYSFUNC;
SYSFUNC SysFunc[] = { {"",""}, {"int","printf"} };

int  insts[MAXINST];    // 命令配列.  tix.nInst: 命令数
char mem[MAXMEM+32];    // メインメモリ. +32はチェックを簡単にするためのマージン
int  ixLabel = 1;       // ラベルの通し番号(ラベル配列の添え字)
int  status;            // ST_FUNC または ST_GVAR（ローカル/グローバル処理の区別）
int  baseSpace;         // スタック上の相対アドレス（ローカル変数の相対アドレス）

// Expressionおよび関数コールでの引数に対しては抽象構文木を作成してからコード生成を行う。
void Statement(int locBreak, int locContinue, int rtnType);     // 文の構文解析
void Expression(int mode, int *pType);                          // 式の構文解析

//==================== 構文解析用共通関数 ======================//
int is_n(char *s, int n) {     // 予約語および演算子のチェック
    if (token[ix.tix+n].type != ID && token[ix.tix+n].type != SY) return 0;
    return ix.tix+n < nToken ? strcmp(token[ix.tix+n].str,s)==0 : 0;
}
int  is(char *s) { return is_n(s,0); }
int  ispp(char *s) { if (is(s)) { ix.tix++; return 1; } return 0; }
void skip(char *s) { if (!ispp(s)) error("'%s' expected", s); }
int  getLabel() { return ixLabel++; }
int  isTypeSpecifier() { return (is("void")||is("int")||is("string")||is("double")); }

// 中間語命令コード出力関数
int F(int type) { return type == DT_DBL ? DBL : 0; }
void outInst(int code) { insts[ix.nInst++] = code; }    // オペランドなし
void outInst1(int code, int val) {                      // オペランド一つ
    insts[ix.nInst++] = code;
    insts[ix.nInst++] = val;
}

/// TypeSpecifier ::= ("void" | "int" | "string" | "double")
char *TypeSpecifier() {
    if (!isTypeSpecifier()) error("%d: '%s' not typespecifier", ix.tix, token[ix.tix].str);
    return token[ix.tix++].str;        // データ型.
}

//==================== 最上位の構文解析 ======================//
/// Program ::= (FunctionDefinition | VariableDeclaration ";" | StructDefinition)*
void FunctionDefinition();   // 関数定義     後で定義する
void VariableDeclaration();  // 変数宣言     後で定義する

void Program() {
    for (ix.tix = 0; ix.tix < nToken; ) {
        if (fCTrace) printf("PG: tix=%d %s\n", ix.tix, token[ix.tix].str);
        if (token[ix.tix].type==ID && is_n("(",2)) {   // 関数定義かどうかを調べる
            status = ST_FUNC;
            FunctionDefinition();       // 関数定義
        } else { 
            error("グローバル変数未実装");
        }
    }
}

void parser() {
    int n;
    for (n = 1; n < sizeof(SysFunc)/sizeof(SYSFUNC); n++) {
        appendName(GLOBAL, NM_FUNC, SysFunc[n].rtnType, SysFunc[n].funcName, -n);
    }
    Program();
}

//================================ 変数宣言の構文解析 ================================//
/// VarDeclarator ::= Identifier ( "[" Constant "]" )*
char *VarDeclarator(int *pSize, int *pPtrs, int unit_size) { 
    char *name = token[ix.tix++].str;
    *pPtrs = 0;
    pSize[0] = unit_size;
    return name;                // 変数名、関数名
}

int varSize(char *varType) { return (*varType=='d' ? 8 : 4); } // double:8byte, int:4byte

/// VariableDeclaration ::= TypeSpecifier VarDeclarator ("," VarDeclarator)* 
void VariableDeclaration() {
    int size[8], ptr2;
    char *varType = TypeSpecifier();               // データ型名
    int unit_size = varSize(varType);
    do {
        Name *pName;
        char *varName = VarDeclarator(size, &ptr2, unit_size);     // 変数名
        if (status == ST_FUNC) {
            baseSpace -= abs(size[0]);
            pName = appendName(LOCAL, NM_VAR, varType, varName, baseSpace);
        } else if (status == ST_GVAR) {
            error("グローバル変数未実装");
        }
        memcpy(pName->size, size, sizeof(size));  // 変数(0)、配列要素数、ポインタ
        pName->ptrs = ptr2;
    } while (ispp(","));
}

//================================ 関数定義の構文解析 ================================//
/// FunctionDefinition ::= TypeSpecifier  VarDeclarator 
///     "(" [ VariableDeclaration ("," VariableDeclaration)* ] ")" CompoundStatement
void CompoundStatement(int, int, int);    // 後で定義する
void FunctionDefinition() {
    int size[8], ptr, rtnType, unit_size;
    Name *pName;
    int  *pSub;                                 // ADDSP命令の位置
    char *varType = TypeSpecifier();            // 返り値のデータ型
    char *varName = token[ix.tix++].str;        // 関数名

    outInst1(FUNCTION, (int)outText(varName));  // 疑似命令
    pName = appendName(GLOBAL, NM_FUNC, varType, varName, ix.nInst);
    outInst(ENTRY);

    nLocal = nGlobal;                   //==== ローカル変数表初期化 ====

    rtnType = pName->dataType;          // varType の ID
    pName->size[0] = 0;                 // 返り値のデータ型付属情報
    pName->ptrs = 0;
    baseSpace = 2*4;                    // 現在のspは旧bpを指す,その下は戻り番地(共に4byte)
    pSub = &insts[ix.nInst+1];          // 後で書き換える場所
    outInst1(ADDSP, 0);                 // 0 の値は後で書き換え. ローカル変数領域確保
    for (skip("("); !ispp(")"); ispp(",")) {
        varType = TypeSpecifier();
        unit_size = varSize(varType);
        varName = VarDeclarator(size, &ptr, unit_size);
        pName = appendName(LOCAL, NM_VAR, varType, varName, baseSpace);
        baseSpace += unit_size;
        memcpy(pName->size,size,sizeof(size));  // 引数のデータ型付属情報
        pName->ptrs = ptr;
    }
    baseSpace = 0;    // ここからローカル変数領域 (旧bpの上。相対番地は負)
    CompoundStatement(0, 0, rtnType);   // 関数本体
    *pSub = baseSpace;                  // ここで ADDSP の値を変更(0または負数)
    if (insts[ix.nInst-1] != RET) outInst(RET); // "return"がないとき補う
}

//================================ 文の構文解析 ================================//
/// CompoundStatement ::= "{" (VariableDeclaration ";" | Statements)* "}"
void CompoundStatement(int locBreak, int locContinue, int rtnType) {
    int nLocalSave = nLocal;  //==== このブロックのローカル名前管理表のはじまり ====
    skip("{");
    while (ix.tix < nToken && !ispp("}")) {
        if (isTypeSpecifier()) { VariableDeclaration(); skip(";"); } // ローカル変数の宣言
        else Statement(locBreak, locContinue, rtnType);
    }
    nLocal = nLocalSave;    //==== 当ブロックで宣言した変数を名前管理表から廃棄する ====
}

/// ExpressionStatement ::= Expression ";"
void ExpressionStatement() {
    int type; 
    Expression(TOP, &type);
    skip(";");
}

/// Statement ::= CompoundStmt | ExpressionStmt | ";"
void Statement(int locBreak, int locContinue, int rtnType) {
    if (ispp(";")) ;      // 空文（null statement）
    else if (is("{")) CompoundStatement(locBreak, locContinue, rtnType);
    else ExpressionStatement();
}

//============================ 式の構文解析～コード生成 ==============================//
Node *ExpressionAST();
void ExpressionCode(Node *pNode, int mode, int *pt);     // 中間語コード生成

Node *FunctionCallAST() {
    Node *pCall = allocNode(FCALL, (int)token[ix.tix++].str, 0, 4); // 最初,引数は4個まで
    for (skip("("); !ispp(")"); ispp(",")) {
        pCall = addSubNode(pCall, ExpressionAST());
    }   // 子ノードポインタ配列拡張により pCallが変わることがある
    return pCall;
}

/// PrimaryExpression ::= IDENTIFIER | CONST | FuncCall
Node *PrimaryExpressionAST() {          // #1
    if (is_n("(",1)) return FunctionCallAST(); 
    if (token[ix.tix].type == CINT) return genNode(CINT, token[ix.tix++].ival, 0);
    if (token[ix.tix].type == CSTR) return genNode(CSTR, token[ix.tix++].ival, 0);
    if (token[ix.tix].type == ID) return genNode(VAR, (int)token[ix.tix++].str, 0);
    error("PrimaryExpressionではない");
    return NULL;
}

/// MulExpression ::= PrimaryExpression ( ("*" | "/" | "%") PrimaryExpression )*
Node *MulExpressionAST() {              // #4
    int fMul=0, fDiv=0;
    Node *pLeft = PrimaryExpressionAST();
    while ((fMul=ispp("*")) || (fDiv=ispp("/")) || ispp("%")) {
        Node *pRight = PrimaryExpressionAST();
        pLeft = genNode(fMul? MUL : fDiv? DIV : MOD, 0, 2, pLeft, pRight);
    }
    return pLeft;
}

/// AddExpression ::= MulExpression ( ("+" | "-") MulExpression )*
Node *AddExpressionAST() {              // #5
    int fAdd;
    Node *pLeft = MulExpressionAST();    
    while ((fAdd=ispp("+")) || ispp("-")) {
        Node *pRight = MulExpressionAST(); // 右辺の値
        pLeft = genNode(fAdd? ADD : SUB, 0, 2, pLeft, pRight);
    }
    return pLeft;
}

/// AssignExpression ::= AddExpression "=" Expression
Node *AssignExpressionAST() {           // #15
    Node *pLeft = AddExpressionAST();
    if (ispp("=")) {
        Node *pRight = ExpressionAST();
        pLeft = genNode(SET, 0, 2, pLeft, pRight);
    }
    return pLeft;
}

/// Expression ::= AssignExpression
void Expression(int mode, int *pt) {
    Node *pTop = ExpressionAST();                       // 構文解析：抽象構文木作成
    if (pTop != NULL && (mode%256) == TOP) { 
        if (fAST) { printAST(pTop); printf("\n"); }
        ExpressionCode(pTop, VAL, pt);                  // 意味解析：中間語コード生成
        freeNodes(pTop);                                // 抽象構文木削除
    }
}

Node *ExpressionAST() { return AssignExpressionAST(); } // 構文解析：抽象構文木作成

//-----------------------------------------------------------------------------//
//                   　　意味解析・中間語コード生成                            //
//-----------------------------------------------------------------------------//
void FunctionCallCode(Node *pNode, int mode, int *pt) {
    int k, type, argsSize = 0;
    Name *pName = searchName(GLOBAL, NM_FUNC, (char*)pNode->val);
    *pt = pName->dataType;
    for (k = pNode->nSub - 1; k >= 0; k--) {
        ExpressionCode(pNode->sub[k], VAL, &type);      // 結果は eax/st に置く
        outInst(PushArg+F(type));
        argsSize += type==DT_DBL ? 8 : 4;
    }
    outInst1(SYSCALL, -pName->address);                  // ライブラリ関数
    if (argsSize > 0) outInst1(ADDSP, argsSize);
}

void VarCode(Node *pNode, int mode, int *pt) {
    Name *pName = searchName(LOCAL, NM_VAR, (char*)pNode->val);
    if (pName != NULL) outInst1(LOADLA, pName->address);   // アドレスを eax レジスタに置く
    else error("グローバル変数未実装");
    *pt = pName->dataType;
    if (mode == VAL) outInst(LOADV+F(*pt));
}

void PrimaryExprCode(Node *pNode, int mode, int *pt) {
    if (pNode->type == FCALL) FunctionCallCode(pNode, mode, pt);
    else if (pNode->type == CINT) { outInst1(LOADI, pNode->val); *pt = DT_INT; } 
    else if (pNode->type == CSTR) { outInst1(LOADS, pNode->val); *pt = DT_STR; }
    else if (pNode->type == VAR) VarCode(pNode, mode, pt);
    else error("PrimExpr: NodeType=%d\n", pNode->type);
}

void BinaryExprCode(Node *pNode, int mode, int *pt) {
    int typeRight, type = pNode->type;
    Node *pLeft  = pNode->sub[0];
    Node *pRight = pNode->sub[1];
    ExpressionCode(pLeft, mode, pt);    // eax: 左辺の値
    outInst(PushAX+F(*pt));
    ExpressionCode(pRight, mode, &typeRight);
    outInst(PopCX+F(*pt));
    outInst(type+F(*pt));
}

void AssignExprCode(Node *pNode, int mode, int *pt) {
    int typeRight;
    ExpressionCode(pNode->sub[0], ADDR, pt);            // 左辺
    outInst(PushAX);    // 左辺（アドレス）
    ExpressionCode(pNode->sub[1], mode, &typeRight);    // 右辺
    outInst(PopCX);
    outInst(pNode->type + F(*pt));
}

void ExpressionCode(Node *pNode, int mode, int *pt) {
    int type = pNode->type;
    if (fCTrace) printf("ExprCode: NodeType=%d %s\n", type, type==VAR ? (char*)pNode->val : "");
    if (ADD <= type && type <= SHR) BinaryExprCode(pNode, mode, pt);
    else if (SET <= type && type <= DIVSET) AssignExprCode(pNode, mode, pt);
    else if (CINT <= type && type < LAND) PrimaryExprCode(pNode, mode, pt);
    else error("ExpressionCode: NodeType=%d\n", type);
}

//--------------------------------------------------------------------------------------//
//                                　　 仮想マシン                                       //
//--------------------------------------------------------------------------------------//
int    sp = MAXMEM, bp, pc;     // スタックポインタ、ベースポインタ、プログラムカウンタ
int    eax, ecx;                // 演算レジスタ
int    *mem4 = (int*)mem;       // double *mem8 = (double*)mem とはできない
void   pushi(int ival)    { sp -= 4; mem4[sp/4] = ival; }
int    popi() { sp += 4; return mem4[sp/4-1]; }

void printInst(int n) {         // デバッグ用
    int code = insts[n] % 128;
    int fDbl = insts[n] / 128;
    int ival = code >= PUSHI ? insts[n+1] : 0;
    printf("%2d: %s%s ", n, fDbl ? "f" : "", OPCODE[code]);
    if (code == PUSHA || code == FUNCTION) printf("\"%s\"\n", (char*)ival);
    else if (code == SYSCALL) printf("\"%s\"\n", SysFunc[ival].funcName);
    else if (code == LOADS) printf("\"%s\"\n", (char*)ival);
    else if (code < PUSHI) printf("\n");
    else printf("%d\n", ival);
}

int getEntryPoint(char *funcName) {     // 最初に実行する関数を探す。その入口が返される。
    int n;
    for (n = 0; n < ix.nInst; ) {
        int code = insts[n++] % 128;
        int ival = (code >= PUSHI) ? insts[n++] : 0;
        if (code == FUNCTION && strcmp(funcName,(char*)ival) == 0) {
            return n; // ENTRY命令の位置. 最初に実行する関数の先頭アドレス
        }
    }
    return -1;  // 関数 funcName はない
}

int exec(int entryPoint) {      // entryPoint: 最初に実行する関数のアドレス
    if (entryPoint < 0) return 0;
    pushi(-1);                  // 戻り番地が-1のとき実行終了
    pc = entryPoint;
    while (pc >= 0) {
        int code = insts[pc++];
        int ival = (code%128 >= PUSHI) ? insts[pc++] : 0;
        if (fTrace) printInst(pc);
        if (code >= 128) error("実数演算未実装");
        switch (code) {
            case ADD: eax += ecx; break;
            case SUB: eax = ecx - eax; break;
            case MUL: eax *= ecx; break;
            case DIV: eax = ecx / eax; break;
            case PushAX: case PushArg: pushi(eax); break;
            case PopCX: ecx = popi(); break;
            case SET:   mem4[ecx/4] = eax; break;
            case LOADLA: eax = bp+ival; break;      // ローカル変数のアドレス
            case LOADI: eax = ival; break;          // immidiate
            case LOADS: eax = ival; break;          // 文字列アドレス
            case LOADV: eax = mem4[eax/4]; break;   // アドレスから値に変更
            case ENTRY: pushi(bp); bp = sp; break;  // 関数入口 bp+2 が引数の先頭アドレス
            case ADDSP: case WADDSP: sp += ival; break;
            case RET: sp = bp; bp = popi(); pc = popi(); break;
            case SYSCALL: eax = vprintf((char*)mem4[sp/4], (void*)&mem4[sp/4+1]); break;
            default: error("%d: 未実装整数演算opcode=%s\n", pc-1, OPCODE[code]); break;
        }
    }
    return 1;
}

int VM(int argc, char *argv[]) {  // 中間語コード実行
    int n, cd, entryPoint;
    for (n = 0; n < ix.nInst; n += (cd >= PUSHI) ? 2 : 1) {
        cd = insts[n]%128;
        if (fCode) printInst(n);
    }
    entryPoint = getEntryPoint("main"); 
    if (entryPoint < 0) error("メイン関数がありません");
    return exec(entryPoint);       // コンソールアプリとして実行
}

//--------------------------------------------------------------------------------------//
//                                メインプログラム                                      //
//--------------------------------------------------------------------------------------//
// オプションはsrcfileより前に置く。srcfileより後ろがユーザプログラムのパラメータ
int main(int argc, char* argv[]) {
    int  n;
    char *srcfile = NULL;
    for (n = 1; n < argc; n++) {
        if (strcmp(argv[n],"-token")  == 0)      fToken = 1;
        else if (strcmp(argv[n],"-ctrace") == 0) fCTrace= 1;   // コンパイルトレース
        else if (strcmp(argv[n],"-ast")    == 0) fAST   = 1;   // 抽象構文木表示
        else if (strcmp(argv[n],"-code")   == 0) fCode  = 1;   // 中間語コード表示
        else if (strcmp(argv[n],"-trace")  == 0) fTrace = 1;   // 実行時トレース
        else { srcfile = argv[n]; break; }
    }
    lexer(srcfile);
    parser();
    ix.tix = -1;        // エラーメッセージで行番号の表示を抑止する。
    return VM(argc-n, &argv[n]);
}
