// mci01.c
#include <stdio.h>
#include <stdlib.h>     // atof関数を使用するために必要
#include <string.h>
#include <ctype.h>

#define TRUE   1
#define FALSE  0

char   *token[1000];
int     nToken;
int     tix;

char   *name[100];
int     nName;
double  mem[100];       // 各要素は name配列の各要素と１対１対応

void error(const char *format, ...) {
    vfprintf(stderr, format, (char*)&format + sizeof(char*));
    exit(1);
}

int isJp(int c){ return (c>=0x81 && c<=0x9F) || (c>=0xE0 && c<=0xFC); } // 2byte文字の判定

void skip(char *s) {    // 現在のトークンが s に一致すれば tix++ 、そうでなければエラー終了
    if (strcmp(token[tix++],s) != 0) { 
        error("'%s' expected. tix=%d", s, tix); 
    }
}

// 名前表探索
int searchName(char *s) {
    int n = 0;
    while (n < nName && strcmp(name[n],s) != 0) n++;  
    if (n == nName) {           // 見つからなかった
        error("変数'%s'は宣言されていません\n", s);
    }
    return n;   // 変数番号(0, 1, 2, ...) を返す 
}

// 字句解析
int lexer(int fToken) { // fToken が 1 のとき、トークンを表示する
    char linebuffer[1024];     // 行読み込みバッファ
    char buf[256];      // トークン文字列最大長 255 バイト（最後に終端文字 '\0'）
    char *p, *pBgn; 
    int  n = 0;         // トークン配列の添え字
  
    while ((p=fgets(linebuffer, sizeof(linebuffer), stdin)) != NULL) {
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
            sprintf(buf, "%.*s", p-pBgn, pBgn); // pBgnからp-pBgn文字をbufにコピー、末尾に'\0'
            if (fToken) printf("%03d: %s\n", n, buf);
            token[n++] = strdup(buf);
        }
    }
    return n;   // トークン数を返す 
}

// Prim ::= 数値 | Id | "(" Expr ")"
double Expr();           // 後で定義する

double Prim() {                 // 優先順位 #1
    double v;
    char *tkn = token[tix++];   // トークン
    if (isdigit(*tkn)) {      // トークンの先頭文字だけチェック
        v = atof(tkn); 
    } else if (isalpha(*tkn)) {
        int n = searchName(tkn);
        v = mem[n];
    } else if (*tkn == '(') {
        v = Expr();      // ( と ) の中の式を計算する
        skip(")");
    } else {
        error("'%s'は Prim ではありません\n", tkn);
    }
    return v;
}

// Mul  ::= Prim ( ( '*' | '/' ) Prim )*
double Mul() {                                  // 優先順位 #2
    double v1 = Prim();                 // ２項演算の左辺
    while (*token[tix] == '*' || *token[tix] == '/') {
        int op = *token[tix++];         // '*' または '/'
        double v2 = Prim();             // ２項演算の右辺
        v1 = op=='*' ? v1*v2 : v1/v2;
    }
    return v1;
}

// Add  ::= Mul  ( ( '+' | '-' ) Mul  )* 
double Add() {                                  // 優先順位 #3
    double v1 = Mul();                  // ２項演算の左辺
    while (*token[tix] == '+' || *token[tix] == '-') {
        int op = *token[tix++];         // '+' または '-'
        double v2 = Mul();              // ２項演算の右辺
        v1 = op=='+' ? v1+v2 : v1-v2;
    }
    return v1;
}

// Assign ::= Id "=" Expr
void Assign() {                                         // 優先順位 #4
    int n = searchName(token[tix++]);
    skip("=");
    mem[n] = Expr();
}

double Expr() { return Add(); }

// FuncCall ::= Id "(" [ Expr ( "," Expr )* ] ")"
void FuncCall() {
    char *fmt;
    double argv[16];    // printするのは double型数値のみ
    int n;

    skip("printf");     // 現在は printf関数しかサポートしていない
    skip("(");
    fmt = token[tix++];	// 書式文字列を特別扱い
    for (n = 0; strcmp(token[tix],")") != 0; n++) {
        skip(",");
        argv[n] = Expr();
    }
    skip(")");
    vprintf(fmt, (char*)&argv);
}

// Stmt ::= ( Assign | FuncCall ) ";"
void Stmt() {
    if (*token[tix+1] == '=') Assign();
    else FuncCall();
    skip(";");
}

// VarDecl ::= "double" Id ( "," Id )* ";"
void VarDecl() {
    skip("double");
    name[nName++] = token[tix++];
    while (strcmp(token[tix], ",") == 0) {
        skip(",");
        name[nName++] = token[tix++];
    }
    skip(";");
}

// Prog ::= ( VarDecl | Stmt )*
void Prog() {
    for (tix = 0; tix < nToken; ) {
        if (strcmp(token[tix],"double") == 0) VarDecl();
        else Stmt();
    }
}

int main() {
    nToken = lexer(TRUE);
    Prog();
}
