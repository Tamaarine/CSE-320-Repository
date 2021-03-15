#include <stdio.h>
#include "sfmm.h"
#include "helpingFunction.h"

int main(int argc, char const *argv[]) {
    // char * ptr1 = sf_malloc(1);
    // *(ptr1) = 'A';
    
    // sf_show_blocks();   
    // printf("\n"); 
    
    // char * ptr2 = sf_malloc(1);
    // *(ptr2) = 'B';
    
    // sf_show_blocks();    
    // printf("\n"); 
    
    // int * ptr3 = sf_malloc(2020 * sizeof(int));
    // *(ptr3 + 0) = 1;
    // *(ptr3 + 1) = 69;
    // *(ptr3 + 2) = 80;
    // *(ptr3 + 23) = 69;

    // sf_show_blocks();  
    // printf("\n"); 
    
    // char *ptr4 = sf_malloc(8168);
    // *(ptr4) = 'Y';
    
    // sf_show_blocks();  
    // printf("\n"); 
    
    // int * ptr5 = sf_malloc(114688);
    // if(ptr5 != NULL)
    // {
    //     *(ptr5) = 'A';
    // }
    
    // sf_show_blocks();  
    // printf("\n"); 
        
    // sf_show_free_lists();  
    // printf("\n"); 
    
    char * ptr1 = sf_malloc(131064);
    if(ptr1 != NULL)
        *(ptr1) = 'A';
    
    
    
    // double* ptr = sf_malloc(sizeof(double));

    // *ptr = 320320320e-320;

    // printf("%f\n", *ptr);

    // sf_free(ptr);

    return EXIT_SUCCESS;
}
