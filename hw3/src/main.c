#include <stdio.h>
#include "sfmm.h"
#include "helpingFunction.h"

int main(int argc, char const *argv[]) {
    
    char * p1 = sf_malloc(12);
	*(p1) = 'A';
	char * p2 = sf_malloc(12);
	*(p2) = 'A';
	char * p3 = sf_malloc(40);
	*(p3) = 'A';
	char * p4 = sf_malloc(40);
	*(p4) = 'A';
	char * p5 = sf_malloc(67);
	*(p5) = 'A';
	char * p6 = sf_malloc(67);
	*(p6) = 'A';
	char * p7 = sf_malloc(150);
	*(p7) = 'A';
	char * p8 = sf_malloc(150);
	*(p8) = 'A';
	char * p9 = sf_malloc(270);
	*(p9) = 'A';
	char * p10 = sf_malloc(270);
	*(p10) = 'A';
	char * p11 = sf_malloc(700);
	*(p11) = 'A';
	char * p12 = sf_malloc(700);
	*(p12) = 'A';
	char * p13 = sf_malloc(2000);
	*(p13) = 'A';
	char * p14 = sf_malloc(2000);
	*(p14) = 'A';
    
    sf_free(p1);
	sf_free(p3);
	sf_free(p5);
	sf_free(p7);
	sf_free(p9);
	sf_free(p11);
	sf_free(p13);
    
    sf_show_blocks();
    printf("\n");
    
    sf_show_free_lists();
    
    return EXIT_SUCCESS;
}
