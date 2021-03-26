#include <criterion/criterion.h>
#include <errno.h>
#include <signal.h>
#include "debug.h"
#include "sfmm.h"
#define TEST_TIMEOUT 15

/*
 * Assert the total number of free blocks of a specified size.
 * If size == 0, then assert the total number of all free blocks.
 */
void assert_free_block_count(size_t size, int count) {
    int cnt = 0;
    for(int i = 0; i < NUM_FREE_LISTS; i++) {
	sf_block *bp = sf_free_list_heads[i].body.links.next;
	while(bp != &sf_free_list_heads[i]) {
	    if(size == 0 || size == (bp->header & ~0xf)) {
		cnt++;
	    }
	    bp = bp->body.links.next;
	}
    }
    if(size == 0) {
	cr_assert_eq(cnt, count, "Wrong number of free blocks (exp=%d, found=%d)",
		     count, cnt);
    } else {
	cr_assert_eq(cnt, count, "Wrong number of free blocks of size %ld (exp=%d, found=%d)",
		     size, count, cnt);
    }
}

Test(sfmm_basecode_suite, malloc_an_int, .timeout = TEST_TIMEOUT) {
	sf_errno = 0;
	size_t sz = 4;
	int *x = sf_malloc(sz);

	cr_assert_not_null(x, "x is NULL!");

	*x = 4;

	cr_assert(*x == 4, "sf_malloc failed to give proper space for an int!");

	assert_free_block_count(0, 1);
	assert_free_block_count(8112, 1);

	cr_assert(sf_errno == 0, "sf_errno is not zero!");
	cr_assert(sf_mem_start() + 8192 == sf_mem_end(), "Allocated more than necessary!");
}

Test(sfmm_basecode_suite, malloc_four_pages, .timeout = TEST_TIMEOUT) {
	sf_errno = 0;

	// We want to allocate up to exactly four pages, so there has to be space
	// for the header, footer, and the link pointers.
	void *x = sf_malloc(32704);
	cr_assert_not_null(x, "x is NULL!");
	assert_free_block_count(0, 0);
	cr_assert(sf_errno == 0, "sf_errno is not 0!");
}

Test(sfmm_basecode_suite, malloc_too_large, .timeout = TEST_TIMEOUT) {
	sf_errno = 0;
	void *x = sf_malloc(524288);

	cr_assert_null(x, "x is not NULL!");
	assert_free_block_count(0, 1);
	assert_free_block_count(131024, 1);
	cr_assert(sf_errno == ENOMEM, "sf_errno is not ENOMEM!");
}

Test(sfmm_basecode_suite, free_no_coalesce, .timeout = TEST_TIMEOUT) {
	sf_errno = 0;
	size_t sz_x = 8, sz_y = 200, sz_z = 1;
	/* void *x = */ sf_malloc(sz_x);
	void *y = sf_malloc(sz_y);
	/* void *z = */ sf_malloc(sz_z);

	sf_free(y);

	assert_free_block_count(0, 2);
	assert_free_block_count(208, 1);
	assert_free_block_count(7872, 1);
	cr_assert(sf_errno == 0, "sf_errno is not zero!");
}

Test(sfmm_basecode_suite, free_coalesce, .timeout = TEST_TIMEOUT) {
	sf_errno = 0;
	size_t sz_w = 8, sz_x = 200, sz_y = 300, sz_z = 4;
	/* void *w = */ sf_malloc(sz_w);
	void *x = sf_malloc(sz_x);
	void *y = sf_malloc(sz_y);
	/* void *z = */ sf_malloc(sz_z);

	sf_free(y);
	sf_free(x);

	assert_free_block_count(0, 2);
	assert_free_block_count(528, 1);
	assert_free_block_count(7552, 1);
	cr_assert(sf_errno == 0, "sf_errno is not zero!");
}

Test(sfmm_basecode_suite, freelist, .timeout = TEST_TIMEOUT) {
        size_t sz_u = 200, sz_v = 300, sz_w = 200, sz_x = 500, sz_y = 200, sz_z = 700;
	void *u = sf_malloc(sz_u);
	/* void *v = */ sf_malloc(sz_v);
	void *w = sf_malloc(sz_w);
	/* void *x = */ sf_malloc(sz_x);
	void *y = sf_malloc(sz_y);
	/* void *z = */ sf_malloc(sz_z);

	sf_free(u);
	sf_free(w);
	sf_free(y);

	assert_free_block_count(0, 4);
	assert_free_block_count(208, 3);
	assert_free_block_count(5968, 1);

	// First block in list should be the most recently freed block.
	int i = 3;
	sf_block *bp = sf_free_list_heads[i].body.links.next;
	cr_assert_eq(bp, (char *)y - 8,
		     "Wrong first block in free list %d: (found=%p, exp=%p)",
                     i, bp, (char *)y - 8);
}

