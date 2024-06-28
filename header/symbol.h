#ifndef SYMBOL_H
#define SYMBOL_H
#include "type.h"

typedef enum {SYMBOL_GLOBAL, SYMBOL_PARAM, SYMBOL_LOCAL} symbol_t;

struct symbol
{
    symbol_t kind;
    int which;
    struct type* type;
    char* name;
};

struct symbol* symbol_create(symbol_t kind, struct type* type, char* name);
const char* symbol_codegen(struct symbol* symbol);
struct symbol* symbol_copy(struct symbol* symbol);
bool symbol_equal(struct symbol* a, struct symbol* b);
void symbol_print(struct symbol* sym);
void symbol_destroy(struct symbol** sym);

void scope_enter(void);
void scope_exit(void);
int scope_level(void);

void scope_bind(const char* name,struct symbol* sym);
struct symbol* scope_lookup(const char* name);
struct symbol* scope_lookup_current(const char* name);

#endif
