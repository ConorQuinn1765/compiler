#ifndef PARAM_LIST_H
#define PARAM_LIST_H
#include "type.h"
#include "symbol.h"

struct param_list
{
    char* name;
    struct type* type;
    struct param_list* next;
    struct symbol* symbol;
};

struct param_list* param_list_create(char* name, struct type* type, struct param_list* next);
bool param_list_equals(struct param_list* a, struct param_list* b);
struct param_list* param_list_copy(struct param_list* pParams);

void param_list_resolve(struct param_list* pParams);
void param_list_typecheck(struct param_list* a, struct param_list* b);
void param_list_typecheck_call(struct param_list* a, struct expr* b);
void param_list_print(struct param_list* pParams);
void param_list_destroy(struct param_list** ppParam_list);

#endif
