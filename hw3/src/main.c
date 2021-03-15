#include <stdio.h>
#include "sfmm.h"
#include "helpingFunction.h"

int main(int argc, char const *argv[]) {
    char * ptr1 = sf_malloc(1);
    *(ptr1) = 'A';
    // printf("%c\n", *ptr1);
    sf_show_blocks();    
    
    char * ptr2 = sf_malloc(1);
    *(ptr2) = 'B';
    
    sf_show_blocks();    
    
    int * ptr3 = sf_malloc(8 * sizeof(int));
    *(ptr3 + 0) = 1;
    *(ptr3 + 1) = 69;
    *(ptr3 + 2) = 80;

    sf_show_blocks();    
    
    
    
    
    // double* ptr = sf_malloc(sizeof(double));

    // *ptr = 320320320e-320;

    // printf("%f\n", *ptr);

    // sf_free(ptr);

    return EXIT_SUCCESS;
}
