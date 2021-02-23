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
    // Initialize the hash map
    initialize_bdd_hash_map();
    
    // Initialize the index map
    initialize_bdd_index_map();
    
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
    // After the arguments are validated we can begin getting each portion of the
    // global_options to determine what functions we need to call    
    int inputByte = 0x0000000F & global_options;
    // inputByte don't need any shifts
    
    // outputByte requires 4 shifts to the right    
    int outputByte = 0x000000F0 & global_options;
    outputByte = outputByte >> 4;

    // // Transformation requires 8 shifts to the right
    // int transformationByte = 0x00000F00 & global_options;
    // transformationByte = transformationByte >> 8;
    
    // // Paramater requires 16 shifts to the right 
    // int parameterByte = 0x00FF0000 & global_options;
    // parameterByte = parameterByte >> 16;
    
    // This variable will be telling us whether or not the oepration is successful
    int result = 0;

    // Okay now we will actually determine which function to do base on the arguments
    // pgm to pgm operation
    if(inputByte == 1 && outputByte == 1)
    {
        // We will never be given this case, if we are here then we return error 
        USAGE(*argv, EXIT_FAILURE);
    }
    // pgm to birp operation
    else if(inputByte == 1 && outputByte == 2)
    {
        // We can call pgm_to_birp
        result = pgm_to_birp(stdin, stdout);
        
        // Done testing number # 2
        
    }
    // pgm to ascii operation
    else if(inputByte == 1 && outputByte == 3)
    {
        // We will call pgm_to_ascii
        result = pgm_to_ascii(stdin, stdout);
        
        // Done testing number # 2
        
    }
    // birp to pgm operation
    else if(inputByte == 2 && outputByte == 1)
    {
        // we will call birp_to_pgm
        result = birp_to_pgm(stdin, stdout);
        
        // DONE TESTING, KEEP IN MIND THAT COMMENTS ARE IN THOSE FILES
    }
    // birp to birp operation
    else if(inputByte == 2 && outputByte == 2)
    {
        // we will call birp_to_birp
        result = birp_to_birp(stdin, stdout);
    }
    // birp to ascii operation
    else if(inputByte == 2 && outputByte == 3)
    {
        // Call birp_to_ascii
        result = birp_to_ascii(stdin, stdout);
        
        // This one needs confirmation from professor
    }
    
    if(result == 0)
    {
        return EXIT_SUCCESS;
    }
    else
    {
        return EXIT_FAILURE;
    }
    
    // Just trying out my global_options see if it is correct
    // printf("Input %d\n", inputByte);    
    // printf("Output %d\n", outputByte);    
    // printf("Transformation %d\n", transformationByte);    
    // printf("Parameter %d\n", parameterByte);    
}

/*
 * Just a reminder: All non-main functions should
 * be in another file not named main.c
 */
