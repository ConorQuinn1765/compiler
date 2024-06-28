#include <stdio.h>
#include <stdlib.h>
#include "register.h"

extern int STRMAX;
extern struct scratch regs[];

static int label_num = 0;

int scratch_alloc(struct scratch regs[])
{
    for(int i = 0; i < 7; i++)
    {
        if(!regs[i].in_use)
        {
            regs[i].in_use = true;
            return i;
        }
    }

    return -1;
}

void scratch_free(struct scratch regs[], int r)
{
    if(r < 0 || r > 6) return;

    regs[r].in_use = false;
}

const char* scratch_name(struct scratch regs[], int r)
{
    if(r < 0 || r > 6) return NULL;

    return regs[r].name;
}

int label_create()
{
    return label_num++;
}

char* label_name(int label)
{
    char* label_buf = malloc(sizeof(char) * STRMAX);
    if(label_buf)
        snprintf(label_buf, STRMAX, ".L%d", label);

    return label_buf;
}

