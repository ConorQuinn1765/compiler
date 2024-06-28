%{
#include "parser.h"
#ifdef YYLMAX
#undef YYLMAX
#endif
#define YYLMAX 256
%}

DIGIT   [0-9]
CHAR    \'(\\?.)?\'
ID      [a-zA-Z_][a-zA-Z0-9_]*
STRING  \"[^\"]*\"
COMMENT (\/\*((\*[^\/])|[^*])*(\*\/))|(\/\/.*)

%%
(" "|\t|\n|\r) {}

"array"    { return TOKEN_ARRAY; }
"auto"     { return TOKEN_AUTO; }
"boolean"  { return TOKEN_BOOLEAN; }
"char"     { return TOKEN_CHAR; }
"else"     { return TOKEN_ELSE; }
"false"    { return TOKEN_FALSE; }
"for"      { return TOKEN_FOR; }
"function" { return TOKEN_FUNCTION; }
"if"       { return TOKEN_IF; }
"integer"  { return TOKEN_INTEGER; }
"print"    { return TOKEN_PRINT; }
"return"   { return TOKEN_RETURN; }
"string"   { return TOKEN_STRING; }
"true"     { return TOKEN_TRUE; }
"void"     { return TOKEN_VOID; }
"while"    { return TOKEN_WHILE; }


{DIGIT}+    { yylval.name = yytext; return TOKEN_INT_CONSTANT; }

{CHAR}      { yylval.name = yytext; return TOKEN_CHAR_LITERAL; }

{ID}        { yylval.name = yytext; return TOKEN_ID; }

{STRING}    { yylval.name = yytext; return TOKEN_STRING_LITERAL; }

{COMMENT}

"++"        { return TOKEN_INC; }
"--"        { return TOKEN_DEC; }
"<="        { return TOKEN_LE; }
"<"         { return TOKEN_LT; }
">="        { return TOKEN_GE; }
">"         { return TOKEN_GT; }
"=="        { return TOKEN_EQ; }
"!="        { return TOKEN_NE; }
"&&"        { return TOKEN_AND; }
"||"        { return TOKEN_OR; }
"+"         { return TOKEN_PLUS; }
"-"         { return TOKEN_MINUS; }
"*"         { return TOKEN_STAR; }
"/"         { return TOKEN_SLASH; }
"%"         { return TOKEN_PERCENT; }
"^"         { return TOKEN_CARET; }
"!"         { return TOKEN_NOT; }
"="         { return TOKEN_ASSIGN; }
"("         { return TOKEN_LPAREN;}
")"         { return TOKEN_RPAREN; }
"["         { return TOKEN_LBRACKET; }
"]"         { return TOKEN_RBRACKET; }
"{"         { return TOKEN_LBRACE; }
"}"         { return TOKEN_RBRACE; }
":"         { return TOKEN_COLON; }
";"         { return TOKEN_SEMICOLON; }
","         { return TOKEN_COMMA; }
.           { return TOKEN_ERROR; }

%%

int yywrap(void) { return 1; }
