%token TOKEN_EOF TOKEN_ARRAY TOKEN_AUTO TOKEN_BOOLEAN TOKEN_CHAR TOKEN_ELSE TOKEN_FALSE
%token TOKEN_FOR TOKEN_FUNCTION TOKEN_IF TOKEN_INTEGER TOKEN_PRINT TOKEN_RETURN TOKEN_STRING
%token TOKEN_TRUE TOKEN_VOID TOKEN_WHILE TOKEN_STRING_LITERAL TOKEN_CHAR_LITERAL TOKEN_INT_CONSTANT
%token TOKEN_ID TOKEN_INC TOKEN_DEC TOKEN_LE TOKEN_LT TOKEN_GE TOKEN_GT TOKEN_EQ TOKEN_NE TOKEN_AND
%token TOKEN_OR TOKEN_PLUS TOKEN_MINUS TOKEN_STAR TOKEN_SLASH TOKEN_PERCENT TOKEN_CARET TOKEN_NOT
%token TOKEN_ASSIGN TOKEN_COLON TOKEN_SEMICOLON TOKEN_COMMA TOKEN_LPAREN TOKEN_RPAREN TOKEN_LBRACKET
%token TOKEN_RBRACKET TOKEN_LBRACE TOKEN_RBRACE TOKEN_ERROR

%expect 1

%union {
    struct decl* decl;
    struct stmt* stmt;
    struct expr* expr;
    struct type* type;
    struct param_list* param_list;
    char* name;
};

%type <decl> program global_decl function_decl decl
%type <stmt> stmt compound_stmt compound_stmt_list
%type <expr> expr assign_expr  or_expr and_expr compare_expr add_expr 
%type <expr> mul_expr exponent_expr base_expr unary_expr nested_init increment_expr
%type <expr> add_op mul_op compare_op primary_expr postfix_expr argument_list argument_list_p
%type <expr> init_list init_list_p opt_expr
%type <type> prim_type  array_type array_no_expr arg_type type
%type <param_list> formal_argument_list decl_arg_list arg_decl
%type <name> id

%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "decl.h"
#include "stmt.h"
#include "expr.h"
#include "type.h"
#include "param_list.h"

#define STRMAX 256

extern char *yytext;
extern int yylex();
extern int yyerror( char *str );

struct decl* parser_result = 0;

void cleanString(char* input, char* output);

%}

%%
program: global_decl program { parser_result = $1; $1->next = $2; };
program: %empty { $$ = 0; };

global_decl: function_decl { $$ = $1; };
global_decl: decl TOKEN_SEMICOLON { $$ = $1; };

function_decl: id TOKEN_COLON TOKEN_FUNCTION type formal_argument_list TOKEN_SEMICOLON
                    { $$ = decl_create($1, type_create(TYPE_FUNCTION, $4, $5, 0), 0, 0, 0); };
function_decl: id TOKEN_COLON TOKEN_FUNCTION type formal_argument_list TOKEN_ASSIGN compound_stmt
                    { $$ = decl_create($1, type_create(TYPE_FUNCTION, $4, $5, 0), 0, $7, 0); };

formal_argument_list: TOKEN_LPAREN TOKEN_RPAREN { $$ = 0; };
formal_argument_list: TOKEN_LPAREN decl_arg_list TOKEN_RPAREN { $$ = $2; };

decl_arg_list: arg_decl { $$ = $1; };
decl_arg_list: arg_decl TOKEN_COMMA decl_arg_list { $$ = $1; $1->next = $3; };

arg_decl: id TOKEN_COLON arg_type { $$ = param_list_create($1, $3, 0); };

decl: id TOKEN_COLON prim_type { $$ = decl_create($1, $3, 0, 0, 0); };
decl: id TOKEN_COLON prim_type TOKEN_ASSIGN or_expr { $$ = decl_create($1, $3, $5, 0, 0); };
decl: id TOKEN_COLON TOKEN_AUTO TOKEN_ASSIGN or_expr { $$ = decl_create($1, type_create(TYPE_AUTO, 0, 0, 0), $5, 0, 0); };
decl: id TOKEN_COLON TOKEN_AUTO TOKEN_ASSIGN init_list { $$ = decl_create($1, type_create(TYPE_AUTO, 0, 0, 0), $5, 0, 0); };
decl: id TOKEN_COLON array_type { $$ = decl_create($1, $3, 0, 0, 0); };
decl: id TOKEN_COLON array_type TOKEN_ASSIGN init_list { $$ = decl_create($1, $3, $5, 0, 0); };

init_list: TOKEN_LBRACE nested_init init_list_p TOKEN_RBRACE { $$ = expr_create(EXPR_INIT_LIST, $2, $3); };
init_list_p: TOKEN_COMMA nested_init init_list_p { $$ = expr_create(EXPR_ARG, $2, $3); };
init_list_p: %empty { $$ = 0; };

