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
#include "image.h"
int main(int argc, char **argv)
{    
    // Initialize the hash map
    initialize_bdd_hash_map();
    
    // Initialize the index map
    initialize_bdd_index_map();
    
    // Initilize global_options to just 0
    global_options = 0;
    
    // unsigned char input4[] = {1,2,5,6,3,4,7,8,9,10,13,14,11,12,15,16}; // 4x4 tree, full binary tree. we expect 15 nodes
    // bdd_from_raster(4,4, input4);

    // unsigned char input5[] = {1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8}; // 4x4, Level 1 nodes don't exist here. Every level 1 node will be pointing to the same color, and thus not exist. (7 nodes total)
    // bdd_from_raster(4,4, input5);

    // unsigned char input6[] = {1,2,5,6,9,10,13,14,3,4,7,8,11,12,15,16,17,18,21,22,25,26,29,30,19,20,23,24,27,28,31,32,33,34,37,38,41,42,45,46,35,36,39,40,43,44,47,48,49,50,53,54,57,58,61,62,51,52,55,56,59,60,63,64};
    // // last one's a 8x8 full binary tree, you can expect 63 nodes there
    // bdd_from_raster(8,8, input6);
    
    // Testing pgm_to_birp
    // int result = pgm_to_birp(stdin, stdout);
    
    // int result = pgm_to_ascii(stdin, stdout);
    
    // BDD_NODE * ptr = bdd_deserialize(stdin);
    
    // int width = 0;
    // int height = 0;
    // BDD_NODE * ptr = img_read_birp(stdin, &width, &height);
    
    // // Make sure you call initialize_bdd_index_map() again after you do img_read_birp
    // initialize_bdd_index_map();
    
    // int result = img_write_birp(ptr, width, height, stdout);
    
    // unsigned char input1[] = {4,4,5,5,4,4,5,5,6,6,7,7,6,6,7,7};
    // BDD_NODE * root = bdd_from_raster(4,4, input1);
    
    // unsigned char input2[] = {4,4,5,5,4,4,5,5,6,6,5,5,6,6,5,5};
    // BDD_NODE * root = bdd_from_raster(4,4, input2);
    
    // unsigned char input3[] = {4,4,6,0,6,6,6,0,7,7,7,0,0,0,0,0};
    // BDD_NODE * root = bdd_from_raster(4,4, input3);
    
    // unsigned char output[100];
    
    // bdd_to_raster(root, 4, 4, output);    
    
    // for(int i=0;i<16;i++)
    // {
    //     printf("%d\n", output[i]);
    // }
    
    
    // Test birp_to_pgm
    // int result = birp_to_pgm(stdin, stdout);
    

    
    
    if(validargs(argc, argv))
    {
        printf("%d\n", global_options);
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

    // Transformation requires 8 shifts to the right
    int transformationByte = 0x00000F00 & global_options;
    transformationByte = transformationByte >> 8;
    
    // Paramater requires 16 shifts to the right 
    int parameterByte = 0x00FF0000 & global_options;
    parameterByte = parameterByte >> 16;
    
    // This variable will be telling us whether or not the oepration is successful
    int result = 0;

    // Okay now we will actually determine which function to do base on the arguments
    // pgm to pgm operation
    if(inputByte == 1 && outputByte == 1)
    {
        
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
    
    
        
    return EXIT_SUCCESS;
}

/*
 * Just a reminder: All non-main functions should
 * be in another file not named main.c
 */