Test(sfmm_basecode_suite, realloc_larger_block, .timeout = TEST_TIMEOUT) {
        size_t sz_x = sizeof(int), sz_y = 10, sz_x1 = sizeof(int) * 20;
	void *x = sf_malloc(sz_x);
	/* void *y = */ sf_malloc(sz_y);
	x = sf_realloc(x, sz_x1);

	cr_assert_not_null(x, "x is NULL!");
	sf_block *bp = (sf_block *)((char *)x - 8);
	cr_assert(bp->header & 0x1, "Allocated bit is not set!");
	cr_assert((bp->header & ~0xf) == 96,
		  "Realloc'ed block size (0x%ld) not what was expected (0x%ld)!",
		  bp->header & ~0xf, 96);

	assert_free_block_count(0, 2);
	assert_free_block_count(32, 1);
	assert_free_block_count(7984, 1);
}

Test(sfmm_basecode_suite, realloc_smaller_block_splinter, .timeout = TEST_TIMEOUT) {
        size_t sz_x = 80, sz_y = 64;
	void *x = sf_malloc(sz_x);
	void *y = sf_realloc(x, sz_y);

	cr_assert_not_null(y, "y is NULL!");
	cr_assert(x == y, "Payload addresses are different!");

	sf_block *bp = (sf_block *)((char *)x - 8);
	cr_assert(bp->header & 0x1, "Allocated bit is not set!");
	cr_assert((bp->header & ~0xf) == 96, "Block size not what was expected!");

	// There should be only one free block.
	assert_free_block_count(0, 1);
	assert_free_block_count(8048, 1);
}

Test(sfmm_basecode_suite, realloc_smaller_block_free_block, .timeout = TEST_TIMEOUT) {
        size_t sz_x = 64, sz_y = 8;
	void *x = sf_malloc(sz_x);
	void *y = sf_realloc(x, sz_y);

	cr_assert_not_null(y, "y is NULL!");

	sf_block *bp = (sf_block *)((char *)x - 8);
	cr_assert(bp->header & 0x1, "Allocated bit is not set!");
	cr_assert((bp->header & ~0xf) == 32, "Realloc'ed block size not what was expected!");

	// After realloc'ing x, we can return a block of size 48
	// to the freelist.  This block will go into the main freelist and be coalesced.
	assert_free_block_count(0, 1);
	assert_free_block_count(8112, 1);
}
//############################################
//STUDENT UNIT TESTS SHOULD BE WRITTEN BELOW
//DO NOT DELETE THESE COMMENTS
//############################################

// Test(sfmm_basecode_suite, ranout_of_memory, .timeout = TEST_TIMEOUT)
// {
// 	int * ptrs[16];
	
// 	for(int i=0;i<16;i++)
// 	{
// 		ptrs[i] = sf_malloc(9000);
// 	}
	
// 	for(int i=0;i<16;i++)
// 	{
// 		*(ptrs[i]) = 5;
// 	}
// 	printf("%p\n", ptrs[15]);
// 	cr_assert(ptrs[15] == NULL, "ENOMEM is not set due to memory ran out");
// }

Test(sfmm_basecode_suite, sf_memalign_test, .timeout = TEST_TIMEOUT)
{
	char * ptr1 = sf_memalign(50, 64);
    *(ptr1) = '3';
    
    char * ptr2 = sf_memalign(900, 128);
    *(ptr2) = '3';
    
    char * ptr3= sf_memalign(12800, 128);
    *(ptr3) = '5';
    
    char * ptr4= sf_memalign(120, 128);
    *(ptr4) = '5';
    
    char * ptr5= sf_memalign(360, 2048);
    *(ptr5) = '5';
    
	cr_assert((size_t)ptr1 % 64 == 0, "ptr1 is not aligned with the requested alignment");
	cr_assert((size_t)ptr2 % 128 == 0, "ptr2 is not aligned with the requested alignment");
	cr_assert((size_t)ptr3 % 128 == 0, "ptr3 is not aligned with the requested alignment");
	cr_assert((size_t)ptr4 % 128 == 0, "ptr4 is not aligned with the requested alignment");
	cr_assert((size_t)ptr5 % 2048 == 0, "ptr5 is not aligned with the requested alignment");
	
	sf_free(ptr1);
	sf_free(ptr2);
	sf_free(ptr3);
	sf_free(ptr4);
	sf_free(ptr5);
	
	assert_free_block_count(0, 1);
}

