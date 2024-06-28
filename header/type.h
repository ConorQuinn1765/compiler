#ifndef TYPE_H
#define TYPE_H
#include <stdbool.h>

typedef enum {TYPE_INTEGER, TYPE_STRING, TYPE_CHAR, TYPE_BOOL, TYPE_AUTO, TYPE_FUNCTION, TYPE_ARRAY, TYPE_VOID} type_t;

struct type
{
    type_t kind;
    struct type* subtype;
    struct param_list* params;
    struct expr* value;
};

struct type* type_create(type_t kind, struct type* subtype, struct param_list* params, struct expr* value);

bool type_equals(struct type* a, struct type* b);
struct type* type_copy(struct type* type);

void type_resolve(struct type* pType);
void type_print(struct type* pType);
const char* type_string(struct type* pType);
void type_destroy(struct type** ppType);

#endif
