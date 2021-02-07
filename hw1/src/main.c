#include <stdio.h>
#include <stdlib.h>

#include "const.h"
#include "debug.h"

#ifdef _STRING_H
#error "Do not #include <string.h>. You will get a ZERO."
#endif

#ifdef _STRINGS_H
#error "Do not #include <strings.h>. You will get a ZERO."
#endif

#ifdef _CTYPE_H
#error "Do not #include <ctype.h>. You will get a ZERO."
#endif

// My own functions
#include "helpingFunctions.h"

int main(int argc, char **argv)
{    
    // printf("%d\n", validateNumberString("9874"));
    // printf("%d\n", stringToInteger("10"));
    // printf("%d\n", stringToInteger("487"));
    // printf("%d\n", stringToInteger("12321"));
    // printf("%d\n", stringToInteger("10"));
    // printf("%d\n", stringToInteger("0"));
    // printf("%d\n", stringToInteger("1"));
    // printf("%d\n", stringToInteger("123"));
    // printf("%d\n", stringToInteger("89765"));
    // printf("%d\n", compareString("Hello World", "Hello Worid"));
    // printf("%d\n", compareString(*(argv+ 2), "birp"));
    
    // Testing the function for reading pgm
    int result = pgm_to_ascii(stdin, stdout);
    
    
    // Initilize global_options to just 0
    global_options = 0;
    
    if(validargs(argc, argv))
    {
        USAGE(*argv, EXIT_FAILURE);
    }
    if(global_options & HELP_OPTION)
    {
        USAGE(*argv, EXIT_SUCCESS);
    }
    
    // TO BE IMPLEMENTED
    return EXIT_SUCCESS;
}

/*
 * Just a reminder: All non-main functions should
 * be in another file not named main.c
 */
