#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "param_list.h"
#include "symbol.h"
#include "type.h"
#include "expr.h"

extern int STRMAX;

struct param_list* param_list_create(char* name, struct type* type, struct param_list* next)
{
    struct param_list* pParams = malloc(sizeof(struct param_list));

    if(pParams)
    {
        pParams->name = name;
        pParams->type = type;
        pParams->next = next;
        pParams->symbol = NULL;
    }

    return pParams;
}

bool param_list_equals(struct param_list* a, struct param_list* b)
{
    if(!a && !b) return true;
    if(!a || !b) return false;

    if(strncmp(a->name, b->name, STRMAX) == 0 && type_equals(a->type, b->type) && param_list_equals(a->next, b->next))
        return true;

    return false;
}

struct param_list* param_list_copy(struct param_list* params)
{
    if(!params) return NULL;

    struct param_list* pParams = malloc(sizeof(struct param_list));
    if(pParams)
    {
        pParams->name = malloc(sizeof(char) * STRMAX);
        if(!pParams->name)
        {
            free(pParams);
            return NULL;
        }

        strncpy(pParams->name, params->name, STRMAX);
        pParams->type = type_copy(params->type);
        pParams->next = param_list_copy(params->next);
        pParams->symbol = params->symbol;
    }

    return pParams;
}

void param_list_resolve(struct param_list* pParams)
{
    if(!pParams) return;

    pParams->symbol = symbol_create(SYMBOL_PARAM, pParams->type, pParams->name);
    scope_bind(pParams->name, pParams->symbol);    

    param_list_resolve(pParams->next);
}

void param_list_typecheck(struct param_list* a, struct param_list* b)
{
    if(!a && !b)
        return;

    if(!type_equals(a->type, b->type))
    {
        printf("type error: function declaration has different parameter list than function definition.\n");
        printf("Expected type ("); type_print(b->type); printf("), Actual type (");
        type_print(a->type); printf(")\n");
    }

    param_list_typecheck(a->next, b->next);
}

void param_list_typecheck_call(struct param_list* a, struct expr* b)
{
    if(!a && b)
    {
        printf("type error: function call requires fewer arguments than it was given\n");
        return;
    }

    if(a && !b)
    {
        printf("type error: function call requires more arguments than it was given\n");
        return;
    }

    if(!b) return;

    struct type* b_type = expr_typecheck(b->left);
    if(!type_equals(a->type, b_type))
    {
        printf("type error: function call expects type (");
        type_print(a->type); printf(") recieved type (");
        type_print(b_type); printf(")\n");
        expr_print(b); printf("\n");
    }

    type_destroy(&b_type);
    param_list_typecheck_call(a->next, b->right);
}

void param_list_print(struct param_list* pParams)
{
    if(pParams)
    {
        printf("%s: ", pParams->name);
        type_print(pParams->type);

        if(pParams->next)
        {
            printf(", ");
            param_list_print(pParams->next);
        }
    }
}

void param_list_destroy(struct param_list** ppParam_list)
{
    if(ppParam_list && *ppParam_list)
    {
        struct param_list* pParams = *ppParam_list;
        param_list_destroy(&pParams->next);
        
        type_destroy(&pParams->type);
        symbol_destroy(&pParams->symbol);

        free(pParams->name);
        free(pParams);
        *ppParam_list = NULL;
    }
}

