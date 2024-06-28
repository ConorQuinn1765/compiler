#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "decl.h"
#include "expr.h"
#include "param_list.h"
#include "stmt.h"
#include "symbol.h"
#include "type.h"

static int pushLocalVarsToStack(struct decl* pDecl);
static int  countDeclarations(struct stmt* pStmt);
static void moveParamsToStack(struct param_list* pParams);

struct decl* decl_create(char* name, struct type* type, struct expr* value, struct stmt* code, struct decl* next)
{
    struct decl* pDecl = malloc(sizeof(struct decl));
    if(pDecl)
    {
        pDecl->name = name;
        pDecl->type = type;
        pDecl->value = value;
        pDecl->code = code;
        pDecl->symbol = NULL;
        pDecl->next = next;
    }

    return pDecl;
}

void decl_resolve(struct decl* pDecl)
{
    if(!pDecl) return;

    symbol_t kind = scope_level() > 1 ? SYMBOL_LOCAL : SYMBOL_GLOBAL;
    if(pDecl->type->kind == TYPE_AUTO)
    {
        struct type* temp = expr_typecheck(pDecl->value);
        type_destroy(&pDecl->type);
        pDecl->type = temp;
    }

    pDecl->symbol = symbol_create(kind, pDecl->type, pDecl->name);
    scope_bind(pDecl->name, pDecl->symbol);

    expr_resolve(pDecl->value);

    if(pDecl->code)
    {
        scope_enter();
        param_list_resolve(pDecl->type->params);
        stmt_resolve(pDecl->code);
        scope_exit();
    }

    decl_resolve(pDecl->next);
}

void decl_typecheck(struct decl* pDecl)
{
    if(!pDecl) return;

    if(pDecl->type->kind == TYPE_ARRAY)
    {
        if(pDecl->type->value == NULL)
        {
            printf("type error: arrays must be declared with fixed size - ");
            decl_print(pDecl);
        }
    }

    if(pDecl->value)
    {
        struct type* t = expr_typecheck(pDecl->value);
        if(pDecl->type->kind == TYPE_AUTO)
        {
            type_destroy(&pDecl->type);
            type_destroy(&pDecl->symbol->type);
            pDecl->type = type_copy(t);
            pDecl->symbol->type = type_copy(t);
        }
        else if(!type_equals(t, pDecl->type))
        {
            printf("type error: cannot assign an expression of type ");
            type_print(t); printf(" (");
            expr_print(pDecl->value);
            printf(") to a variable of type ");
            type_print(pDecl->type); printf(" (%s)\n", pDecl->name);
        }
        type_destroy(&t);
    }

    if(pDecl->code)
    {
        struct symbol* sym = scope_lookup(pDecl->name);
        param_list_typecheck(pDecl->type->params, sym->type->params);
        stmt_typecheck(pDecl->code, sym);
    }

    decl_typecheck(pDecl->next);
}

