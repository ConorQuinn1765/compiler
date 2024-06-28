#ifndef HASH_TABLE_H
#define HASH_TABLE_H
#include <stdbool.h>
#include "vector.h"

typedef struct node Node;
struct node
{
    char* key;
    void* value;
};

struct hash_table
{
    int size;
    int capacity;
    Node** arr;
};

struct hash_table* hash_table_create(void);
bool hash_table_insert(struct hash_table* ht, const char* key, void* value);
void* hash_table_remove(struct hash_table* ht, const char* key);
void* hash_table_at(struct hash_table* ht, const char* key);
Vector* hash_table_keys(struct hash_table* ht);
void hash_table_destroy(struct hash_table** ht);

#endif
