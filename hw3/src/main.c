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
    
    // int * ptr5 = sf_malloc(9000);
    // if(ptr5 != NULL)
    // {
    //     *(ptr5) = 'A';
    // }
    
    // sf_show_blocks();  
    // printf("\n"); 
        
    
    // int * ptr6 = sf_malloc(7368);
    // *(ptr6) = 3;
    // sf_show_blocks();  
    // printf("\n"); 
    
    // int * ptr7 = sf_malloc(8192);
    // *(ptr7) = 3;
    // sf_show_blocks();  
    // printf("\n"); 
    
    // int * ptr8 = sf_malloc(81920);
    // *(ptr8) = 3;
    // sf_show_blocks();  
    // printf("\n"); 
    
    // int * ptr9 = sf_malloc(8121);
    // *(ptr9) = 3;
    // sf_show_blocks();  
    // printf("\n"); 
    
    // sf_free(ptr9);  
    // sf_show_blocks();  
    // printf("\n"); 
    
    // sf_free(ptr7);  
    // sf_show_blocks();  
    // printf("\n"); 
    
    // sf_free(ptr5);  
    // sf_show_blocks();  
    // printf("\n"); 
    
    // // // char * ptr1 = sf_malloc(131064);
    // // // if(ptr1 != NULL)
    // // //     *(ptr1) = 'A';
    
    // sf_show_free_lists();  
    // printf("\n"); 
    
    // sf_free(ptr8);  
    // sf_show_blocks();  
    // printf("\n"); 
    
    // sf_free(ptr6);  
    // sf_show_blocks();  
    // printf("\n"); 
    
    // sf_free(ptr1);  
    // sf_show_blocks();  
    // printf("\n"); 
    
    // sf_free(ptr2);  
    // sf_show_blocks();  
    // printf("\n"); 
    
    // sf_free(ptr3);  
    // sf_show_blocks();  
    // printf("\n"); 
    
    // sf_free(ptr4);  
    // sf_show_blocks();  
    // printf("\n"); 
    
    // sf_show_free_lists();  
    // printf("\n"); 
    
    
    
    
    printf("%ld\n", sizeof(double));
    
    double* ptr1 = sf_malloc(6 * sizeof(double));
    printf("\n&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&\n");
    sf_show_heap();
	printf("\n&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&\n");
    *ptr1 = 320320320e-320;
    printf("%f\n", *ptr1);

    double* ptr2 = sf_malloc(6 * sizeof(double));
    printf("\n&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&\n");
    sf_show_heap();
	printf("\n&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&\n");
    //*ptr1 = 320320320e-320;
    //printf("%f\n", *ptr1);

    double* ptr3 = sf_malloc(200 * sizeof(double));
    printf("\n&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&\n");
    sf_show_heap();
	printf("\n&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&\n");
    *ptr3 = 320320320e-320;
    printf("The number is %f\n", *ptr3);

    sf_free(ptr2);
    printf("\n&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&\n");
    sf_show_heap();
	printf("\n&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&\n");

   	double* ptr4 = sf_malloc(150 * sizeof(double));
   	*ptr4 = 320320320e-320;
    printf("%f\n", *ptr4);
   	printf("\n&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&\n");
    sf_show_heap();
	printf("\n&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&\n");

	sf_free(ptr1);
	printf("\n&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&\n");
    sf_show_heap();
	printf("\n&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&\n");

	double* ptr5 = sf_malloc(sizeof(double));
	*ptr5 = 320320320e-320;
    printf("%f\n", *ptr5);
   	printf("\n&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&\n");
    sf_show_heap();
	printf("\n&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&\n");

	double* ptr6 = sf_malloc(sizeof(double));
	*ptr6 = 320320320e-320;
    printf("%f\n", *ptr6);
   	printf("\n&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&\n");
    sf_show_heap();
	printf("\n&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&\n");

	double* ptr7 = sf_malloc(sizeof(double));
	*ptr7 = 320320320e-320;
    printf("%f\n", *ptr7);
   	printf("\n&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&\n");
    sf_show_heap();
	printf("\n&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&\n");
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    // double* ptr = sf_malloc(sizeof(double));

    // *ptr = 320320320e-320;

    // printf("%f\n", *ptr);

    // sf_free(ptr);

    return EXIT_SUCCESS;
}
