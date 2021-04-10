#include <stdio.h>

int nextFreeIndex = 0;

void printRequiredArgs(int required, int given, char * command, FILE * out)
{
    fprintf(out, "Wrong number of args (given: %d, required: %d) for CLI command '%s'\n", given, required, command);
}