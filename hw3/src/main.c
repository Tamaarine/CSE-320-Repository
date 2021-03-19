#include <stdio.h>
#include "sfmm.h"
#include "helpingFunction.h"

int main(int argc, char const *argv[]) {

    char * ptr1 = sf_malloc(50 * sizeof(double));
    *(ptr1) = 'A';
    
    sf_show_blocks();
    printf("\n");
    
    char * ptr2 = sf_malloc(78 * sizeof(double));
    *(ptr2) = 'A';
    
    sf_show_blocks();
    printf("\n");
    
    char * ptr3 = sf_malloc(1 * sizeof(double));
    *(ptr3) = 'A';
    
    sf_show_blocks();
    printf("\n");
    
    ptr1 = sf_realloc(ptr1, 300);
    
    sf_show_blocks();
    printf("\n");
    
    ptr2 = sf_realloc(ptr2, 640);
    
    sf_show_blocks();
    printf("\n");
    
    sf_free(ptr1);
    sf_show_blocks();
    printf("\n");
    
    ptr2 = sf_realloc(ptr2, 300);
    sf_show_blocks();
    printf("\n");
    
    sf_show_free_lists();
    
    
    
    
    // double* ptr = sf_malloc(sizeof(double));

    // *ptr = 320320320e-320;

    // printf("%f\n", *ptr);

    // sf_free(ptr);

    return EXIT_SUCCESS;
}
