// lexer02.c
#include <stdio.h>
#include <ctype.h>
#include <string.h>

enum EnumTokenType { ID, NM, ST, SY };
char *TokenType[] = { "ID", "NM", "ST", "SY" };
typedef struct _Token {
    int  type;
    char *str;
} Token;

#define NTOKEN 1000

Token token[NTOKEN];

int lexer(char *path) {
    FILE *fin;
    int  ch;
    char buf[256], *p;
    int n = 0;

    if((fin = fopen(path, "r")) == NULL) {
        fprintf(stderr,"入力ファイルオープンエラー\n");
        exit(0);
    }
    while ((ch = fgetc(fin)) != EOF) {
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
            if (ch != '\"') fprintf(stderr, "文字列末尾の二重引用符(\")がない");
        } else {
            token[n].type = SY;
            *p++ = ch;                            // 記号
        }
        *p++ = '\0';
        token[n++].str = strdup(buf);
    }
    fclose(fin);
    return n;
}

void main(int argc, char* argv[]) {
    int n;
    int nToken = lexer(argv[1]);
    for (n = 0; n < nToken; n++) {
        printf("%s: %s\n", TokenType[token[n].type], token[n].str);
    }
}
