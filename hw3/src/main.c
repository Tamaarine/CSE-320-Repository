#include <stdio.h>
#include "sfmm.h"
#include "helpingFunction.h"

int main(int argc, char const *argv[]) {
    
    // In the beginning only 8144 is wilderness
    char * p1 = sf_malloc(500); // Allocate 512 blocks
    *(p1) = 'A'; // 7632 left
    
    char * p2 = sf_malloc(2000); // Allocate 2016 blocks
    *(p2) = 'A'; // 5616
    
    char * p3 = sf_malloc(5600); // Allocate 5616 blocks
    *(p3) = 'A'; // 0

    sf_free(p1); // Should go into index 4 list
    sf_show_blocks();
    printf("\n");
    
    p1 = sf_malloc(300); // Allocate 320 blocks
    *(p1) = 'A'; // 192 left should go into index 3 list
    sf_show_blocks();
    printf("\n");
    
    sf_show_free_lists();
    
    sf_free(p2); // Freeing the 2016 block should go into index 6 list merge with 192, 2208
    sf_show_blocks();
    printf("\n");
    
    char * p4 = sf_malloc(1200); // Allocate 1216
    *(p4) = 'A'; // Left 992 should go to index 5 list
    sf_show_blocks();
    printf("\n");
    
    char * p5 = sf_malloc(472); // Allocate 480
    *(p5) = 'A'; // Left 512 should go to index 4 list
    sf_show_blocks();
    printf("\n");
    
    char * p6 = sf_malloc(504); // Allocate 512 used up all the free blocks right now
    *(p6) = 'A'; // Left 0
    sf_show_blocks();
    printf("\n");
    
    sf_free(p3); // Then free the last block in the heap which will result in wilderness with no merging
    sf_show_blocks();
    printf("\n");
    
    sf_free(p1); // Freeing the first block in the heap, should no merge with prologue and be placed in index 4
    sf_show_blocks();
    printf("\n");
    
    sf_free(p5); // Freeing the 480 block in the heap, which should also go into index 4
    sf_show_blocks();
    printf("\n");
    
    char * p7 = sf_malloc(500); // Allocating 512 block
    *(p7) = 'A';
    char * p8 = sf_malloc(500); // Allocating 512 block
    *(p8) = 'A';
    char * p9 = sf_malloc(500); // Allocating 512 block
    *(p9) = 'A';
    
    sf_show_blocks();
    printf("\n");
    
    sf_free(p7);
    sf_free(p9); // Should add 1 512 block into list 4 and the other one should merge with wilderness
    sf_show_blocks();
    printf("\n");
    
    char * p10 = sf_malloc(200); // Allocate 208 block should take from list 4, and place the remaining into list 4
    *(p10) = 'A';
    sf_show_blocks();
    printf("\n");
    
    char * p11 = sf_malloc(400); // Allocate 408 block, place remaining into list 1
    *(p11) = 'A';
    sf_show_blocks();
    printf("\n");
    
    char * p12 = sf_malloc(32); // Allocate 48 blocks
    *(p12) = 'A';
    char * p13 = sf_malloc(32); // Allocate 48 blocks
    *(p13) = 'A';
    char * p14 = sf_malloc(32); // Allocate 48 blocks
    *(p14) = 'A';
    char * p15 = sf_malloc(32); // Allocate 48 blocks
    *(p15) = 'A';
    
    sf_show_blocks();
    printf("\n");
    
    
    // Now we test each freeing case
    sf_free(p15); // Free with coalesce with only next
    sf_show_blocks();
    printf("\n");
    
    sf_free(p8); // Free with coalesce with prev and next
    sf_show_blocks();
    printf("\n");
    
    sf_show_free_lists();
    
    return EXIT_SUCCESS;
}
