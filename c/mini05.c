// mini05.c　　レジスタマシン
#include <stdio.h>
#include <stdlib.h>     // atof関数を使用するために必要
#include <string.h>
#include <ctype.h>

#define TRUE   1
#define FALSE  0

void error(const char *format, ...) {
    vfprintf(stderr, format, (char*)&format + sizeof(char*));
    exit(1);
}

//--------------------------------------------------------------------------------------//
//                                     字句解析                                         //
//--------------------------------------------------------------------------------------//
char   *token[1000];
int     nToken;
int     tix;

int isJp(int c){ return (c>=0x81 && c<=0x9F) || (c>=0xE0 && c<=0xFC); } // 2byte文字の判定

int lexer(char *path, int fToken) { // fToken が 1 のとき、トークンを表示する
    char linebuffer[1024];     // 行読み込みバッファ
    char buf[256];      // トークン文字列最大長 255 バイト（最後に終端文字 '\0'）
    char *p, *pBgn; 
    int  n = 0;         // トークン配列の添え字
  
    FILE *fin = fopen(path, "r");
    if (fin == NULL) error("入力ファイル<%s>オープンエラー\n", path);
    while ((p=fgets(linebuffer,sizeof buf,fin)) != NULL) {
        while (*p != '\0') {
            if (*p <= ' ') { p++; continue; }   // 空白文字 [\n\t ]
            pBgn = p;                           // トークンの始まり
            if (isalpha(*p) || *p=='_') {       // 予約語,識別子 [_A-Za-z][_A-Za-z0-9]*
                for (p++; isalnum(*p) || *p=='_'; ) p++;
            } else if (isdigit(*p)) {           // 数値
                strtod(p, &p);     // 数値(10進)
            } else if (*p == '\"') {            // 文字列
                for (p++ ; *p != '\0' && *p != *pBgn; p++) {
                    if (*p == '\\' || isJp(p[0]&0xff)) p++;
                }
                if (*p++ != *pBgn) error("文字列末尾の引用符(%c)がない", *pBgn);
            } else {
                p++;
            }
            sprintf(buf, "%.*s", p - pBgn - (*pBgn=='"' ? 1 : 0), pBgn);
            if (fToken) printf("%03d: %s\n", n, buf);
            token[n++] = strdup(buf);
        }
    }
    fclose(fin);
    return n;   // トークン数を返す 
}

//----------------------------------------------------------------------------//
//                               名前管理                                     //
//----------------------------------------------------------------------------//
typedef struct {
    int  dataType;              // データ型
    char *name;                 // 変数名または関数名
} Name;

Name    name[1000];             // 名前管理表
int     nName;                  // 登録数

// 名前登録関数
int appendName(int type, char *s) {
    name[nName].dataType = type;
    name[nName++].name = s;
    return nName-1;
}

// 名前探索関数
int searchName(char *s) {
    int n = 0;
    while (n < nName && strcmp(name[n].name,s) != 0) n++;  
    if (n == nName) error("変数'%s'は宣言されていません\n", s);
    return n;
}

//----------------------------------------------------------------------------//
//                               コード生成                                   //
//----------------------------------------------------------------------------//
int  insts[10000];      // 中間語コード配列
int  nInst;             // 登録数
char text[1000];        // 文字列定数、浮動小数点定数を格納する
int  nText;             // 格納サイズ

enum { 
    // オペランドなし
    ADD=1, SUB, MUL, DIV, PushEAX, PopECX, PushFAX, PopFCX,
    // オペランドあり
    PUSHI, LOAD, STORE, REV, LOADI, LOADS, CALL, BEQ0, JUMP, LABEL };
char *OPCODE[] = { 
        "", "add", "sub", "mul", "div", "push_eax", "pop_ecx", "push_fax", "pop_fcx",
        "pushi", "load", "store", "rev", "loadi", "loads", "call",
        "beq0", "jump", "label" };

enum { DT_DBL = 0, DT_INT, DT_STR, DT_ERR };    // データ型

