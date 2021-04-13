#include <stdio.h>

int nextFreeIndex = 0;

void printRequiredArgs(int required, int given, char * command, FILE * out)
{
    fprintf(out, "Wrong number of args (given: %d, required: %d) for CLI command '%s'\n", given, required, command);
}

void removeNewline(char * str)
{
    char * ptr = str;
    while(*(ptr) != '\n')
    {
        ptr++;
    }
    
    // If we are here then that means we encountered a \n hence remove it
    *(ptr) = 0;
}
