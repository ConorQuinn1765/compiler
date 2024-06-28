#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "stack.h"

static void stack_resize(stack* pStack);

stack* stack_create(void)
{
    stack* pStack = malloc(sizeof(stack));
    if(pStack)
    {
        pStack->size = 0;
        pStack->capacity = 2;
        pStack->arr = calloc(pStack->capacity, sizeof(void*));
        
        if(!pStack->arr)
        {
            fprintf(stderr, "stack_create - Failed to allocate space for stack\n");
            free(pStack);
            return NULL;
        }
    }

    return pStack;
}

void stack_push(stack* pStack, void* data)
{
    if(!pStack) return;

    if(pStack->size >= pStack->capacity)
        stack_resize(pStack);

    pStack->arr[pStack->size] = data;
    pStack->size++;
}

void* stack_pop(stack* pStack)
{
    if(!pStack)
    {
        fprintf(stderr, "stack_pop - Invalid stack given as arg\n");
        return NULL;
    }

    if(pStack->size == 0)
    {
        fprintf(stderr, "stack_pop - Cannot pop from an empty stack\n");
        return NULL;
    }

    pStack->size--;
    void* data = pStack->arr[pStack->size];
    pStack->arr[pStack->size] = NULL;

    return data;
}

int stack_size(stack* pStack)
{
    if(!pStack) return -1;

    return pStack->size;
}

void* stack_item(stack* pStack, int index)
{
    if(!pStack)
    {
        fprintf(stderr, "stack_item - Invalid stack given as arg\n");
        return NULL;
    }

    if(index < 0 || index >= pStack->size)
    {
        fprintf(stderr, "stack_item - Index %d out of range. Must be 0-%d\n", index, pStack->size - 1);
        return NULL;
    }

    return pStack->arr[index];
}

void stack_destroy(stack** pStack)
{
    if(pStack && *pStack)
    {
        free((*pStack)->arr);
        free(*pStack);

        *pStack = NULL;
    }
}

static void stack_resize(stack* pStack)
{
    if(!pStack) return;

    void** temp = calloc(pStack->capacity * 2, sizeof(void*));
    if(!temp)
    {
        fprintf(stderr, "stack_resize - Failed to allocate space for stack\n");
        return;
    }

    int i = 0;
    for(; i < pStack->capacity; i++)
        temp[i] = pStack->arr[i];

    free(pStack->arr);
    pStack->arr = temp;

    pStack->capacity *= 2;
}

