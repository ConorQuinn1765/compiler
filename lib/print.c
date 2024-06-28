#include <stdio.h>

void printBool(int b)
{
    printf("%s", b ? "true" : "false");
}

void printChar(int c)
{
    printf("%c", c);
}

void printInt(int i)
{
    printf("%d", i);
}

void printString(const char* s)
{
    printf("%s", s);
}

