%{
#include <stdio.h>
#include <math.h>
void yyerror(char *s) { printf("%s\n",s); }
int  yywrap(void) { return 1; }
%}

%union {
   float val;
}

%token NUMBER
%token PLUS MINUS MULT DIV EXPON
%token EOL
%token LB RB

%left  MINUS PLUS
%left  MULT DIV
%right EXPON

%type  <val> exp NUMBER

%%
input   :
        | input line
        ;

line    : EOL
        | exp EOL { printf("%g\n",$1);}

exp     : NUMBER                 { $$ = $1;        }
        | exp PLUS  exp          { $$ = $1 + $3;   }
        | exp MINUS exp          { $$ = $1 - $3;   }
        | exp MULT  exp          { $$ = $1 * $3;   }
        | exp DIV   exp          { $$ = $1 / $3;   }
        | MINUS  exp %prec MINUS { $$ = -$2;       }
        | exp EXPON exp          { $$ = pow($1,$3);}
        | LB exp RB              { $$ = $2;        }
        ;
%%

int main() {
  yyparse();
}