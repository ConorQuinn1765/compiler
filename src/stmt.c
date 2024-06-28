#include <stdio.h>
#include <stdlib.h>
#include "expr.h"
#include "stmt.h"
#include "decl.h"
#include "type.h"

static void print_tab(void);

struct stmt* stmt_create(stmt_t kind, struct decl* decl, struct expr* init_expr, struct expr* expr,
                struct expr* next_expr, struct stmt* body, struct stmt* else_body, struct stmt* next)
{
    struct stmt* pStmt = malloc(sizeof(struct stmt));
    if(pStmt)
    {
        pStmt->kind = kind;
        pStmt->decl = decl;
        pStmt->init_expr = init_expr;
        pStmt->expr = expr;
        pStmt->next_expr = next_expr;
        pStmt->body = body;
        pStmt->else_body = else_body;
        pStmt->next = next;
    }

    return pStmt;
}

void stmt_resolve(struct stmt* pStmt)
{
    if(!pStmt) return;

    decl_resolve(pStmt->decl);

    expr_resolve(pStmt->init_expr);
    expr_resolve(pStmt->expr);
    expr_resolve(pStmt->next_expr);

    if(pStmt->body)
    {
        scope_enter();
        stmt_resolve(pStmt->body);
        scope_exit();
    }
    if(pStmt->else_body)
    {
        scope_enter();
        stmt_resolve(pStmt->else_body);
        scope_exit();
    }

    stmt_resolve(pStmt->next);
}

void stmt_typecheck(struct stmt* pStmt, struct symbol* symbol)
{
    if(!pStmt) return;

    struct type* type = NULL;
    switch(pStmt->kind)
    {
        case STMT_DECL:
            decl_typecheck(pStmt->decl);
            break;
        case STMT_EXPR:
            type = expr_typecheck(pStmt->expr);
            type_destroy(&type);
            break;
        case STMT_IF_ELSE:
            type = expr_typecheck(pStmt->expr);
            if(!type || type->kind != TYPE_BOOL)
            {
                printf("type error: if statement requires a boolean condition (");
                expr_print(pStmt->expr); printf(")\n");
            }
            type_destroy(&type);

            break;
        case STMT_FOR:
            type = expr_typecheck(pStmt->expr);
            if(type && type->kind != TYPE_BOOL)
            {
                printf("type error: for statement requires a boolean conditional expression (");
                expr_print(pStmt->expr); printf(")\n");
            }
            type_destroy(&type);

            break;
        case STMT_PRINT:
            type = expr_typecheck(pStmt->expr);

            if(type && (type->kind == TYPE_FUNCTION || type->kind == TYPE_ARRAY))
            {
                printf("type error: print statements must be a list of atomic types (");
                expr_print(pStmt->expr); printf(")\n");
            }

            type_destroy(&type);
            break;
        case STMT_RETURN:
            type = expr_typecheck(pStmt->expr);
            if(type && (type->kind == TYPE_FUNCTION || type->kind == TYPE_ARRAY))
            {
                printf("type error: functions must return an atomic type or void\n");
                expr_print(pStmt->expr); printf("\n");
            }

            if(!symbol)
            {
                fprintf(stderr, "type error: no return type to compare against\n");
            }
            else if((!type_equals(type, symbol->type->subtype)))
            {
                printf("type error: mismatched return type in function %s. Expected type (", symbol->name);
                type_print(symbol->type->subtype); printf("), Actual type (");
                type_print(type); printf(")\n");
                printf("  - return "); expr_print(pStmt->expr); printf(";\n");
            }

            type_destroy(&type);
            break;
        case STMT_BLOCK:
            stmt_typecheck(pStmt->body, symbol);
            break;
        default:
            printf("error: invalid statement kind\n");
            stmt_print(pStmt, 0);
            break;
    }

    stmt_typecheck(pStmt->next, symbol);
}

