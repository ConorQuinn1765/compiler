#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "symbol.h"
#include "hash_table.h"
#include "stack.h"
#include "type.h"
#include "expr.h"

extern int STRMAX;
stack* scope_stack = NULL;

struct symbol* symbol_create(symbol_t kind, struct type* type, char* name)
{
    static int local_var_count = 0;
    struct symbol* sym = malloc(sizeof(struct symbol));
    if(sym)
    {
        sym->kind = kind;
        sym->type = type_copy(type);
        if(sym->type->kind == TYPE_AUTO)
        {
            struct type* temp = expr_typecheck(sym->type->value);
            type_destroy(&sym->type);
            sym->type = temp;
        }

        if(sym->kind == SYMBOL_GLOBAL)
        {
            sym->which = 0;
            local_var_count = 0;
        }
        else
        {
            if(sym->type->kind == TYPE_ARRAY)
            {
                if(sym->kind == SYMBOL_LOCAL)
                {
                    sym->which = local_var_count;
                    local_var_count += sym->type->value->integer_value;
                }
            }
            else
                sym->which = local_var_count++;
        }

        sym->name = malloc(sizeof(char) * STRMAX);
        if(!sym->name)
        {
            free(sym);
            return NULL;
        }

        strncpy(sym->name, name, STRMAX);
    }

    return sym;
}

const char* symbol_codegen(struct symbol* symbol)
{
    if(!symbol) { return NULL; }
    char* buf = malloc(sizeof(char) * STRMAX);
    if(!buf) { return NULL; }
 
    if(symbol->kind == SYMBOL_GLOBAL)
        snprintf(buf, STRMAX, "%s(%%rip)", symbol->name);
    else
        snprintf(buf, STRMAX, "-%d(%%rbp)", (symbol->which + 1) * 8);

    return buf;
}

struct symbol* symbol_copy(struct symbol* symbol)
{
    if(!symbol) return NULL;

    struct symbol* sym = malloc(sizeof(struct symbol)); 
    if(sym)
    {
        sym->name = malloc(sizeof(char) * STRMAX);
        if(!sym->name)
        {
            free(sym);
            return NULL;
        }

        strncpy(sym->name, symbol->name, STRMAX);
        sym->kind = symbol->kind;
        sym->type = type_copy(symbol->type);
        sym->which = symbol->which;
    }

    return sym;
}

bool symbol_equal(struct symbol* a, struct symbol* b)
{
    if(!a && !b) return true;
    if((a && !b) || (!a && b)) return false;

    if(a->kind == b->kind && type_equals(a->type, b->type) && strncmp(a->name, b->name, STRMAX) == 0 && a->which == b->which)
        return true;

    return false;
}

void scope_enter(void)
{
    if(!scope_stack)
        scope_stack = stack_create();

    struct hash_table* table = hash_table_create();

    stack_push(scope_stack, (void*)table);
}

void scope_exit(void)
{
    struct hash_table* table = (struct hash_table*)stack_pop(scope_stack);
    hash_table_destroy(&table);

    if(stack_size(scope_stack) == 0)
    {
        stack_destroy(&scope_stack);
        scope_stack = NULL;
    }
}

int scope_level(void)
{
    return stack_size(scope_stack);
}

void scope_bind(const char* name, struct symbol* sym)
{
    struct symbol* temp = scope_lookup_current(name);
    if(temp && strncmp(temp->name, sym->name, STRMAX) == 0 && !symbol_equal(sym, temp))
        return;

    int index = stack_size(scope_stack) - 1;
    hash_table_insert(stack_item(scope_stack, index), name, (void*)sym);
}

struct symbol* scope_lookup(const char* name)
{
    if(!name) return NULL;

    for(int i = stack_size(scope_stack) - 1; i >= 0; i--)
    {
        struct hash_table* ht = (struct hash_table*)stack_item(scope_stack, i);
        if(!ht || !ht->arr)
            continue;

        for(int j = 0; j < ht->capacity; j++)
        {
            if(!ht->arr[j] || !ht->arr[j]->key)
                continue;

            if(strncmp(name, ht->arr[j]->key, STRMAX) == 0)
                return (struct symbol*)ht->arr[j]->value;
        }
    }
    return NULL;
}

struct symbol* scope_lookup_current(const char* name)
{
    if(!name) return NULL;

    int index = stack_size(scope_stack) - 1;

    struct hash_table* ht = (struct hash_table*)stack_item(scope_stack, index);
    if(ht && ht->arr)
    {
        for(int i = 0; i < ht->capacity; i++)
        {
            if(!ht->arr[i] || !ht->arr[i]->key)
                continue;

            if(strncmp(name, ht->arr[i]->key, STRMAX) == 0)
                return (struct symbol*)ht->arr[i]->value;
        }
    }
    return NULL; 
}

void symbol_print(struct symbol* sym)
{
    if(!sym) return;

    printf("Name: %s\n", sym->name);

    switch(sym->kind) {
    case SYMBOL_GLOBAL:
        printf("Global Symbol\n");
        break;
    case SYMBOL_PARAM:
        printf("Parameter Symbol\n");
        break;
    case SYMBOL_LOCAL:
        printf("Local Symbol\n");
        break;
    }

    printf("Position: %d\n", sym->which);

    type_print(sym->type);
}

void symbol_destroy(struct symbol** sym)
{
    if(sym && *sym)
    {
        struct symbol* symbol = *sym;

        type_destroy(&symbol->type);
        free(symbol->name);
        free(symbol);
        *sym = NULL;
    }
}

