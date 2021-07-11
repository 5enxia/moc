#include <stdio.h>
#include <ctype.h>

int main() {
    int  ch;

    while ((ch = getchar()) != '\n') {
        if (ch <= ' ') continue;                    // 空白文字 [\n\t ]
        if (isalpha(ch)) {                          // 識別子   [A-Za-z][A-Za-z0-9]*
            printf("ID: %c", ch);
            while (isalnum(ch=getchar())){ 
				putchar(ch);
			}
            ungetc(ch, stdin);      // 最後の英数字でなかった文字 ch を入力バッファに戻す
            putchar('\n');
        } else if (isdigit(ch)) {                   // 数値     [0-9][0-9]*
            printf("NM: %c", ch);
            while (isdigit(ch=getchar())) {
				putchar(ch);
			}
            ungetc(ch, stdin);      // 最後の数字でなかった文字 ch を入力バッファに戻す
            putchar('\n');
        } else {                                    // 記号     [^A-Za-z0-9]
            printf("SY: %c\n", ch);
        }
    }
}