nested_init: init_list { $$ = $1; };
nested_init: or_expr { $$ = $1; };

stmt: decl TOKEN_SEMICOLON { $$ = stmt_create(STMT_DECL, $1, 0, 0, 0, 0, 0, 0); };
stmt: expr TOKEN_SEMICOLON { $$ = stmt_create(STMT_EXPR, 0, 0, $1, 0, 0, 0, 0); };
stmt: TOKEN_FOR TOKEN_LPAREN opt_expr TOKEN_SEMICOLON opt_expr TOKEN_SEMICOLON opt_expr TOKEN_RPAREN stmt 
        { $$ = stmt_create(STMT_FOR, 0, $3, $5, $7, $9, 0, 0); };
stmt: TOKEN_IF TOKEN_LPAREN expr TOKEN_RPAREN stmt { $$ = stmt_create(STMT_IF_ELSE, 0, 0, $3, 0, $5, 0, 0); };
stmt: TOKEN_IF TOKEN_LPAREN expr TOKEN_RPAREN stmt TOKEN_ELSE stmt { $$ = stmt_create(STMT_IF_ELSE, 0, 0, $3, 0, $5, $7, 0); };
stmt: TOKEN_PRINT argument_list TOKEN_SEMICOLON { $$ = stmt_create(STMT_PRINT, 0, 0, $2, 0, 0, 0, 0); };
stmt: TOKEN_RETURN opt_expr TOKEN_SEMICOLON { $$ = stmt_create(STMT_RETURN, 0, 0, $2, 0, 0, 0, 0); };
stmt: compound_stmt { $$ = $1; };

opt_expr: expr { $$ = $1; };
opt_expr: %empty { $$ = 0; };

compound_stmt: TOKEN_LBRACE compound_stmt_list TOKEN_RBRACE { $$ = stmt_create(STMT_BLOCK, 0, 0, 0, 0, $2, 0, 0); };
compound_stmt_list: stmt compound_stmt_list { $$ = $1; $1->next = $2; };
compound_stmt_list: %empty { $$ = 0; };

expr: assign_expr { $$ = $1; };

assign_expr: or_expr TOKEN_ASSIGN assign_expr { $$ = expr_create(EXPR_ASSIGN, $1, $3); };
assign_expr: or_expr { $$ = $1; };

or_expr: and_expr TOKEN_OR or_expr { $$ = expr_create(EXPR_OR, $1, $3); };
or_expr: and_expr { $$ = $1; };

and_expr: compare_expr TOKEN_AND and_expr { $$ = expr_create(EXPR_AND, $1, $3); };
and_expr: compare_expr { $$ = $1; };

compare_expr: add_expr compare_op compare_expr { $$ = $2; $2->left = $1; $2->right = $3; };
compare_expr: add_expr { $$ = $1; };

add_expr: mul_expr add_op add_expr { $$ = $2; $2->left = $1; $2->right = $3; };
add_expr: mul_expr { $$ = $1; };

mul_expr: exponent_expr mul_op mul_expr { $$ = $2; $2->left = $1; $2->right = $3; };
mul_expr: exponent_expr { $$ = $1; };

exponent_expr: base_expr TOKEN_CARET exponent_expr { $$ = expr_create(EXPR_EXPONENT, $1, $3); };
exponent_expr: base_expr { $$ = $1; };

base_expr: unary_expr { $$ = $1; };

add_op: TOKEN_PLUS { $$ = expr_create(EXPR_ADD, 0, 0); };
add_op: TOKEN_MINUS { $$ = expr_create(EXPR_SUB, 0, 0); };

mul_op: TOKEN_STAR { $$ = expr_create(EXPR_MUL, 0, 0); };
mul_op: TOKEN_SLASH { $$ = expr_create(EXPR_DIV, 0, 0); };
mul_op: TOKEN_PERCENT { $$ = expr_create(EXPR_MOD, 0, 0); };

compare_op: TOKEN_LT { $$ = expr_create(EXPR_LT, 0, 0); };
compare_op: TOKEN_LE { $$ = expr_create(EXPR_LE, 0, 0); };
compare_op: TOKEN_GT { $$ = expr_create(EXPR_GT, 0, 0); };
compare_op: TOKEN_GE { $$ = expr_create(EXPR_GE, 0, 0); };
compare_op: TOKEN_EQ { $$ = expr_create(EXPR_EQ, 0, 0); };
compare_op: TOKEN_NE { $$ = expr_create(EXPR_NE, 0, 0); };

