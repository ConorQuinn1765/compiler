#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "expr.h"
#include "param_list.h"
#include "register.h"
#include "symbol.h"
#include "vector.h"
#include "type.h"

static void print_operator(struct expr* pExpr);
static void unclean_string(const char* input, char* output);
static int init_list_typecheck(struct type* base, struct expr* list);
static void moveParamsToRegs(struct expr* pExpr, struct decl* pDecl, struct scratch regs[], int offset, bool isGlobal);
static bool stringCmp(void* a, void* b);
static void stringFree(void** item);

const int STRMAX = 128;

struct expr* expr_create(expr_t kind, struct expr* left, struct expr* right)
{
    struct expr* pExpr = malloc(sizeof(struct expr));
    if(pExpr)
    {
        pExpr->kind = kind;
        pExpr->left = left;
        pExpr->right = right;
        pExpr->integer_value = 0;
        pExpr->name = NULL;
        pExpr->string_literal = NULL;
        pExpr->symbol = NULL;
        pExpr->reg = -1;
    }

    return pExpr;
}

struct expr* expr_create_name( const char *name )
{
    struct expr* pExpr = malloc(sizeof(struct expr));
    if(pExpr)
    {
        pExpr->kind = EXPR_NAME;
        pExpr->name = name;

        pExpr->left = NULL;
        pExpr->right = NULL;
        pExpr->integer_value = 0;
        pExpr->string_literal = NULL;
        pExpr->symbol = NULL;
        pExpr->reg = -1;
    }
    
    return pExpr;
}

struct expr* expr_create_integer_literal( int i )
{
    struct expr* pExpr = malloc(sizeof(struct expr));
    if(pExpr)
    {
        pExpr->kind = EXPR_INT_LITERAL;
        pExpr->integer_value = i;

        pExpr->left = NULL;
        pExpr->right = NULL;
        pExpr->name = NULL;
        pExpr->string_literal = NULL;
        pExpr->symbol = NULL;
        pExpr->reg = -1;
   }
    
    return pExpr;
}

struct expr* expr_create_boolean_literal( int b )
{
    struct expr* pExpr = malloc(sizeof(struct expr));
    if(pExpr)
    {
        pExpr->kind = EXPR_BOOL_LITERAL;
        pExpr->integer_value = b;

        pExpr->left = NULL;
        pExpr->right = NULL;
        pExpr->name = NULL;
        pExpr->string_literal = NULL;
        pExpr->symbol = NULL;
        pExpr->reg = -1;
   }
    
    return pExpr;
}

struct expr* expr_create_char_literal( char c )
{
    struct expr* pExpr = malloc(sizeof(struct expr));
    if(pExpr)
    {
        pExpr->kind = EXPR_CHAR_LITERAL;
        pExpr->integer_value = c;

        pExpr->left = NULL;
        pExpr->right = NULL;
        pExpr->name = NULL;
        pExpr->string_literal = NULL;
        pExpr->symbol = NULL;
        pExpr->reg = -1;
   }
    
    return pExpr;
}

struct expr* expr_create_string_literal( const char *str )
{
    struct expr* pExpr = malloc(sizeof(struct expr));
    if(pExpr)
    {
        pExpr->kind = EXPR_STRING_LITERAL;
        pExpr->string_literal = malloc(sizeof(char) * strlen(str) + 1);
        if(!pExpr->string_literal)
        {
            fprintf(stderr, "[ERROR] expr_create_string_literal - Failed to allocate space for string literal\n");
            free(pExpr);
            return NULL;
        }
        strncpy((char*)pExpr->string_literal, str, strlen(str) + 1);

        pExpr->left = NULL;
        pExpr->right = NULL;
        pExpr->name = NULL;
        pExpr->integer_value = 0;
        pExpr->symbol = NULL;
        pExpr->reg = -1;
   }
    
    return pExpr;
}

struct expr* expr_copy(struct expr* expr)
{
    if(!expr) return NULL;

    struct expr* pExpr = malloc(sizeof(struct expr));
    if(pExpr)
    {
        pExpr->name = NULL;
        if(expr->name)
        {
            pExpr->name = malloc(sizeof(char) * STRMAX);
            if(!pExpr->name)
            {
                free(pExpr);
                return NULL;
            }
        }

        pExpr->string_literal = NULL;
        if(expr->string_literal)
        {
            pExpr->string_literal = malloc(sizeof(char) * STRMAX);
            if(!pExpr->string_literal)
            {
                if(pExpr->name)
                    free((char*)pExpr->name);
                free(pExpr);
                return NULL;
            }
        }

        pExpr->kind = expr->kind;
        pExpr->left = expr_copy(expr->left);
        pExpr->right = expr_copy(expr->right);
        pExpr->integer_value = expr->integer_value;
        pExpr->symbol = expr->symbol;
        pExpr->reg = expr->reg;
    }
    
