#include <string.h>

int stringCompare(char* a, char* b)
{
    return strncmp(a, b, strlen(a));    
}
