#include <stdio.h>

int nextFreeIndex = 0;

void printRequiredArgs(int required, int given, char * command, FILE * out)
{
    fprintf(out, "Wrong number of args (given: %d, required: %d) for CLI command '%s'\n", given, required, command);
}

void removeNewline(char * str, int byteRead)
{
    char * ptr = str;
    
    // Only remove the \n if it is ending with \n
    if(*(ptr + byteRead - 1) == '\n')
    {
        *(ptr + byteRead - 1) = 0;
    }
}

int string2Integer(char * str)
{
    char first = *(str);
    char second = *(str + 1);
    
    // Only one digit
    if(second == '\0')
    {
        if(first < '0' || first > '9')
        {
            return -1;
        }
        else
        {
            return first - '0';
        }
    }
    // Can be 2 digits
    else
    {
        // Both of the digits read are numbers hence we can convert
        if((first >= '0' && first <= '9') && (second >= '0' && second <= '9'))
        {
            int output = (first - '0') * 10 + (second - '0');
            
            if(output >= 64)
            {
                return -1;
            }
            else
            {
                return output;
            }
        }
        else
        {
            return -1;
        }
    }
}
