#include <stdio.h>
#include "sfmm.h"
#include "helpingFunction.h"

int main(int argc, char const *argv[]) {
    
    // char * ptr1 = sf_malloc(50 * sizeof(double));
    // *(ptr1) = 'A';
    
    // sf_show_blocks();
    // printf("\n");
    
    // char * ptr2 = sf_malloc(78 * sizeof(double));
    // *(ptr2) = 'A';
    
    // sf_show_blocks();
    // printf("\n");
    
    // char * ptr3 = sf_malloc(1 * sizeof(double));
    // *(ptr3) = 'A';
    
    // sf_show_blocks();
    // printf("\n");
    
    // ptr2 = sf_realloc(ptr2, 500);
    // sf_show_blocks();
    // printf("\n");
    
    // ptr3 = sf_realloc(ptr3, 12); // Should be just using the same block
    // sf_show_blocks();
    // printf("\n");
    
    // char * ptr4 = sf_malloc(7000); // Allocate 7008 bytes 
    // *(ptr4) = 'A'; // Should only have 48 bytes left
    // sf_show_blocks();
    // printf("\n");
    
    // ptr3 = sf_realloc(ptr3, 10); // Should again use the same block now that it is not in fron tof wilderness
    // sf_show_blocks();
    // printf("\n");
    
    
    // char * ptr5 = sf_realloc(ptr2, 200); // Reallocing with merging
    // *(ptr5) = 'A';
    // sf_show_blocks();
    // printf("\n");
    
    // char * ptr6 = sf_realloc(ptr4, 2300); //Reallocing with merging with the wilderness
    // *(ptr6) = 'A';
    // sf_show_blocks();
    // printf("\n");
    
    // char * ptrA = sf_memalign(50, 128); // Want a 50 byte block that is 128 aligned
    // *(ptrA) = 'A';
    // sf_show_blocks();
    // printf("\n");
    
    // char * ptrB = sf_memalign(1200, 32); // Want a 1200 byte block that is 32 byte aligned
    // *(ptrB) = 'A';
    // sf_show_blocks();
    // printf("\n");
    
    // sf_free(ptr1);
    // sf_show_blocks();
    // printf("\n");
    
    // char * ptrC = sf_memalign(200, 64); // Want a 200 byte block that is 64 aligned
    // *(ptrC) = 'A'; // No idea which block is which by this point sorry
    // sf_show_blocks();
    // printf("\n");
    
    
    // char * ptrD = sf_memalign(32, 32); // Want a 32 byte block that is 32 byte aligned
    // *(ptrD) = 'A';
    // sf_show_blocks();
    // printf("\n");
    
    // sf_show_free_lists();
    
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
    
    ptr2 = sf_realloc(ptr2, 500);
    sf_show_blocks();
    printf("\n");
    
    ptr3 = sf_realloc(ptr3, 12); // Should be just using the same block
    sf_show_blocks();
    printf("\n");
    
    char * ptr4 = sf_malloc(7000); // Allocate 7008 bytes 
    *(ptr4) = 'A'; // Should only have 48 bytes left
    sf_show_blocks();
    printf("\n");
    
    ptr3 = sf_realloc(ptr3, 10); // Should again use the same block now that it is not in fron tof wilderness
    sf_show_blocks();
    printf("\n");
    
    
    char * ptr5 = sf_realloc(ptr2, 200); // Reallocing with merging
    *(ptr5) = 'A';
    sf_show_blocks();
    printf("\n");
    
    char * ptr6 = sf_realloc(ptr4, 2300); //Reallocing with merging with the wilderness
    *(ptr6) = 'A';
    sf_show_blocks();
    printf("\n");
    
    char * ptrA = sf_memalign(50, 128); // Want a 50 byte block that is 128 aligned
    *(ptrA) = 'A';
    sf_show_blocks();
    printf("\n");
    
    char * ptrB = sf_memalign(1200, 32); // Want a 1200 byte block that is 32 byte aligned
    *(ptrB) = 'A';
    sf_show_blocks();
    printf("\n");
    
    sf_free(ptr1);
    sf_show_blocks();
    printf("\n");
    
    char * ptrC = sf_memalign(200, 64); // Want a 200 byte block that is 64 aligned
    *(ptrC) = 'A'; // No idea which block is which by this point sorry
    sf_show_blocks();
    printf("\n");
    
    
    sf_free(ptrC);
    sf_free(ptrB);
    sf_free(ptrA);
    sf_free(ptr6);
    sf_free(ptr5);
    sf_free(ptr3);
    
    sf_show_blocks();
    printf("\n");
    sf_show_free_lists();
    return EXIT_SUCCESS;
}
