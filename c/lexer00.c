#include <stdio.h>
#include <ctype.h>

int main() {
    int  ch;
        
    while ((ch = getchar()) != EOF) {
        if (ch <= ' ') continue;                  // 空白文字 [\n\t ]
        if (isalpha(ch)) {                        // 識別子   [A-Za-z][A-Za-z0-9]*
            printf("ID: %c", ch);
            while (isalnum(ch=getchar())) putchar(ch);
            ungetc(ch, stdin);                    // 未処理文字をバッファに戻す
        } else if (isdigit(ch)) {                 // 数値     [0-9]+
            printf("NM: %c", ch);
            while (isdigit(ch=getchar())) putchar(ch);
            ungetc(ch, stdin);                    // 未処理文字をバッファに戻す
        } else {
            printf("SY: %c", ch);                 // 記号
        }
        putchar('\n');
    }
}