#ifndef VECTOR_H
#define VECTOR_H
#include <stdbool.h>

typedef struct vector
{
    int size;
    int capacity;
    void** arr;
    bool (*itemEquals)(void* a, void* b);
    void (*itemDestroy)(void** item);
}Vector;

Vector* vectorInit(bool (*itemEquals)(void* a, void* b), void (*itemDestroy)(void** item));
bool vectorInsert(Vector* pVector, void* data);
bool vectorRemove(Vector* pVector, void* data);
void* vectorRemoveAt(Vector* pVector, int idx);
void* vectorAt(Vector* pVector, int idx);
void vectorDestroy(Vector** phVector);

#endif