    return pExpr;

}

void expr_resolve(struct expr* pExpr)
{
    if(!pExpr) return;

    if(pExpr->kind == EXPR_NAME)
    {
        pExpr->symbol = scope_lookup(pExpr->name);
        if(!pExpr->symbol)
        {
            printf("resolve error: %s is not defined\n", pExpr->name);
        }
    }
    else
    {
        expr_resolve(pExpr->left);
        expr_resolve(pExpr->right);
    }
}

struct type* expr_typecheck(struct expr* pExpr)
{
    if(!pExpr) return NULL;

    struct type* lt = expr_typecheck(pExpr->left);
    struct type* rt = expr_typecheck(pExpr->right);
    struct type* type = NULL;
    struct symbol* symbol = NULL;
    int size = 0;

    switch(pExpr->kind)
    {
        case EXPR_CHAR_LITERAL:
            type = type_create(TYPE_CHAR, 0, 0, 0);
            break;
        case EXPR_BOOL_LITERAL:
            type = type_create(TYPE_BOOL, 0, 0, 0);
            break;
        case EXPR_STRING_LITERAL:
            type = type_create(TYPE_STRING, 0, 0, 0);
            break;
        case EXPR_INT_LITERAL:
            type = type_create(TYPE_INTEGER, 0, 0, 0);
            break;
        case EXPR_NAME:
            if(!pExpr->symbol)
            {
                symbol = scope_lookup(pExpr->name);
                pExpr->symbol = symbol;
            }
            else
                symbol = pExpr->symbol;

            type = symbol ? type_copy(symbol->type) : NULL;
            break;
        case EXPR_ASSIGN:
            if(!type_equals(lt, rt))
            {
                printf("type error: Cannot assign an expression of type ");
                type_print(rt); printf(" ("); expr_print(pExpr->right);
                printf(") to a variable of type ");
                type_print(lt); printf(" ("); expr_print(pExpr->left);
                printf(")\n");
            }

            type = type_copy(lt);
            break;
        case EXPR_OR:
        case EXPR_AND:
            if(!type_equals(lt, rt) || (lt && lt->kind != TYPE_BOOL))
            {
                printf("type error: Logical operators require boolean operands\n");
                printf("("); expr_print(pExpr); printf(")\n");
            }

            type = type_copy(lt);
            break;
        case EXPR_EQ:
        case EXPR_NE:
            if(!type_equals(lt, rt))
            {
                printf("type error: type mismatch. Cannot compare type ");
                type_print(lt); printf(" to type "); type_print(rt);
                printf(" ("); expr_print(pExpr); printf(")\n");
            }
            if(lt && rt && (lt->kind == TYPE_VOID || lt->kind == TYPE_FUNCTION || lt->kind == TYPE_ARRAY ||
                rt->kind == TYPE_VOID || rt->kind == TYPE_FUNCTION || rt->kind == TYPE_ARRAY))
            {
                printf("type error: Equality operators require boolean operands.\n");
                expr_print(pExpr); printf("\n");
            }

            type = type_create(TYPE_BOOL, 0, 0, 0);
            break;
        case EXPR_LT:
        case EXPR_LE:
        case EXPR_GT:
        case EXPR_GE:
            if(!type_equals(lt, rt) || (lt && lt->kind != TYPE_INTEGER))
            {
                printf("type error: Comparison operators require integer operands. Cannot compare type ");
                type_print(lt); printf(" to type "); type_print(rt);
                printf(" ("); expr_print(pExpr); printf(")\n");
            }

            type = type_create(TYPE_BOOL, 0, 0, 0);
            break;
        case EXPR_ADD:
        case EXPR_SUB:
        case EXPR_MUL:
        case EXPR_DIV:
        case EXPR_MOD:
        case EXPR_EXPONENT:
            if(!type_equals(lt, rt) || (lt && lt->kind != TYPE_INTEGER))
            {
                printf("type error: Arithmetic operators require integer operands. Cannot use type ");
                type_print(lt); printf(" with type "); type_print(rt);
                printf(" ("); expr_print(pExpr); printf(")\n");
            }

            type = type_create(TYPE_INTEGER, 0, 0, 0);
            break;
        case EXPR_INC:
        case EXPR_DEC:
            if(lt && lt->kind != TYPE_INTEGER)
            {
                printf("type error: Increment operators require an integer operand\n");
                expr_print(pExpr); printf("\n");
            }

            type = type_create(TYPE_INTEGER, 0, 0, 0);
            break;
        case EXPR_SUBSCRIPT:
            if(lt && rt && ((lt->kind != TYPE_ARRAY && lt->kind != TYPE_STRING) || rt->kind != TYPE_INTEGER))
            {
                printf("type error: The derefernce operator requires an array/string type and an integer type as operands - ");
                expr_print(pExpr); printf("\n");
            }

            if(lt->kind == TYPE_STRING)
                type = type_create(TYPE_CHAR, 0, 0, 0);
            else
                type = lt ? type_copy(lt->subtype) : NULL;
            break;
        case EXPR_CALL:
            symbol = scope_lookup(pExpr->left->name);
            if(!symbol)
            {
                printf("resolve error: Attempt to use undeclared function - ");
                expr_print(pExpr); printf("\n");
                break;
            }

            param_list_typecheck_call(symbol->type->params, pExpr->right);
            type = lt ? type_copy(lt->subtype) : NULL;
            break;
        case EXPR_ARG:
            type = type_copy(lt);
            break;
        case EXPR_GROUP:
            type = type_copy(lt);
            break;
        case EXPR_INIT_LIST:
            size = init_list_typecheck(lt, pExpr);

            type = type_create(TYPE_ARRAY, type_copy(lt), 0, expr_create_integer_literal(size));
            break;
        case EXPR_NOT:
            if(lt->kind != TYPE_BOOL)
            {
                printf("type error: The not operator requires a boolean operand\n");
                expr_print(pExpr); printf("\n");
            }

            type = type_create(TYPE_BOOL, 0, 0, 0); 
            break;
        case EXPR_UNARY_MINUS:
            if(lt && lt->kind != TYPE_INTEGER)
            {
                printf("type error: The unary minus operator requires an integer operand\n");
                expr_print(pExpr); printf("\n");
            }

            type = type_create(TYPE_INTEGER, 0, 0, 0); 
            break;
    default:
        printf("error: invalid expression kind - %d\n", pExpr->kind);
        expr_print(pExpr); printf("\n");
        break;
    }

