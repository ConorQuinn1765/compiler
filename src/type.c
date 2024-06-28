#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "param_list.h"
#include "expr.h"

struct type* type_create(type_t kind, struct type* subtype, struct param_list* params, struct expr* value)
{
    struct type* pType = malloc(sizeof(struct type));
    
    if(pType)
    {
        pType->kind = kind;
        pType->subtype = subtype;
        pType->params = params;
        pType->value = value;
    }

    return pType;
}

bool type_equals(struct type* a, struct type* b)
{
    if(!a || !b) return false;

    if(a->kind == b->kind)
    {
        if(a->kind == TYPE_ARRAY)
        {
            if(!a->value && !b->value)
                return type_equals(a->subtype, b->subtype);

            if(!a->value || !b->value)
                return false;

            return type_equals(a->subtype, b->subtype) && a->value->integer_value == b->value->integer_value;
        }

        if(a->kind == TYPE_FUNCTION)
            return type_equals(a->subtype, b->subtype) && param_list_equals(a->params, b->params);

        return true;
    }

    return false;
}

struct type* type_copy(struct type* type)
{
    if(!type) return NULL;

    struct type* pType = malloc(sizeof(struct type));
    
    if(pType)
    {
        pType->kind = type->kind;
        pType->subtype = type_copy(type->subtype);
        pType->params = param_list_copy(type->params);
        pType->value = expr_copy(type->value);
    }

    return pType;
}

void type_resolve(struct type* pType)
{
    if(!pType) return;

    type_resolve(pType->subtype);
    param_list_resolve(pType->params);
    expr_resolve(pType->value);
}

void type_print(struct type* pType)
{
    if(pType)
    {
        switch(pType->kind)
        {
            case TYPE_BOOL:
                printf("boolean");
                break;
            case TYPE_CHAR:
                printf("char");
                break;
            case TYPE_VOID:
                printf("void");
                break;
            case TYPE_AUTO:
                printf("auto");
                break;
            case TYPE_ARRAY:
                printf("array [");
                if(pType->value)
                    expr_print(pType->value);
                printf("] ");
                type_print(pType->subtype);
                break;
            case TYPE_STRING:
                printf("string");
                break;
            case TYPE_INTEGER:
                printf("integer");
                break;
            case TYPE_FUNCTION:
                printf("function ");
                type_print(pType->subtype);
                printf("(");
                param_list_print(pType->params);
                printf(")");
                break;
            default:
                break;
        }
    }
}

const char* type_string(struct type* pType)
{
    if(!pType) return NULL;

    switch(pType->kind)
    {
    case TYPE_BOOL:
        return "bool";
    case TYPE_CHAR:
        return "character";
    case TYPE_STRING:
        return "string";
    case TYPE_INTEGER:
        return "integer";
    case TYPE_ARRAY:
        return "array";
    case TYPE_FUNCTION:
        return "function";
    default:
        return "unknown";
    }
}

void type_destroy(struct type** ppType)
{
    if(ppType && *ppType)
    {
        struct type* pType = *ppType;

        type_destroy(&pType->subtype);
        param_list_destroy(&pType->params);
        expr_destroy(&pType->value);

        free(pType);
        *ppType = NULL;
    }
}

