%{
#include "y.tab.h"
%}

%%
[0-9]+     { yylval.val = atof(yytext);
             return(NUMBER);
           }
[0-9]+\.[0-9]+ { 
             sscanf(yytext,"%f",&yylval.val);
             return(NUMBER);
           }
"+"        return(PLUS);
"-"        return(MINUS);
"*"        return(MULT);
"/"        return(DIV);
"^"        return(EXPON);
"("        return(LB);
")"        return(RB);
\n         return(EOL);
.          { yyerror("Illegal character"); return(EOL); }
%%