    type_destroy(&lt);
    type_destroy(&rt);

    return type;
}

void expr_codegen(struct expr *pExpr, struct decl *pDecl, struct scratch regs[], int offset, bool isGlobal)
{
    if(!pExpr) return;

    int r1 = -1, r2 = -1, count = 0;
    char *loop, *done, *t1;
    const char* sym_code;
    const char *l1, *l2;
    struct expr* temp;
    struct type* type;
    type_t kind;
    Vector* vec;
    char outBuf[256] = {0};

    switch(pExpr->kind)
    {
        case EXPR_CHAR_LITERAL:
        case EXPR_BOOL_LITERAL:
        case EXPR_INT_LITERAL:
            pExpr->reg = scratch_alloc(regs);
            printf("MOVQ $%d, %s\n",
                    pExpr->integer_value, scratch_name(regs, pExpr->reg));
            break;
        case EXPR_STRING_LITERAL:
            pExpr->reg = scratch_alloc(regs);
            l1 = label_name(label_create());
            l2 = label_name(label_create());
            unclean_string(pExpr->string_literal, outBuf);
           
            printf(".data\n");
            printf("%s: .string \"%s\"\n", l1, outBuf);
            printf("%s: .quad %s\n", l2, l1);
            printf(".text\n");
            printf("MOVQ %s(%%rip), %s\n", l2, scratch_name(regs, pExpr->reg));
           
            free((void*)l1);
            free((void*)l2);
            break;
        case EXPR_NAME:
            pExpr->reg = scratch_alloc(regs);
            sym_code = symbol_codegen(pExpr->symbol);
            printf("MOVQ %s, %s\n",
                sym_code, scratch_name(regs, pExpr->reg));

            free((void*)sym_code);
            break;
        case EXPR_ASSIGN:
            expr_codegen(pExpr->right, pDecl, regs, offset, isGlobal);

            sym_code = symbol_codegen(pExpr->left->symbol);

            printf("MOVQ %s, %s\n",
                    scratch_name(regs, pExpr->right->reg), sym_code);

            pExpr->reg = pExpr->right->reg;
            scratch_free(regs, pExpr->left->reg);
            free((void*)sym_code);
            break;
        case EXPR_OR:
            expr_codegen(pExpr->left, pDecl, regs, offset, isGlobal);
            expr_codegen(pExpr->right, pDecl, regs, offset, isGlobal);

            t1 = label_name(label_create());
            done = label_name(label_create());

            printf("CMPQ $1, %s\n", scratch_name(regs, pExpr->left->reg));
            printf("JE %s\n", t1);
            printf("CMPQ $1, %s\n", scratch_name(regs, pExpr->right->reg));
            printf("JE %s\n", t1);
            printf("MOVQ $0, %s\n", scratch_name(regs, pExpr->right->reg));
            printf("JMP %s\n", done);
            printf("%s:", t1);
            printf("MOVQ $1, %s\n", scratch_name(regs, pExpr->right->reg));
            printf("%s:", done);

            pExpr->reg = pExpr->right->reg;
            scratch_free(regs, pExpr->left->reg);
            free(t1);
            free(done);
            break;
        case EXPR_AND:
            expr_codegen(pExpr->left, pDecl, regs, offset, isGlobal);
            expr_codegen(pExpr->right, pDecl, regs, offset, isGlobal);

            t1 = label_name(label_create());
            done = label_name(label_create());

            printf("CMPQ $1, %s\n", scratch_name(regs, pExpr->left->reg));
            printf("JNE %s\n", t1);
            printf("CMPQ $1, %s\n", scratch_name(regs, pExpr->right->reg));
            printf("JNE %s\n", t1);
            printf("MOVQ $1, %s\n", scratch_name(regs, pExpr->right->reg));
            printf("JMP %s\n", done);
            printf("%s:", t1);
            printf("MOVQ $0, %s\n", scratch_name(regs, pExpr->right->reg));
            printf("%s:", done);

            pExpr->reg = pExpr->right->reg;
            scratch_free(regs, pExpr->left->reg);

            free(t1);
            free(done);
            break;
        case EXPR_EQ:
            expr_codegen(pExpr->left, pDecl, regs, offset, isGlobal);
            expr_codegen(pExpr->right, pDecl, regs, offset, isGlobal);
            type = expr_typecheck(pExpr->left);

            if(type->kind == TYPE_STRING)
            {
                 printf("MOVQ %s, %%rdi\n", scratch_name(regs, pExpr->left->reg)); 
                 printf("MOVQ %s, %%rsi\n", scratch_name(regs, pExpr->right->reg));
                 printf("CALL stringCompare\n");
                 printf("CMPQ $0, %%rax\n");
            }
            else
            {
                printf("CMPQ %s, %s\n", scratch_name(regs, pExpr->left->reg),
                                        scratch_name(regs, pExpr->right->reg));
            }

            t1 = label_name(label_create());
            done = label_name(label_create());

            printf("JNE %s\n", t1);
            printf("MOVQ $1, %s\n", scratch_name(regs, pExpr->right->reg));
            printf("JMP %s\n", done);
            printf("%s:\n", t1);
            printf("MOVQ $0, %s\n", scratch_name(regs, pExpr->right->reg));
            printf("%s:\n", done);

            pExpr->reg = pExpr->right->reg;
            scratch_free(regs, pExpr->left->reg);
            free(t1);
            free(done);
            free(type);
            break;
        case EXPR_NE:
            expr_codegen(pExpr->left, pDecl, regs, offset, isGlobal);
            expr_codegen(pExpr->right, pDecl, regs, offset, isGlobal);

            t1 = label_name(label_create());
            done = label_name(label_create());

            printf("CMPQ %s, %s\n", scratch_name(regs, pExpr->left->reg), scratch_name(regs, pExpr->right->reg));
            printf("JE %s\n", t1);
            printf("MOVQ $1, %s\n", scratch_name(regs, pExpr->right->reg));
            printf("JMP %s\n", done);
            printf("%s:\n", t1);
            printf("MOVQ $0, %s\n", scratch_name(regs, pExpr->right->reg));
            printf("%s:\n", done);

            pExpr->reg = pExpr->right->reg;
            scratch_free(regs, pExpr->left->reg);
            free(t1);
            free(done);
            break;
        case EXPR_LT:
            expr_codegen(pExpr->left, pDecl, regs, offset, isGlobal);
            expr_codegen(pExpr->right, pDecl, regs, offset, isGlobal);

            t1 = label_name(label_create());
            done = label_name(label_create());

            printf("CMPQ %s, %s\n", scratch_name(regs, pExpr->right->reg), scratch_name(regs, pExpr->left->reg));
            printf("JGE %s\n", t1);
            printf("MOVQ $1, %s\n", scratch_name(regs, pExpr->left->reg));
            printf("JMP %s\n", done);
            printf("%s:\n", t1);
            printf("MOVQ $0, %s\n", scratch_name(regs, pExpr->left->reg));
            printf("%s:\n", done);

            pExpr->reg = pExpr->left->reg;
            scratch_free(regs, pExpr->right->reg);
            free(t1);
            free(done);
            break;
        case EXPR_LE:
            expr_codegen(pExpr->left, pDecl, regs, offset, isGlobal);
            expr_codegen(pExpr->right, pDecl, regs, offset, isGlobal);
 
            t1 = label_name(label_create());
            done = label_name(label_create());

            printf("CMPQ %s, %s\n", scratch_name(regs, pExpr->right->reg), scratch_name(regs, pExpr->left->reg));
            printf("JG %s\n", t1);
            printf("MOVQ $1, %s\n", scratch_name(regs, pExpr->left->reg));
            printf("JMP %s\n", done);
            printf("%s:\n", t1);
            printf("MOVQ $0, %s\n", scratch_name(regs, pExpr->left->reg));
            printf("%s:\n", done);

            pExpr->reg = pExpr->left->reg;
            scratch_free(regs, pExpr->right->reg);
            free(t1);
            free(done);
            break;
        case EXPR_GT:
            expr_codegen(pExpr->left, pDecl, regs, offset, isGlobal);
            expr_codegen(pExpr->right, pDecl, regs, offset, isGlobal);
 
            t1 = label_name(label_create());
            done = label_name(label_create());

            printf("CMPQ %s, %s\n", scratch_name(regs, pExpr->right->reg), scratch_name(regs, pExpr->left->reg));
            printf("JLE %s\n", t1);
            printf("MOVQ $1, %s\n", scratch_name(regs, pExpr->left->reg));
            printf("JMP %s\n", done);
            printf("%s:\n", t1);
            printf("MOVQ $0, %s\n", scratch_name(regs, pExpr->left->reg));
            printf("%s:\n", done);

            pExpr->reg = pExpr->left->reg;
            scratch_free(regs, pExpr->right->reg);
            free(t1);
            free(done);
            break;
        case EXPR_GE:
            expr_codegen(pExpr->left, pDecl, regs, offset, isGlobal);
            expr_codegen(pExpr->right, pDecl, regs, offset, isGlobal);
 
            t1 = label_name(label_create());
            done = label_name(label_create());

            printf("CMPQ %s, %s\n", scratch_name(regs, pExpr->right->reg), scratch_name(regs, pExpr->left->reg));
            printf("JL %s\n", t1);
            printf("MOVQ $1, %s\n", scratch_name(regs, pExpr->left->reg));
            printf("JMP %s\n", done);
            printf("%s:\n", t1);
            printf("MOVQ $0, %s\n", scratch_name(regs, pExpr->left->reg));
            printf("%s:\n", done);

            pExpr->reg = pExpr->left->reg;
            scratch_free(regs, pExpr->right->reg);
            free(t1);
            free(done);
            break;
        case EXPR_ADD:
        case EXPR_SUB:
            expr_codegen(pExpr->left, pDecl, regs, offset, isGlobal);
            expr_codegen(pExpr->right, pDecl, regs, offset, isGlobal);
 
            printf("%s %s, %s\n", (pExpr->kind == EXPR_ADD) ? "ADDQ" : "SUBQ",
                    scratch_name(regs, pExpr->right->reg), scratch_name(regs, pExpr->left->reg));

            pExpr->reg = pExpr->left->reg;
            scratch_free(regs, pExpr->right->reg);
            break;
        case EXPR_MUL:
            expr_codegen(pExpr->left, pDecl, regs, offset, isGlobal);
            expr_codegen(pExpr->right, pDecl, regs, offset, isGlobal);
 
            printf("MOVQ %s, %%rax\n", scratch_name(regs, pExpr->left->reg));
            printf("IMULQ %s\n", scratch_name(regs, pExpr->right->reg));
            printf("MOVQ %%rax, %s\n", scratch_name(regs, pExpr->right->reg));

            pExpr->reg = pExpr->right->reg;
            scratch_free(regs, pExpr->left->reg);
            break;
        case EXPR_DIV:
        case EXPR_MOD:
            expr_codegen(pExpr->left, pDecl, regs, offset, isGlobal);
            expr_codegen(pExpr->right, pDecl, regs, offset, isGlobal);
 
            printf("MOVQ %s, %%rax\n", scratch_name(regs, pExpr->left->reg));
            printf("CQTO\n");
            printf("IDIVQ %s\n", scratch_name(regs, pExpr->right->reg));
            printf("MOVQ %s, %s\n", (pExpr->kind == EXPR_DIV) ? "%rax" : "%rdx",
                    scratch_name(regs, pExpr->right->reg));

            pExpr->reg = pExpr->right->reg;
            scratch_free(regs, pExpr->left->reg);
            break;
        case EXPR_EXPONENT:
            expr_codegen(pExpr->left, pDecl, regs, offset, isGlobal);
            expr_codegen(pExpr->right, pDecl, regs, offset, isGlobal);
 
            r1 = scratch_alloc(regs);
            r2 = scratch_alloc(regs);

            printf("MOVQ %s, %%rax\n", scratch_name(regs, pExpr->left->reg));
            printf("MOVQ %s, %s\n", scratch_name(regs, pExpr->right->reg), scratch_name(regs, r1));
            printf("MOVQ %s, %s\n", scratch_name(regs, pExpr->left->reg), scratch_name(regs, r2));

            loop = label_name(label_create());
            done = label_name(label_create());

            printf("%s:\n", loop);
            printf("CMPQ $1, %s\n", scratch_name(regs, r1));
            printf("JLE %s\n", done);
            printf("IMULQ %s\n", scratch_name(regs, r2));
            printf("DEC %s\n", scratch_name(regs, r1));
            printf("JMP %s\n", loop);
            printf("%s:\n", done);
            printf("MOVQ %%rax, %s\n", scratch_name(regs, pExpr->right->reg));

            pExpr->reg = pExpr->right->reg;
            scratch_free(regs, r1);
            scratch_free(regs, r2);
            scratch_free(regs, pExpr->left->reg);

            free(loop);
            free(done);
            break;
        case EXPR_INC:
        case EXPR_DEC:
            expr_codegen(pExpr->left, pDecl, regs, offset, isGlobal);
 
            sym_code = symbol_codegen(pExpr->left->symbol);
            printf("%s %s\n", (pExpr->kind == EXPR_INC) ? "INCQ" : "DECQ", sym_code);
            printf("MOVQ %s, %s\n", sym_code, scratch_name(regs, pExpr->left->reg));

            free((void*)sym_code);
            pExpr->reg = pExpr->left->reg;
            break;
        case EXPR_SUBSCRIPT:
            pExpr->reg = scratch_alloc(regs);
            type = expr_typecheck(pExpr);
            expr_codegen(pExpr->right, pDecl, regs, offset, isGlobal);
 
            if(isGlobal || pExpr->left->symbol->kind == SYMBOL_GLOBAL)
            {
                
                if(type->kind == TYPE_STRING)
                {
                    printf("MOVQ %s(, %s, 8), %s\n", pExpr->left->name,
                            scratch_name(regs, pExpr->right->reg), scratch_name(regs, pExpr->reg));
                }
                else
                {
                    r1 = scratch_alloc(regs);
                    printf("LEAQ %s(%%rip), %s\n", pExpr->left->name, scratch_name(regs, r1));
                    printf("MOVQ (%s, %s, 8), %s\n",  scratch_name(regs, r1),
                            scratch_name(regs, pExpr->right->reg), scratch_name(regs, pExpr->reg));

                    scratch_free(regs, r1);
                }
            }
            else
            {
                printf("MOVQ -%d(%%rbp, %s, 8), %s\n", 
                        (pExpr->left->symbol->type->value->integer_value + pExpr->left->symbol->which) * 8,
                        scratch_name(regs, pExpr->right->reg), scratch_name(regs, pExpr->reg));
            }

            scratch_free(regs, pExpr->right->reg);
            break;
        case EXPR_CALL:
            if(pExpr->left->symbol)
                moveParamsToRegs(pExpr->right, pDecl, regs, offset, isGlobal);

            printf("PUSHQ %%r10\n");
            printf("PUSHQ %%r11\n");

            printf("CALL %s\n", pExpr->left->name);

            printf("POPQ %%r11\n");
            printf("POPQ %%r10\n");

            if(pExpr->left->symbol && pExpr->left->symbol->type->subtype->kind != TYPE_VOID)
            {
                pExpr->reg = scratch_alloc(regs);
                printf("MOVQ %%rax, %s\n", scratch_name(regs, pExpr->reg));
            }

            break;
        case EXPR_ARG:
            printf("EXPR_ARG - Not Implemented Yet.\n");
            break;
        case EXPR_GROUP:
            expr_codegen(pExpr->left, pDecl, regs, offset, isGlobal);
            pExpr->reg = pExpr->left->reg;
            break;
        case EXPR_INIT_LIST:
            type = expr_typecheck(pExpr);
            kind = type->subtype->kind;
            type_destroy(&type);
            
            count = 0;
            temp = pExpr;
            while(temp)
            {
                count++;
                temp = temp->right;
            }

            if(kind == TYPE_INTEGER || kind == TYPE_CHAR || kind == TYPE_BOOL)
            {
                if(isGlobal)
                {
                    printf("\n\t.quad %d\n", pExpr->left->integer_value);
                    struct expr* temp = pExpr->right;
                    while(temp)
                    {
                        printf("\t.quad %d\n", temp->left->integer_value);
                        temp = temp->right; 
                    }
                }
                else
                {
                    printf("MOVQ $%d, -%d(%%rbp)\n", pExpr->left->integer_value, (count+ offset) * 8);

                    temp = pExpr->right;
                    while(temp)
                    {
                        count--;
                        printf("MOVQ $%d, -%d(%%rbp)\n", temp->left->integer_value, (count + offset) * 8);
                        temp = temp->right;
                    }
                }
            }
            else
            {
                vec = vectorInit(stringCmp, stringFree);
                vectorInsert(vec, label_name(label_create()));

                printf(".data\n");
                printf("%s:\n", (char*)vectorAt(vec, vec->size - 1));
                printf("\t.string \"%s\"\n", pExpr->left->string_literal);
                struct expr* temp = pExpr->right;
                while(temp)
                {
                    vectorInsert(vec, label_name(label_create()));
                    printf("%s:\n", (char*)vectorAt(vec, vec->size - 1));
                    printf("\t.string \"%s\"\n", temp->left->string_literal);
                    temp = temp->right; 
                }

                if(isGlobal)
                {
                    printf("%s:\n", pDecl->name);
                    for(int i = 0; i < vec->size; i++)
                        printf("\t.quad %s\n", (char*)vectorAt(vec, i));

                    printf(".text\n");
                }
                else
                {
                    printf(".text\n");
                    printf("MOVQ $%s, -%d(%%rbp)\n", (char*)vectorAt(vec, count - 1), (count+ offset) * 8);

                    temp = pExpr->right;
                    while(temp)
                    {
                        count--;
                        printf("MOVQ $%s, -%d(%%rbp)\n", (char*)vectorAt(vec, count - 1), (count + offset) * 8);
                        temp = temp->right;
                    }
                }

                vectorDestroy(&vec);
            }

            break;
        case EXPR_NOT:
            expr_codegen(pExpr->left, pDecl, regs, offset, isGlobal);
 
            t1 = label_name(label_create());
            done = label_name(label_create());

            printf("CMPQ $0, %s\n", scratch_name(regs, pExpr->left->reg));
            printf("JE %s\n", t1);
            printf("MOVQ $0, %s\n", scratch_name(regs, pExpr->left->reg));
            printf("JMP %s\n", done);
            printf("%s:\n", t1);
            printf("MOVQ $1, %s\n", scratch_name(regs, pExpr->left->reg));
            printf("%s:\n", done);

            pExpr->reg = pExpr->left->reg;
            free(t1);
            free(done);
            break;
        case EXPR_UNARY_MINUS:
            expr_codegen(pExpr->left, pDecl, regs, offset, isGlobal);

            r1 = scratch_alloc(regs);
            printf("MOVQ $0, %s\n", scratch_name(regs, r1));
            printf("SUBQ %s, %s\n", scratch_name(regs, pExpr->left->reg), scratch_name(regs, r1));

            pExpr->reg = r1;
            scratch_free(regs, pExpr->left->reg);
            break;
    default:
        printf("error: invalid expression kind - %d\n", pExpr->kind);
        expr_print(pExpr); printf("\n");
        break;
    }
}

