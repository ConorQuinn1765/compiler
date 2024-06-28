#include <stdio.h>
#include <stdlib.h>
#include "vector.h"

Vector* vectorInit(bool (*itemEquals)(void* a, void* b), void (*itemDestroy)(void** item))
{
    Vector* pVector = malloc(sizeof(Vector));
    if(pVector)
    {
        pVector->size = 0;
        pVector->capacity = 10;
        pVector->arr = calloc(pVector->capacity, sizeof(void*));
        if(!pVector->arr)
        {
            fprintf(stderr, "vectorInit - Failed to allocate vector array\n");
            free(pVector);
            return NULL;
        }

        pVector->itemEquals = itemEquals;
        pVector->itemDestroy = itemDestroy;
    }
    return pVector;
}

bool vectorInsert(Vector* pVector, void* data)
{
    if(!pVector) return false;

    if(pVector->size >= pVector->capacity)
    {
        void** temp = realloc(pVector->arr, pVector->capacity * 2);
        if(temp == NULL) return false;

        pVector->arr = temp;
        pVector->capacity *= 2;
    }

    pVector->arr[pVector->size] = data;
    pVector->size++;

    return true;
}

bool vectorRemove(Vector* pVector, void* data)
{
    if(!pVector) return true;

    for(int i = 0; i < pVector->size; i++)
    {
        if(pVector->itemEquals(pVector->arr[i], data))
        {
            pVector->itemDestroy(pVector->arr[i]);

            for(int j = i; j < pVector->size - 1; j++)
                pVector->arr[j] = pVector->arr[j+1];

            pVector->arr[pVector->size - 1] = NULL;
            pVector->size--;
            return true;
        }
    }

    return false;
}

void* vectorRemoveAt(Vector* pVector, int idx)
{
    if(!pVector) return NULL;
    if(idx < 0 || idx >= pVector->size) return NULL;

    void* result = pVector->arr[idx];
    pVector->itemDestroy(pVector->arr[idx]);

    for(int i = idx; i < pVector->size - 1; i++)
        pVector->arr[i] = pVector->arr[i + 1];

    pVector->arr[pVector->size - 1] = NULL;
    pVector->size--;

    return result;
}

void* vectorAt(Vector* pVector, int idx)
{
    if(!pVector) return NULL;
    if(idx < 0 || idx >= pVector->size) return NULL;

    return pVector->arr[idx];
}

void vectorDestroy(Vector** phVector)
{
    if(!phVector || !*phVector) return;
    Vector* pVector = *phVector;

    for(int i = 0; i < pVector->size; i++)
        pVector->itemDestroy(&pVector->arr[i]);

    free(pVector->arr);
    free(pVector);

    *phVector = NULL;
}
