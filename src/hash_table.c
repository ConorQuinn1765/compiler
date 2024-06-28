#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hash_table.h"

extern int STRMAX;

static int hash(const char* key, int capacity);
static bool resize(struct hash_table* ht);
static bool keys_equal(void* a, void* b);
static void keys_destroy(void** key);

struct hash_table* hash_table_create(void)
{
    struct hash_table* ht = malloc(sizeof(struct hash_table));
    if(ht)
    {
        ht->size = 0;
        ht->capacity = 8;
        ht->arr = calloc(ht->capacity, sizeof(Node*));
        if(!ht->arr)
        {
            free(ht);
            fprintf(stderr, "hash_table_create - Failed to allocate space for hash table\n");
            return NULL;
        }
    }

    return ht;
}

bool hash_table_insert(struct hash_table* ht, const char* key, void* value)
{
    if(!ht || !key) return false;

    if(ht->size >= ht->capacity)
        if(!resize(ht))
            return false;

    int idx = hash(key, ht->capacity);

    if(ht->arr[idx])
        while(ht->arr[idx % ht->capacity])
            idx++;

    Node* n = malloc(sizeof(Node));
    if(!n)
    {
        fprintf(stderr, "hash_table_insert - Failed to allocate space for new node\n");
        return false;
    }

    n->key = malloc(sizeof(char) * STRMAX);
    if(!n->key)
    {
         fprintf(stderr, "hash_table_insert - Failed to allocate space for node key\n");
         free(n);
         return false;
    }

    strncpy(n->key, key, STRMAX);
    n->value = value;

    ht->arr[idx % ht->capacity] = n;
    ht->size++;
    return true;
}

void* hash_table_remove(struct hash_table* ht, const char* key)
{
    if(!ht || !key) return NULL;

    int idx = hash(key, ht->capacity);

    int count = 0;
    void* value = NULL;
    while(count < ht->capacity)
    {
        if(ht->arr[idx % ht->capacity])
        {
            if(strncmp(ht->arr[idx % ht->capacity]->key, key, STRMAX) == 0)
            {
                value = ht->arr[idx % ht->capacity]->value;
                free(ht->arr[idx % ht->capacity]->key);
                free(ht->arr[idx % ht->capacity]);
                ht->arr[idx % ht->capacity] = NULL;
                count = ht->capacity;
            }
            else
                idx++;
        }

        count++;
    }

    ht->size--;
    return value;
}

void* hash_table_at(struct hash_table* ht, const char* key)
{
    if(!ht || !key) return NULL;

    int idx = hash(key, ht->capacity);
    int count = 0;

    while(count < ht->capacity)
    {
        if(ht->arr[idx % ht->capacity])
        {
            if(strncmp(ht->arr[idx % ht->capacity]->key, key, STRMAX) == 0)
                return ht->arr[idx % ht->capacity]->value;
        }

        idx++;
        count++;
    }

    return NULL;
}

Vector* hash_table_keys(struct hash_table* ht)
{
    if(!ht) return NULL;

    Vector* pVector = vectorInit(keys_equal, keys_destroy);
    for(int i = 0; i < ht->size; i++)
        vectorInsert(pVector, ht->arr[i]);

    return pVector;
}

void hash_table_destroy(struct hash_table** ht)
{
    if(!ht || !*ht) return;

    for(int i = 0; i < (*ht)->capacity; i++)
    {
        if((*ht)->arr[i])
        {
            free((*ht)->arr[i]->key);
            free((*ht)->arr[i]);
        }
    }

    free((*ht)->arr);
    free(*ht);
    *ht = NULL;
}

static int hash(const char* key, int capacity)
{
    const unsigned long FNV_OFFSET = 14695981039346656037UL;
    const unsigned long FNV_PRIME =  1099511628211U;

    unsigned long hash = FNV_OFFSET;
    for (const char* p = key; *p; p++) {
        hash ^= (unsigned long)(unsigned char)(*p);
        hash *= FNV_PRIME;
    }

    return hash % capacity;
}

static bool resize(struct hash_table* ht)
{
    if(!ht) return false;

    Node** temp = calloc((ht->capacity * 2), sizeof(Node*));
    if(!temp)
        return false;

    for(int i = 0; i < ht->capacity; i++)
    {
        Node* n = ht->arr[i];
        int idx = hash(n->key, ht->capacity * 2);

        if(temp[idx] == NULL)
            temp[idx] = n;
        else
        {
            while(temp[idx % (ht->capacity * 2)])
                idx++;

            temp[idx % (ht->capacity * 2)] = n;
        }
    }

    free(ht->arr);
    ht->arr = temp;
    ht->capacity *= 2;

    return true;
}

static bool keys_equal(void* a, void* b)
{
    if(!a) return b == NULL;
    if(!b) return a == NULL;

    char* keyA = (char*)a;
    char* keyB = (char*)b;
    int lenA = strlen(keyA);
    int lenB = strlen(keyB);

    if(lenA != lenB) return false;

    return strncmp(keyA, keyB, lenA) == 0;
}

static void keys_destroy(void** key)
{
    if(!key || !*key) return;
    free(*key);
    *key = NULL;
}