void expr_print(struct expr* expr)
{
    if(expr)
    {
        char buffer[STRMAX];
        switch(expr->kind)
        {
            case EXPR_INT_LITERAL:
                printf("%d", expr->integer_value);
                break;
            case EXPR_CHAR_LITERAL:
                printf("'%c'", expr->integer_value);
                break;
            case EXPR_BOOL_LITERAL:
                printf("%s", expr->integer_value ? "true":"false");
                break;
            case EXPR_STRING_LITERAL:
                unclean_string(expr->string_literal, buffer);

                printf("\"%s\"", buffer);
                break;
            case EXPR_NAME:
                printf("%s", expr->name);
                break;
            case EXPR_CALL:
                expr_print(expr->left);     printf("(");
                expr_print(expr->right);    printf(")");
                break;
            case EXPR_INIT_LIST:
                printf("{");
                expr_print(expr->left);
                if(expr->right)
                {
                    printf(", ");
                    expr_print(expr->right);
                }
                printf("}");
                break;
            case EXPR_ARG:
                expr_print(expr->left);
                if(expr->right)
                {
                    printf(", ");
                    expr_print(expr->right);
                }
                break;
            case EXPR_SUBSCRIPT:
                expr_print(expr->left);     printf("[");
                expr_print(expr->right);    printf("]");
                break;
            case EXPR_NOT:
                printf("!");
                expr_print(expr->left);
                break;
            case EXPR_UNARY_MINUS:
                printf("-");
                expr_print(expr->left);
                break;
            case EXPR_INC:
                expr_print(expr->left);
                printf("++");
                break;
            case EXPR_DEC:
                expr_print(expr->left);
                printf("--");
                break;
            case EXPR_GROUP:
                printf("(");
                expr_print(expr->left);
                printf(")");
                break;
            default:
                print_operator(expr);
                break;
        }
    }
}

