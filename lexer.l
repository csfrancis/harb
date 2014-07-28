%{
#include <assert.h>
#include <string.h>

#include "nodes.h"

#define YYSTYPE harb::node::base*

#include "parser.h"

%}

%option noyywrap nounput
%option reentrant
%option bison-bridge
%%

"!="  { return NEQ; }
"=="  { return EQ; }
[aA][nN][dD] { return AND; }
[oO][rR]  { return OR; }
">="  { return GTE; }
"<="  { return LTE; }
">"|"<"|"("|")"|"|"|"\n"|"," { return yytext[0]; }

[sS][tT][aA][tT][sS] { return STATS; }
[tT][oO][pP] { return TOP; }
[rR][oO][oO][tT][pP][aA][tT][hH] { return ROOTPATH; }
[pP][rR][iI][nN][tT] { return PRINT; }
[wW][hH][eE][rR][eE] { return WHERE; }
[bB][yY] { return BY; }

"\""[A-Za-z0-9]+"\"" {
	//SET_NODE_STRING((*yylval), strdup(yytext));
	return STRING;
}

[0-9]+ {
	*yylval = new harb::node::number(strtoull(yytext, NULL, 0));
	return NUM;
}

"0"[xX][a-fA-F0-9]+ {
	*yylval = new harb::node::number(strtoull(yytext, NULL, 0));
	return NUM;
}

[a-z][a-zA-Z0-9]+ {
	//SET_NODE_IDENT((*yylval), strdup(yytext));
	return IDENT;
}

[ \t]	;

. { return INVALID_CHARACTER; }

%%
