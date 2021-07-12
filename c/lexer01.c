// lexer01.c
#include <stdio.h>
#include <ctype.h>

void main(int argc, char* argv[]) {
    int  ch;
    FILE *fin;

    if((fin = fopen(argv[1], "r")) == NULL) {
        fprintf(stderr,"入力ファイルオープンエラー\n");
        exit(0);
    }
    while ((ch = fgetc(fin)) != EOF) {
        if (ch <= ' ') continue;                  // 空白文字 [\n\t ]
        if (isalpha(ch)) {                        // 識別子   [A-Za-z][A-Za-z0-9]*
            printf("ID: %c", ch);
            while (isalnum(ch=fgetc(fin))) putchar(ch);
            ungetc(ch, fin);                    // 未処理文字をバッファに戻す
        } else if (isdigit(ch)) {                 // 数値     [0-9]+
            printf("NM: %c", ch);
            while (isdigit(ch=fgetc(fin))) putchar(ch);
            ungetc(ch, fin);                    // 未処理文字をバッファに戻す
        } else if (ch == '\"') {        //==== 文字列 ====
            printf("ST: %c", ch);
            while ((ch=fgetc(fin)) != EOF && ch != '\"') {
                putchar(ch);
                if (ch=='\\') putchar(fgetc(fin));
            }
            if (ch != '\"') fprintf(stderr, "文字列末尾の二重引用符(\")がない");
            printf("%c", ch);                     // 末尾の "
        } else {
            printf("SY: %c", ch);                 // 記号
        }
        putchar('\n');
    }
    fclose(fin);
}