Test(sfmm_basecode_suite, sf_realloc_merge_withwilderness, .timeout = TEST_TIMEOUT)
{
	char * ptr1 = sf_malloc(200);
    *(ptr1) = 'A';
    
    char * ptr2 = sf_malloc(600 * sizeof(double));
    *(ptr2) = 'A';
    
    char * ptr3 = sf_malloc(666);
    *(ptr3) = 'A';
    
    char * ptr4 = sf_malloc(999);
    *(ptr4) = 'A';
    
    ptr4 = sf_realloc(ptr4, 123);
	
	assert_free_block_count(2288, 1);
}

Test(sfmm_basecode_suite, sf_free_merge_next_and_prev, .timeout = TEST_TIMEOUT)
{
	char * p1 = sf_malloc(200);
	*(p1) = 'A';
	char * p2 = sf_malloc(666);
	*(p2) = 'A';
	char * p3 = sf_malloc(12);
	*(p3) = 'A';
	char * p4 = sf_malloc(32);
	*(p4) = 'A';
	char * p5 = sf_malloc(9999);
	*(p5) = 'A';
	
	sf_free(p1);
	sf_free(p3);
	
	sf_free(p2);
	
	assert_free_block_count(928, 1);
	assert_free_block_count(5344, 1);
}

Test(sfmm_basecode_suite, sf_alloc_correct_list, .timeout = TEST_TIMEOUT)
{
	char * p1 = sf_malloc(200);
	*(p1) = 'A';
	char * p2 = sf_malloc(666);
	*(p2) = 'A';
	char * p3 = sf_malloc(12);
	*(p3) = 'A';
	char * p4 = sf_malloc(32);
	*(p4) = 'A';
	char * p5 = sf_malloc(9999);
	*(p5) = 'A';
	
	sf_free(p1);
	sf_free(p3);
	
	sf_free(p2); // A 928 byte block at list 5
	
	p1 = sf_malloc(500); // Allocating a 512 byte block, the left over 416 should be in list 4
	
	assert_free_block_count(416, 1);
	assert_free_block_count(5344, 1);
}

Test(sfmm_basecode_suite, sf_memalign_testing, .timeout = TEST_TIMEOUT)
{
	char * p1 = sf_malloc(200);
	*(p1) = 'A';
	char * p2 = sf_malloc(666);
	*(p2) = 'A';
	char * p3 = sf_malloc(12);
	*(p3) = 'A';
	char * p4 = sf_malloc(32);
	*(p4) = 'A';
	char * p5 = sf_malloc(9999);
	*(p5) = 'A';
	
	sf_free(p2);
	sf_free(p4);
	
    p2 = sf_memalign(90, 128);
	
	cr_assert((size_t)p2 % 128 == 0 , "Not memaligned");
	
	sf_free(p1);
	sf_free(p2);
	sf_free(p3);
	sf_free(p5);
	
	assert_free_block_count(0, 1);
}

Test(sfmm_basecode_suite, bunch_of_allocate_and_free, .timeout = TEST_TIMEOUT)
{
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
	
	cr_assert((sf_free_list_heads[0].body.links.next->header >> 4 << 4) == 32, "Size doesn't match error");
	cr_assert((sf_free_list_heads[1].body.links.next->header >> 4 << 4) == 48, "Size doesn't match error");
	cr_assert((sf_free_list_heads[2].body.links.next->header >> 4 << 4) == 80, "Size doesn't match error");
	cr_assert((sf_free_list_heads[3].body.links.next->header >> 4 << 4) == 160, "Size doesn't match error");
	cr_assert((sf_free_list_heads[4].body.links.next->header >> 4 << 4) == 288, "Size doesn't match error");
	cr_assert((sf_free_list_heads[5].body.links.next->header >> 4 << 4) == 720, "Size doesn't match error");
	cr_assert((sf_free_list_heads[6].body.links.next->header >> 4 << 4) == 2016, "Size doesn't match error");
	
	
	assert_free_block_count(0, 8);
}

Test(sfmm_basecode_suite, run_out_of_memory, .timeout = TEST_TIMEOUT)
{
	char * ptrs[16];
	
	for(int i=0;i<16;i++)
	{
		ptrs[i] = sf_malloc(12999);
		if(ptrs[i] != NULL)
			*(ptrs[i]) = 'A';
	}
	
	cr_assert(sf_errno == ENOMEM, "Didn't run out of memory");
}