void expr_destroy(struct expr** ppExpr)
{
    if(ppExpr && *ppExpr)
    {
        struct expr* pExpr = *ppExpr;
        if(pExpr->name)
            free((char*)pExpr->name);
        
        if(pExpr->string_literal)
            free((char*)pExpr->string_literal);

        expr_destroy(&pExpr->left);
        expr_destroy(&pExpr->right);

        free(pExpr);

        *ppExpr = NULL;
    }
}

static void print_operator(struct expr* pExpr)
{
    if(pExpr)
    {
        expr_print(pExpr->left);

        switch(pExpr->kind)
        {
            case EXPR_ASSIGN:
                printf(" = ");
                break;
            case EXPR_OR:
                printf(" || ");
                break;
            case EXPR_AND:
                printf(" && ");
                break;
            case EXPR_EQ:
                printf(" == ");
                break;
            case EXPR_NE:
                printf(" != ");
                break;
            case EXPR_LT:
                printf(" < ");
                break;
            case EXPR_LE:
                printf(" <= ");
                break;
            case EXPR_GT:
                printf(" > ");
                break;
            case EXPR_GE:
                printf(" >= ");
                break;
            case EXPR_ADD:
                printf(" + ");
                break;
            case EXPR_SUB:
                printf(" - ");
                break;
            case EXPR_MUL:
                printf(" * ");
                break;
            case EXPR_DIV:
                printf(" / ");
                break;
            case EXPR_MOD:
                printf(" %% ");
                break;
            case EXPR_EXPONENT:
                printf(" ^ ");
                break;
           default:
                printf(" Unknown Expression ");
                break;
        }

        expr_print(pExpr->right);
    }
}

