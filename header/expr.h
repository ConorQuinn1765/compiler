#ifndef EXPR_H
#define EXPR_H
#include "decl.h"
#include "symbol.h"
#include "register.h"

typedef enum {EXPR_ASSIGN, EXPR_OR, EXPR_AND, EXPR_EQ, EXPR_NE, EXPR_LT, EXPR_LE, EXPR_INIT_LIST,
                EXPR_GT, EXPR_GE, EXPR_ADD, EXPR_SUB, EXPR_MUL, EXPR_DIV, EXPR_MOD, EXPR_INC, EXPR_DEC,
                EXPR_SUBSCRIPT, EXPR_CALL, EXPR_ARG, EXPR_CHAR_LITERAL, EXPR_BOOL_LITERAL, EXPR_GROUP,
                EXPR_EXPONENT, EXPR_STRING_LITERAL, EXPR_INT_LITERAL, EXPR_NAME, EXPR_NOT, EXPR_UNARY_MINUS} expr_t;

struct expr
{
    struct expr* left;
    struct expr* right;
    expr_t kind;

    int integer_value;
    const char* name;
    const char* string_literal;
    struct symbol* symbol;
    int reg;
};

struct expr* expr_create(expr_t kind, struct expr* left, struct expr* right);
struct expr* expr_create_name( const char *name );
struct expr* expr_create_integer_literal( int i );
struct expr* expr_create_boolean_literal( int b );
struct expr* expr_create_char_literal( char c );
struct expr* expr_create_string_literal( const char *str );
struct expr* expr_copy(struct expr* expr);

void expr_resolve(struct expr* pExpr);
struct type* expr_typecheck(struct expr* pExpr);
void expr_codegen(struct expr* pExpr, struct decl *pDecl, struct scratch regs[], int offset, bool isGlobal);
void expr_print(struct expr* expr);
void expr_destroy(struct expr** ppExpr);

#endif
