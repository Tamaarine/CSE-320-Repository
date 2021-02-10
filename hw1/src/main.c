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
    
    // Initilize global_options to just 0
    global_options = 0;
    
    unsigned char input4[] = {1,2,5,6,3,4,7,8,9,10,13,14,11,12,15,16}; // 4x4 tree, full binary tree. we expect 15 nodes
    bdd_from_raster(4,4, input4);

    unsigned char input5[] = {1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8}; // 4x4, Level 1 nodes don't exist here. Every level 1 node will be pointing to the same color, and thus not exist. (7 nodes total)
    bdd_from_raster(4,4, input5);

    unsigned char input6[] = {1,2,5,6,9,10,13,14,3,4,7,8,11,12,15,16,17,18,21,22,25,26,29,30,19,20,23,24,27,28,31,32,33,34,37,38,41,42,45,46,35,36,39,40,43,44,47,48,49,50,53,54,57,58,61,62,51,52,55,56,59,60,63,64};
    // last one's a 8x8 full binary tree, you can expect 63 nodes there
    bdd_from_raster(8,8, input6);
    
    
    
    
    
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
