#include <stdio.h>
#include "sfmm.h"
#include "helpingFunction.h"

int main(int argc, char const *argv[]) {
    
    char * ptr1 = sf_malloc(200);
    *(ptr1) = 'A';
    
    sf_show_blocks();
    printf("\n");
    
    char * ptr2 = sf_malloc(600 * sizeof(double));
    *(ptr2) = 'A';
    
    sf_show_blocks();
    printf("\n");
    
    char * ptr3 = sf_malloc(666);
    *(ptr3) = 'A';
    
    sf_show_blocks();
    printf("\n");
    
    char * ptr4 = sf_malloc(999);
    *(ptr4) = 'A';
    
    sf_show_blocks();
    printf("\n");
    
    ptr4 = sf_realloc(ptr4, 123);
    sf_show_blocks();
    printf("\n");
    
    
    
    
    sf_show_free_lists();

    return EXIT_SUCCESS;
}