static void unclean_string(const char* input, char* output)
{
    memset(output, 0, STRMAX);
    int inputLen = strlen(input);
    int outIdx = 0;

    for(int i = 0; i < inputLen; i++)
    {
        switch(input[i])
        {
           case 0xa: // newline
                output[outIdx] = '\\';
                output[outIdx + 1] = 'n';
                outIdx += 2;
                break;
            case 0x2f: // slash
                output[outIdx] = '\\';
                output[outIdx + 1] = '\\';
                outIdx += 2;
                break;
            case 0x22: // double quote
                output[outIdx] = '\\';
                output[outIdx + 1] = '\"';
                outIdx += 2;
                break;
            case 0x27: // single quote
                output[outIdx] = '\\';
                output[outIdx + 1] = '\'';
                outIdx += 2;
                break;
            default:
                output[outIdx] = input[i];
                outIdx++;
                break;
        }
    }
}

static int init_list_typecheck(struct type* base, struct expr* list)
{
    if(!base || !list)
    {
        fprintf(stderr, "[ERROR] init_list_typecheck - invalid pointers passed to function\n");
        return -1;
    }

    int count = 0;
    struct expr* temp = list;
    while(temp)
    {
        struct type* t = expr_typecheck(temp->left);
        if(!type_equals(base, t))
        {
            printf("type error: Every element of the init list must be of type ");
            type_print(base); printf(", type ");
            type_print(t); printf(" found in list.\n");
            printf("  - "); expr_print(list); printf("\n");
        }
        type_destroy(&t);

        count++;
        temp = temp->right;
    }

    return count;
}

static void moveParamsToRegs(struct expr* pExpr, struct decl* pDecl, struct scratch regs[], int offset, bool isGlobal)
{

    if(!pExpr) return;
    
    char* paramRegs[] = {"%rdi", "%rsi", "%rdx", "%rcx", "%r8", "%r9"};

    int i = 0;
    while(pExpr && i < 6)
    {
        expr_codegen(pExpr->left, pDecl, regs, offset, isGlobal);
        printf("MOVQ %s, %s\n", scratch_name(regs, pExpr->left->reg), paramRegs[i]);
        scratch_free(regs, pExpr->left->reg);

        pExpr = pExpr->right;
        i++;
    }

    if(pExpr != NULL)
    {
        while(pExpr)
        {
            printf("PUSHQ %s\n", scratch_name(regs, pExpr->left->reg));
            pExpr = pExpr->right;
        }
    }
}

static bool stringCmp(void* a, void* b)
{
    if(!a || !b) return false;

    char* strA = (char*)a;
    char* strB = (char*)b;

    return strncmp(strA, strB, strlen(strA)) == 0;
}

static void stringFree(void** item)
{
    if(!item) return;

    char** pstr = (char**)item;
    char* str = *pstr;

    free(str);
    *pstr = NULL;
}

