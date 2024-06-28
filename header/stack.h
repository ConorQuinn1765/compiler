#ifndef STACK_H
#define STACK_H

typedef struct stack
{
    int size;
    int capacity;
    void** arr;
}stack;

stack* stack_create(void);

void stack_push(stack* pStack, void* data);
void* stack_pop(stack* pStack);
int stack_size(stack* pStack);
void* stack_item(stack* pStack, int index);

void stack_destroy(stack** pStack);

#endif