// 中間語命令出力関数
void outInst(int code) {                                // オペランドなし
    insts[nInst++] = code;
}
void outInst1(int code, int val) {                      // オペランド一つ
    insts[nInst++] = code;
    insts[nInst++] = val;
}


//----------------------------------------------------------------------------//
//                       構文解析～コード生成                                 //
//----------------------------------------------------------------------------//
void Expression(int *pType);                    // 式の構文解析
void Statement(int locBreak, int locContinue);  // 文の構文解析

int is(char *s) { return tix < nToken && strcmp(token[tix],s) == 0; }

void skip(char *s) {    
    // 現在のトークンが s に一致すれば tix++ 、そうでなければエラー終了
    if (tix < nToken && strcmp(token[tix++],s) != 0) 
        error("'%s' expected. tix=%d", s, tix); 
}

//================================ 変数宣言の構文解析 ================================//
int  isTypeSpecifier() { return (is("int") || is("double") || is("string")); }

int TypeSpecifier() {
    int type = is("int")? DT_INT : is("double")? DT_DBL : is("string")? DT_STR : DT_ERR;
    tix++;
    return type;
}

// VariableDeclaration ::= Typespecifier Id ( "," Id )* ";"
void VariableDeclaration() {
    int type = TypeSpecifier();
    appendName(type, token[tix++]);
    while (strcmp(token[tix], ",") == 0) {
        skip(",");
        appendName(type, token[tix++]);
    }
    skip(";");
}

//=================================== 式の構文解析 ==================================//
// Prim ::= 数値 | 文字列 | Id | "(" Expression ")"
void Prim(int *pt) {                 // 優先順位 #1
    char *tkn = token[tix++];   // トークン
    if (isdigit(*tkn)) {  
        *((double*)&text[nText]) = strtod(tkn,NULL);  // text[nText]～text[nText+7] に実数を格納
        outInst1(LOADI, (int)&text[nText]); // eax: 浮動小数点の格納アドレス
        nText += 8;                     // 8バイト格納した
        *pt = DT_DBL;
    } else if (*tkn == '"') {           // 識別子との区別のため " を残している
        outInst1(LOADS, (int)(tkn+1));  // eax: 文字列アドレス. 先頭の " を除く
        *pt = DT_STR;
    } else if (isalpha(*tkn)) {  // 変数
        int n = searchName(tkn);        // n: 変数番号
        outInst1(LOAD, n);              // fax <- mem[n] 
        *pt = name[n].dataType;
    } else if (*tkn == '(') {
        Expression(pt);      // ( と ) の中の式を計算する. 
        skip(")");
    } else {
        error("'%s'は Prim ではありません\n", tkn);
    }
}

// Mul  ::= Prim ( ( '*' | '/' ) Prim )*
void Mul(int *pt) {                                  // 優先順位 #2
    Prim(pt);  // fax:２項演算の左辺
    while (*token[tix] == '*' || *token[tix] == '/') {
        int type2;
        int op = *token[tix++];         // '*' または '/'
        outInst(PushFAX);
        Prim(&type2);  // fax:２項演算の右辺
        outInst(PopFCX);
        outInst(op=='*' ? MUL : DIV);
    }
}

// Add  ::= Mul  ( ( '+' | '-' ) Mul  )* 
void Add(int *pt) {                                  // 優先順位 #3
    Mul(pt);                  // ２項演算の左辺
    while (*token[tix] == '+' || *token[tix] == '-') {
        int type2;
        int op = *token[tix++];         // '+' または '-'
        outInst(PushFAX);
        Mul(&type2);              // ２項演算の右辺
        outInst(PopFCX);
        outInst(op=='+' ? ADD : SUB);
    }
}

// Assign ::= Id "=" Add
void Assign() {                                         // 優先順位 #4
    int n = searchName(token[tix++]);
    int typeLeft = name[n].dataType;    // 左辺のデータ型
    int typeRight;                      // 右辺のデータ型
    skip("=");
    Add(&typeRight);    // fax: 結果
    outInst1(STORE, n); // mem[n] = fax;
}