void decl_codegen(struct decl* pDecl, struct scratch regs[])
{
    if(!pDecl) return;

    if(pDecl->code)
    {
        printf("############################\n");
        printf(".text\n");
        printf("%s:\n", pDecl->name);

        struct scratch funcRegs[] = {
            {0, false, "%rbx"},
            {1, false, "%r10"},
            {2, false, "%r11"},
            {3, false, "%r12"},
            {4, false, "%r13"},
            {5, false, "%r14"},
            {6, false, "%r15"}
        };

        printf("PUSHQ %%rbp\n");
        printf("MOVQ  %%rsp, %%rbp\n");

        pushLocalVarsToStack(pDecl);

        printf("PUSHQ %%rbx\n");
        printf("PUSHQ %%r12\n");
        printf("PUSHQ %%r13\n");
        printf("PUSHQ %%r14\n");
        printf("PUSHQ %%r15\n");

        printf("\n############################\n\n");

        stmt_codegen(pDecl->code, pDecl, funcRegs, pDecl->symbol);

        printf("\n############################\n\n");
        printf(".%s_epilouge:\n", pDecl->name);

        printf("POPQ %%r15\n");
        printf("POPQ %%r14\n");
        printf("POPQ %%r13\n");
        printf("POPQ %%r12\n");
        printf("POPQ %%rbx\n");

        printf("MOVQ %%rbp, %%rsp\n");
        printf("POPQ %%rbp\n");
        printf("RET\n");
    }
    else
    {
        if(pDecl->symbol->kind == SYMBOL_GLOBAL)
        {
            const char* label;
            switch(pDecl->type->kind)
            {
                case TYPE_BOOL:
                case TYPE_CHAR:
                case TYPE_INTEGER:
                    printf(".data\n");
                    printf("%s: ", pDecl->name);
                    printf(".quad %d\n", pDecl->value ? pDecl->value->integer_value : 0);
                    break;
                case TYPE_STRING:
                    printf(".data\n");
                    label = label_name(label_create());
                    printf("%s: ", label);
                    printf(".string \"%s\"\n",  pDecl->value ? pDecl->value->string_literal : "");
                    printf("%s: .quad %s\n", pDecl->name, label);
                    free((void*)label);
                    break;
                case TYPE_ARRAY:
                    printf(".data\n");
                    expr_codegen(pDecl->value, pDecl, regs, 0, true);
                    break;
                default:
                    break;
            }
        }
        else
        {
            expr_codegen(pDecl->value, pDecl, regs, pDecl->symbol->which, false);
            if(pDecl->value && pDecl->value->reg != -1)
            {
                const char* sym_code = symbol_codegen(pDecl->symbol);
                printf("MOVQ %s, %s\n", scratch_name(regs, pDecl->value->reg), sym_code);
                scratch_free(regs, pDecl->value->reg);
                free((void*)sym_code);
            }
        }
    }

    decl_codegen(pDecl->next, regs);
}

void decl_print(struct decl* pDecl)
{
    if(pDecl)
    {
        printf("%s:", pDecl->name);
        type_print(pDecl->type);

        if(pDecl->type->kind == TYPE_FUNCTION)
        {
            if(pDecl->code)
            {
                printf(" = ");
                stmt_print(pDecl->code, 1);

            }
            else
                printf(";");
        }
        else
        {
            if(pDecl->value)
            {
                printf(" = ");
                expr_print(pDecl->value);
            }

            printf(";");
        }

        printf("\n");
        decl_print(pDecl->next);
    }
}

void decl_destroy(struct decl** ppDecl)
{
    if(ppDecl && *ppDecl)
    {
        struct decl* pDecl = *ppDecl;
        decl_destroy(&pDecl->next);

        type_destroy(&pDecl->type);
        expr_destroy(&pDecl->value);
        stmt_destroy(&pDecl->code);
        symbol_destroy(&pDecl->symbol);

        free(pDecl->name);
        free(pDecl);

        *ppDecl = NULL;
    }
}

static int pushLocalVarsToStack(struct decl* pDecl)
{
    if(!pDecl) return 0;

    int count = 0;
    struct param_list* pParams = pDecl->type->params;
    while(pParams)
    {
        count++;
        pParams = pParams->next;
    }

    if(pDecl->code->kind == STMT_BLOCK)
        count += countDeclarations(pDecl->code->body);
    else
        count += countDeclarations(pDecl->code);

    moveParamsToStack(pDecl->type->params);

    if(count != 0)
        printf("SUBQ $%d, %%rsp\n", (count + 5) % 2 == 0 ? count * 8 : (count + 1) * 8);


    return count;
}

static int countDeclarations(struct stmt* pStmt)
{
    if(!pStmt) return 0;

    int count = 0;
    struct decl* temp = pStmt->decl;
    while(temp != NULL)
    {
        if(temp->symbol->type->kind == TYPE_ARRAY)
            count += temp->symbol->type->value->integer_value;
        else
            count++;

        temp = temp->next;
    }

    return count + countDeclarations(pStmt->next);
}

static void moveParamsToStack(struct param_list* pParams)
{
    if(!pParams) return;

    int count = 0;
    char* regs[] = {"%rdi", "%rsi", "%rdx", "%rcx", "%r8", "%r9"};

    while(pParams)
    {
        printf("PUSHQ %s\n", regs[count++]);
        pParams = pParams->next;
    }
}
