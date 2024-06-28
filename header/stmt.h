#ifndef STMT_H
#define STMT_H

#include "expr.h"

typedef enum {STMT_DECL, STMT_EXPR, STMT_IF_ELSE, STMT_FOR, STMT_PRINT, STMT_RETURN, STMT_BLOCK} stmt_t;

struct stmt 
{
    stmt_t kind;
    struct decl* decl;
    struct expr* init_expr;
    struct expr* expr;
    struct expr* next_expr;
    struct stmt* body;
    struct stmt* else_body;
    struct stmt* next;
};

struct stmt* stmt_create(stmt_t kind, struct decl* decl, struct expr* init_expr, struct expr* expr,
                struct expr* next_expr, struct stmt* body, struct stmt* else_body, struct stmt* next);

void stmt_resolve(struct stmt* pStmt);
void stmt_typecheck(struct stmt* pStmt, struct symbol* symbol);
void stmt_codegen(struct stmt* pStmt, struct decl *pDecl, struct scratch regs[], struct symbol* sym);
void stmt_print(struct stmt* pStmt, int depth);
void stmt_destroy(struct stmt** ppStmt);

#endif