// printf文のみサポート。第１引数は書式文字列、第２引数以降は double型数値のみとする。
void FuncCall() {
    int n, type;

    skip("printf");     // 現在は printf関数しかサポートしていない
    skip("(");
    Expression(&type);
    outInst(PushEAX);   // 書式文字列をスタックに push
    outInst(PushEAX);   // 8バイト幅にするため、２回push
    for (n = 1; strcmp(token[tix],")") != 0; n++) {
        skip(",");
        Expression(&type);
        outInst(PushFAX);          // 引数が左から順にスタックに積まれる
    }   // forの終了段階では最後の引数が stack のトップに置かれる
    skip(")");
    outInst1(REV, n); // 最初の引数が stack のトップに来るように反転させる
    outInst(PopECX);    // 余分にpushした書式文字列を削除する
    outInst1(CALL, (int)"printf");
}

void Expression(int *pt) {
    Add(pt);
}

//================================ 文の構文解析 ================================//
// ラベル発行
int ixLabel = 1;
int getLabel() { return ixLabel++; }

/// CompoundStatement ::= "{" (VariableDeclaration ";")* (Statements)* "}"
void CompoundStatement(int locBreak, int locContinue) {
    skip("{");
    while (tix < nToken && isTypeSpecifier()) {
        VariableDeclaration();   // ローカル変数の宣言
        skip(";");
    }
    while (!is("}")) Statement(locBreak, locContinue); 
    skip("}");
}

/// IfStatement ::= "if" "(" Expression ")" Statement1 [ "else" Statement2 ] 
void IfStatement(int locBreak, int locContinue) {
    int type, labelElse, labelEnd;
    skip("if");
    skip("(");
    Expression(&type);    // eax: 実行結果
    skip(")");
    outInst1(BEQ0, labelElse=getLabel()); // popして、0だったら,labelElseに
    Statement(locBreak, locContinue);        // Statement1
    if (is("else")) outInst1(JUMP, labelEnd=getLabel()); 
    outInst1(LABEL, labelElse);
    if (is("else")) {
        tix++;  // else をスキップ
        Statement(locBreak, locContinue);    // Statement2
        outInst1(LABEL, labelEnd);
    }
}

/// WhileStatement ::= "while" "(" Expression ")" Statement
void WhileStatement() {
    int type, labelContinue, labelBreak;
    skip("while");
    skip("(");
    outInst1(LABEL, labelContinue=getLabel());
    Expression(&type);    // eax: 実行結果
    skip(")");
    outInst1(BEQ0, labelBreak=getLabel()); // popして、0だったら,ラベルEndに
    Statement(labelBreak, labelContinue);     // Statement
    outInst1(JUMP, labelContinue);
    outInst1(LABEL, labelBreak);
}

// Statement ::= ( Assign | FuncCall ) ";"
void Statement(int locContinue, int locBreak) {
    if (is("{")) CompoundStatement(locBreak, locContinue);
    else if (is("if")) IfStatement(locContinue, locBreak);
    else if (is("while")) WhileStatement();
    else if (*token[tix+1] == '=') { Assign(); skip(";"); }
    else { FuncCall(); skip(";"); }
}


// Prog ::= ( VariableDeclaration | Statement )*
void Prog() {
    for (tix = 0; tix < nToken; ) {
        if (isTypeSpecifier()) VariableDeclaration();
        else Statement(0, 0);
    }
}

//-----------------------------------------------------------------------------//
//                       　　　　　命令実行                                    //
//-----------------------------------------------------------------------------//
#define CheckOverflow()  if (sp < nName) error("stack overflow! sp=%d", sp);
#define CheckEmpty()     if (sp >= MAXMEM) error("stack empty!");

#define  MAXMEM   1000
char    mem[MAXMEM];       // メモリ. 0～nName*8-1 までは name配列の変数に対応する領域
int     sp, pc;            // スタックポインタ、プログラムカウンタ
int     eax, ecx; 
double  fax, fcx;