void stmt_codegen(struct stmt *pStmt, struct decl* pDecl, struct scratch regs[], struct symbol* sym)
{
    if(!pStmt) return;

    char *falseLabel, *doneLabel, *topLabel;
    struct type* type = NULL;

    switch(pStmt->kind)
    {
        case STMT_DECL:
            decl_codegen(pStmt->decl, regs);
            break;
        case STMT_EXPR:
            expr_codegen(pStmt->expr, pDecl, regs, 0, NULL);
            scratch_free(regs, pStmt->expr->reg);
            break;
        case STMT_IF_ELSE:
            falseLabel = label_name(label_create());
            doneLabel = label_name(label_create());

            expr_codegen(pStmt->expr, pDecl, regs, 0, NULL);
            printf("CMPQ $0, %s\n", scratch_name(regs, pStmt->expr->reg));
            scratch_free(regs, pStmt->expr->reg);
            
            printf("JE %s\n", falseLabel);
            stmt_codegen(pStmt->body, pDecl, regs, sym);
            printf("JMP %s\n", doneLabel);

            printf("%s:\n", falseLabel);
            stmt_codegen(pStmt->else_body, pDecl, regs, sym);
            printf("%s:\n", doneLabel);

            free(falseLabel);
            free(doneLabel);
            break;
        case STMT_FOR:
            topLabel = label_name(label_create());
            doneLabel = label_name(label_create());

            if(pStmt->init_expr)
                expr_codegen(pStmt->init_expr, pDecl, regs, 0, NULL);

            printf("%s:\n", topLabel);
            if(pStmt->expr)
            {
                expr_codegen(pStmt->expr, pDecl, regs, 0, NULL);
                printf("CMPQ $0, %s\n", scratch_name(regs, pStmt->expr->reg));
                printf("JE %s\n", doneLabel);
            }

            stmt_codegen(pStmt->body, pDecl, regs, sym);

            if(pStmt->next_expr)
                expr_codegen(pStmt->next_expr, pDecl, regs, 0, NULL);

            printf("JMP %s\n", topLabel);
            printf("%s:\n", doneLabel);

            if(pStmt->init_expr)
                scratch_free(regs, pStmt->init_expr->reg);
            if(pStmt->expr)
                scratch_free(regs, pStmt->expr->reg);
            if(pStmt->next_expr)
                scratch_free(regs, pStmt->next_expr->reg);

            free(topLabel);
            free(doneLabel);
            break;
        case STMT_PRINT:
            if(pStmt->expr)
            {
                struct expr* e = pStmt->expr;
                while(e)
                {
                    expr_codegen(e->left, pDecl, regs, 0, NULL);
                    e->reg = e->left->reg;
                    type = expr_typecheck(e);

                    printf("PUSHQ %%r10\n");
                    printf("PUSHQ %%r11\n");

                    switch(type->kind)
                    {
                        case TYPE_BOOL:
                            printf("MOVQ %s, %%rdi\n", scratch_name(regs, e->reg));
                            printf("CALL printBool\n");
                            break;
                        case TYPE_CHAR:
                            printf("MOVQ %s, %%rdi\n", scratch_name(regs, e->reg));
                            printf("CALL printChar\n");
                            break;
                        case TYPE_INTEGER:
                            printf("MOVQ %s, %%rdi\n", scratch_name(regs, e->reg));
                            printf("CALL printInt\n");
                            break;
                        case TYPE_STRING:
                            printf("MOVQ %s, %%rdi\n", scratch_name(regs, e->reg));
                            printf("CALL printString\n");
                            break;
                        default:
                            printf("[ERROR] - Non printable type passed to print %s\n", type_string(type));
                            break;
                    }

                    printf("POPQ %%r11\n");
                    printf("POPQ %%r10\n");

                    scratch_free(regs, e->reg);
                    e = e->right;
               }
            }
            break;
        case STMT_RETURN:
            expr_codegen(pStmt->expr, pDecl, regs, 0, NULL);
            if(pDecl->type->subtype->kind != TYPE_VOID)
                printf("MOVQ %s, %%rax\n", scratch_name(regs, pStmt->expr->reg));
            printf("JMP .%s_epilouge\n", pDecl->name);
            scratch_free(regs, pStmt->expr->reg);
            break;
        case STMT_BLOCK:
            stmt_codegen(pStmt->body, pDecl, regs, sym);
            break;
        default:
            printf("error: invalid statement kind\n");
            stmt_print(pStmt, 0);
            break;
    }

    stmt_codegen(pStmt->next, pDecl, regs, sym);
}

void stmt_print(struct stmt* pStmt, int depth)
{
    if(pStmt)
    { 
        switch(pStmt->kind)
        {
            case STMT_DECL:
                for(int i = 0; i < depth; i++)
                    print_tab();

                decl_print(pStmt->decl);
                break;
            case STMT_EXPR:
                for(int i = 0; i < depth; i++)
                    print_tab();

                expr_print(pStmt->expr);
                printf(";");
                break;
            case STMT_PRINT:
                for(int i = 0; i < depth; i++)
                    print_tab();

                printf("print ");
                expr_print(pStmt->expr);
                printf(";");
                break;
            case STMT_FOR:
                for(int i = 0; i < depth; i++)
                    print_tab();

                printf("for(");
                expr_print(pStmt->init_expr);   printf("; ");
                expr_print(pStmt->expr);        printf("; ");
                expr_print(pStmt->next_expr);

                if(pStmt->body && pStmt->body->kind == STMT_BLOCK)
                    printf(") ");
                else
                    printf(")\n");

                stmt_print(pStmt->body, depth + 1); 
                break;
            case STMT_IF_ELSE:
                for(int i = 0; i < depth; i++)
                    print_tab();

                printf("if(");
                expr_print(pStmt->expr);
                
                if(pStmt->body && pStmt->body->kind == STMT_BLOCK)
                    printf(") ");
                else
                    printf(")\n");

                stmt_print(pStmt->body, depth + 1);

                if(pStmt->else_body)
                {
                    print_tab();
                    printf("else ");
                    stmt_print(pStmt->else_body, depth + 1);
                }

                break;
            case STMT_BLOCK:
                printf("{\n");
                stmt_print(pStmt->body, depth);

                for(int i = 0; i < depth - 1; i++)
                    print_tab();

                printf("}");
                break;
            case STMT_RETURN:
                for(int i = 0; i < depth; i++)
                    print_tab();

                printf("return ");
                expr_print(pStmt->expr);
                printf(";");
                break;
        }

        printf("\n");
        stmt_print(pStmt->next, depth);
    }
}

void stmt_destroy(struct stmt** ppStmt)
{
    if(ppStmt && *ppStmt)
    {
        struct stmt* pStmt = *ppStmt;
        decl_destroy(&pStmt->decl);
        
        expr_destroy(&pStmt->init_expr);
        expr_destroy(&pStmt->expr);
        expr_destroy(&pStmt->next_expr);

        stmt_destroy(&pStmt->body);
        stmt_destroy(&pStmt->else_body);
        stmt_destroy(&pStmt->next);

        free(pStmt);
        *ppStmt = NULL;
    }
}

static void print_tab(void)
{
    printf("    ");
}

