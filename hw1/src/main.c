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
    // int result = pgm_to_ascii(stdin, stdout);
    
    initialize_bdd_hash_map();
    
    // // Need to test bdd_lookup
    // int result1 = bdd_lookup(1, 4, 8);
    // printf("%d\n", result1);
    
    // int result2 = bdd_lookup(5, 100, 24);
    // printf("%d\n", result2);
    
    // int result3 = bdd_lookup(1, 4, 8);
    // printf("Result %d\n", result3);
    
    // BDD_NODE * nodePtr2 = *(bdd_hash_map + hashFunction(4,8));
    // printf("nodePtr2 is %d %d %d\n", nodePtr2->level, nodePtr2->left, nodePtr2->right);

    // BDD_NODE * nodePtr3 = *(bdd_hash_map + hashFunction(100,24));
    // printf("nodePtr3 is %d %d %d\n", nodePtr3->level, nodePtr3->left, nodePtr3->right);
    
    // BDD_NODE * nodePtr4 = *(bdd_hash_map + hashFunction(4,8));
    // printf("nodePtr3 is %d %d %d\n", nodePtr4->level, nodePtr4->left, nodePtr4->right);
    
    unsigned char input1[] = {4, 8 ,4, 8}; // Should return 256 because only one node is made
    bdd_from_raster(2, 2, input1);
    
    unsigned char input2[] = {4,2,12,255}; // Should return 259 3 node is created
    bdd_from_raster(2, 2, input2);
    
    unsigned char input3[] = {4,2, 36, 49}; // Should return 261 2 node is created 4,2 is created before
    bdd_from_raster(2,2,input3);
    
    unsigned char input4[] = {4,2, 4,8}; // Should return 262 only 1 node is created
    bdd_from_raster(2,2, input4);
    
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
