#include <stdio.h>
#include "decl.h"
#include "stack.h"

extern FILE* yyin;
extern int yyparse();
extern void yylex_destroy();
extern struct decl* parser_result;
extern struct stack* scope_stack;

int main(int argc, char* argv[])
{
    if(argc == 2)
        yyin = fopen(argv[1], "r");

    if(yyparse()==0)
    {
        fclose(yyin);
        yylex_destroy();

        struct scratch regs[] = {
            {0, false, "%rbx"},
            {1, false, "%r10"},
            {2, false, "%r11"},
            {3, false, "%r12"},
            {4, false, "%r13"},
            {5, false, "%r14"},
            {6, false, "%r15"}
        };

        scope_enter();
       
        decl_resolve(parser_result);
        decl_typecheck(parser_result);


        printf(".global main\n");
        decl_codegen(parser_result, regs);

        if(parser_result->type->kind == TYPE_FUNCTION)
        {
            if(parser_result->type->subtype->kind != TYPE_VOID)
                printf("MOVQ %%rax, %%rdi\n");
            else
                printf("MOVQ $0,  %%rdi\n");
        }
        printf("MOVQ $60, %%rax\n");
        printf("syscall\n");

        scope_exit();

        decl_destroy(&parser_result);
    } 
    else
    {
        fclose(yyin);
        yylex_destroy();
        decl_destroy(&parser_result);
        return 1;
    }

    return 0;
}
