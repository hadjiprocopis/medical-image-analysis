/* suppress "yyunput defined but not used" */
%option yylineno
%option nounput

%{
#include <stdio.h>
#include <string.h>
#define YYSTYPE double
#include "Grammar.tab.h"

extern	YYSTYPE	yylval;
int	lex_lineNumber = 0; /* not 1 */

%}

digit		[0-9]
integer		{digit}+
exponent	[eE][+-]?{integer}
real		{integer}((\.){integer})?{exponent}?
 
%%

{real}		{ yylval = atof(yytext); return H_NUMBER; }

\/\*[^\*]*\*([^\/]([^\*])*\*)*\/					{ lex_lineNumber++; /* C style comment */; }
\/\/([^\n])*\n								{ lex_lineNumber++; /* C++ style comment */; }
\#([^\n])*\n								{ lex_lineNumber++; /* csh style (#) comment */; }

[ \t]+									{ /* ignore whitespace */; }
\n									{ lex_lineNumber++; }

%%