prim_type: TOKEN_INTEGER { $$ = type_create(TYPE_INTEGER, 0, 0, 0); };
prim_type: TOKEN_STRING { $$ = type_create(TYPE_STRING, 0, 0, 0); };
prim_type: TOKEN_CHAR { $$ = type_create(TYPE_CHAR, 0, 0, 0); };
prim_type: TOKEN_BOOLEAN { $$ = type_create(TYPE_BOOL, 0, 0, 0); };

array_no_expr: TOKEN_ARRAY TOKEN_LBRACKET TOKEN_RBRACKET arg_type { $$ = type_create(TYPE_ARRAY, $4, 0, 0); };

array_type: TOKEN_ARRAY TOKEN_LBRACKET or_expr TOKEN_RBRACKET type { $$ = type_create(TYPE_ARRAY, $5, 0, $3); };
array_type: array_no_expr { $$ = $1; };

arg_type: prim_type { $$ = $1; };
arg_type: array_no_expr { $$ = $1; };
arg_type: TOKEN_FUNCTION type formal_argument_list { $$ = type_create(TYPE_FUNCTION, $2, $3, 0); };

type: prim_type  { $$ = $1; };
type: array_type { $$ = $1; };
type: TOKEN_VOID { $$ = type_create(TYPE_VOID, 0, 0, 0); };
type: TOKEN_FUNCTION type formal_argument_list { $$ = type_create(TYPE_FUNCTION, $2, $3, 0); };

id: TOKEN_ID { char* buffer = malloc(sizeof(char) * STRMAX); strncpy(buffer, yylval.name, STRMAX); $$ = buffer; };

primary_expr: id { $$ = expr_create_name($1); };
primary_expr: TOKEN_INT_CONSTANT
    { char buffer[STRMAX]; strncpy(buffer, yylval.name, STRMAX); $$ = expr_create_integer_literal(atoi(buffer)); };
primary_expr: TOKEN_STRING_LITERAL
    { char buffer[STRMAX]; cleanString(yylval.name, buffer); $$ = expr_create_string_literal(buffer); };
primary_expr: TOKEN_CHAR_LITERAL
    { char buffer[STRMAX]; cleanString(yylval.name, buffer); $$ = expr_create_char_literal(buffer[0]); };

primary_expr: TOKEN_TRUE { $$ = expr_create_boolean_literal(1); };
primary_expr: TOKEN_FALSE { $$ = expr_create_boolean_literal(0); };
primary_expr: TOKEN_LPAREN expr TOKEN_RPAREN { $$ = expr_create(EXPR_GROUP, $2, 0); };

postfix_expr: primary_expr { $$ = $1; };
postfix_expr: postfix_expr TOKEN_LBRACKET expr TOKEN_RBRACKET { $$ = expr_create(EXPR_SUBSCRIPT, $1, $3); };
postfix_expr: postfix_expr TOKEN_LPAREN argument_list TOKEN_RPAREN { $$ = expr_create(EXPR_CALL, $1, $3); };

increment_expr: increment_expr TOKEN_INC { $$ = expr_create(EXPR_INC, $1, 0); };
increment_expr: increment_expr TOKEN_DEC { $$ = expr_create(EXPR_DEC, $1, 0); };
increment_expr: postfix_expr { $$ = $1; };

argument_list: or_expr argument_list_p { $$ = expr_create(EXPR_ARG, $1, $2); };
argument_list: %empty { $$ = 0; };
argument_list_p: TOKEN_COMMA or_expr argument_list_p { $$ = expr_create(EXPR_ARG, $2, $3); };
argument_list_p: %empty { $$ = 0; };

unary_expr: TOKEN_MINUS increment_expr { $$ = expr_create(EXPR_UNARY_MINUS, $2, 0); };
unary_expr: TOKEN_NOT increment_expr { $$ = expr_create(EXPR_NOT, $2, 0); };
unary_expr: increment_expr { $$ = $1; };

%%

int yyerror( char *str )
{
	printf("%s\n",str);
	return 0;
}

void cleanString(char* input, char* output)
{
    memset(output, 0, STRMAX);
    int inputLen = strlen(input);
    int idxOut = 0; // index for the next output character

    for(int i=0; i < inputLen; i++)
    {
        switch(input[i])
        {
        case '\\':
            if(i + 1 < inputLen)
            {
                int j = i + 1;
                if(input[j] == 'n')
                {
                    output[idxOut] = 10;    // Actual newline character
                    idxOut++;
                    i++;
                    break;
                }
                else if(input[j] == '0')
                {
                    output[idxOut] = 0;
                    idxOut++;
                    i++;
                    break;
                }
                else
                {
                    output[idxOut] = input[j];
                    idxOut++;
                    i++;
                    break;
                }
            }
            else
            {
                output[idxOut] = input[i];
                idxOut++;
                break;
            }
            break;
        case '"':
        case '\'':
            break;
        default:
            output[idxOut] = input[i];
            idxOut++;
            break;
        }
    }
}
