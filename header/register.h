#ifndef REGISTER_H
#define REGISTER_H
#include <stdbool.h>

struct scratch
{
    int r;
    bool in_use;
    const char* name;
};

int scratch_alloc(struct scratch regs[]);
void scratch_free(struct scratch regs[], int r);
const char* scratch_name(struct scratch regs[], int r);

int label_create();
char* label_name(int label);

#endif
