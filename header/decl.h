#ifndef DECL_H
#define DECL_H
#include "type.h"
#include "symbol.h"
#include "register.h"

struct decl
{
    char* name;
    struct type* type;
    struct expr* value;
    struct stmt* code;
    struct symbol* symbol;
    struct decl* next;
};

struct decl* decl_create(char* name, struct type* type, struct expr* value, struct stmt* code, struct decl* next);
void decl_resolve(struct decl* pDecl);
void decl_typecheck(struct decl* pDecl);
void decl_codegen(struct decl* pDecl, struct scratch regs[]);
void decl_print(struct decl* pDecl);
void decl_destroy(struct decl** ppDecl);

#endif
