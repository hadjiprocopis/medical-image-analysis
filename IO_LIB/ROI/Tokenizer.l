/* suppress "yyunput defined but not used" */
%option yylineno
%option nounput

%{
#include <stdio.h>
#include <string.h>

#define YYSTYPE char *
#include "y.tab.h"
int	lex_lineNumber = 0; /* not 1 */
char	*lex_currentToken; /* holds the current token, for use with the parser */
%}
 
%%



[+\-]?([0-9]+)((\.)([0-9]+))?((\E([\+|\-|]?)([0-9]*))?)			{ lex_currentToken=strdup(yytext); yylval=(YYSTYPE )strdup(yytext); return NUMBER; }

"Irregular_region"							{ lex_currentToken=strdup(yytext); return IRREGULAR; }
"Elliptical_region"							{ lex_currentToken=strdup(yytext); return ELLIPTICAL; }
"Rectangular_region"							{ lex_currentToken=strdup(yytext); return RECTANGULAR; }
"End_region"								{ lex_currentToken=strdup(yytext); return END_REGION; }
"Name"									{ lex_currentToken=strdup(yytext); return NAME; }
"Image"									{ lex_currentToken=strdup(yytext); return IMAGE; }
"Points"								{ lex_currentToken=strdup(yytext); return POINTS; }
"X0"									{ lex_currentToken=strdup(yytext); return X0; }
"Y0"									{ lex_currentToken=strdup(yytext); return Y0; }
"Width"									{ lex_currentToken=strdup(yytext); return WIDTH; }
"Height"								{ lex_currentToken=strdup(yytext); return HEIGHT; }
"EX0"									{ lex_currentToken=strdup(yytext); return EX0; }
"EY0"									{ lex_currentToken=strdup(yytext); return EY0; }
"EA"									{ lex_currentToken=strdup(yytext); return EA; }
"EB"									{ lex_currentToken=strdup(yytext); return EB; }
"ROT"									{ lex_currentToken=strdup(yytext); return ROT; }

"="									{ lex_currentToken=strdup(yytext); return ASSIGN; }
\"(([^\"\\\n\r])|(\\([ntbrf\\'\"]|[0-7]([0-7])?|[0-3][0-7][0-7])))*\"	{ char *dummy = strdup(&(yytext[1])); dummy[strlen(dummy)-1] = '\0'; yylval=(YYSTYPE )dummy; lex_currentToken=strdup(yytext); return STRING; }

\/\*[^\*]*\*([^\/]([^\*])*\*)*\/					{ lex_lineNumber++; /* C style comment */; }
\/\/([^\n])*\n								{ lex_lineNumber++; /* C++ style comment */; }

[ \t]+									{ /* ignore whitespace */; }
\n									{ lex_lineNumber++; }

%%