void pushi(int val) { CheckOverflow(); sp -= 4; *((int*)&mem[sp]) = val; }
void pushd(double val) { CheckOverflow(); sp -= 8; *((double*)&mem[sp]) = val; }
int  popi()  { int v; CheckEmpty(); v = *((int*)&mem[sp]); sp += 4; return v; }
double popd(){ double v; CheckEmpty(); v = *((double*)&mem[sp]); sp += 8; return v; }
char *popp() { char *p; CheckEmpty(); p = *((char**)&mem[sp]); sp += 4; return p; }

void printInst(int n) {
    int code = insts[n];
    printf("%3d: %s ", n, OPCODE[code]);
    if (code==LOAD) printf("%s\n", name[insts[n+1]].name);
    else if (code==STORE) printf("%s\n", name[insts[n+1]].name);
    else if (code==LOADI) printf("%f\n", *((double*)insts[n+1]));
    else if (code==CALL) printf("%s\n", (char*)insts[n+1]);
    else if (code==LOADS) printf("\"%s\"\n", (char*)insts[n+1]);
    else printf("\n");
}

void execute(int fCode, int fTrace) {
    int    location[100];               // 目立つようにグローバル変数にしてもよい
    int    n, code, ival, type, fmt;

    for (n = 0; n < nInst; n += insts[n] < PUSHI ? 1 : 2) {
        if (fCode) printInst(n);
        if (insts[n] == LABEL) location[insts[n+1]] = n;
    }

    sp = MAXMEM;
    for (pc = 0; pc < nInst; ) {
        if (fTrace) printInst(pc);
        code = insts[pc];
        ival = insts[pc+1];
        pc += insts[pc] < PUSHI ? 1 : 2;
        switch (code) {
            case LABEL: break;  // 疑似命令
            case ADD: fax = fcx + fax; break;
            case SUB: fax = fcx - fax; break;
            case MUL: fax = fcx * fax; break;
            case DIV: fax = fcx / fax; break;
            case LOADI: fax = *((double*)ival); break;
            case LOADS: eax = ival; break;
            case LOAD: fax = *((double*)&mem[ival*8]); break;
            case STORE: *((double*)&mem[ival*8]) = fax; break;
            case PushEAX: pushi(eax); break;
            case PopECX: ecx = popi(); break;
            case PushFAX: pushd(fax); break;
            case PopFCX: fcx = popd(); break;
            case REV:   // printf関数呼び出しの引数を逆順にする
                for (n = 0; n < ival/2; n++) {
                    double tmp = *((double*)&mem[sp+n*8]);
                    *((double*)&mem[sp+n*8]) = *((double*)&mem[sp+(ival-n-1)*8]);
                    *((double*)&mem[sp+(ival-n-1)*8]) = tmp;
                }
                break;
            case CALL:
                if (strcmp((char*)ival,"printf") == 0) {
                    int fmt = *((int*)&mem[sp]);        // 書式文字列
                    eax = vprintf((char*)fmt, (char*)(int*)&mem[sp+4]);
                } else {
                    error("未実装システムコール：%s\n", (char*)ival);
                }
                break;
            case BEQ0: if (fax == 0) pc = location[ival]; break;
            case JUMP: pc = location[ival]; break;
            default: error("不明命令コード'%d'", code);
        }
    }
}

int main(int argc, char* argv[]) {
    int  n, fToken=0, fCode=0, fTrace=0;
    char *srcfile = NULL;
    for (n = 1; n < argc; n++) {
        if (strcmp(argv[n],"-token") == 0) fToken = 1;
        else if (strcmp(argv[n],"-code") == 0) fCode = 1;
        else if (strcmp(argv[n],"-trace") == 0) fTrace = 1;
        else { srcfile = argv[n]; break; }
    }
    nToken = lexer(srcfile, fToken);
    Prog();
    execute(fCode, fTrace);
}
