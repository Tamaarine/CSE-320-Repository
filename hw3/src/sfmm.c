/**
 * Do not submit your assignment with a main function in this file.
 * If you submit with a main function in this file, you will get a zero.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "debug.h"
#include "sfmm.h"

void *sf_malloc(size_t size) {
    
    // First before we begin we must check whether or not start and end of
    // the heap are equal to each other if they are, then we have to call
    // mem_grow because we are initializing the heap
    if(sf_mem_start() == sf_mem_end())
    {
        void * startAddress = sf_mem_grow(); 
        
        // Get us to the prologue address by adding 8 byte
        sf_block * prologue = (void *)startAddress + 8;
        prologue->header = 32 | THIS_BLOCK_ALLOCATED; // Minimal of 32 byte and allocation status of 1
        
        // Get us to the epilogue address by subtracting 8 byte
        sf_header * epilogue = (void *)sf_mem_end() - 8;
        *(epilogue) = 0 | THIS_BLOCK_ALLOCATED; // The epilogue have size of 0 and allocation status of 1
        
        // We also have to initialize the sf_free_list_heads
        for(int i=0;i<NUM_FREE_LISTS;i++)
        {
            sf_free_list_heads[i].body.links.next = &sf_free_list_heads[i];
            sf_free_list_heads[i].body.links.prev = &sf_free_list_heads[i];
        }
        
        sf_block * firstFreeBlock = (void *)prologue + 32;
        firstFreeBlock->header = 8144; // Need to change this later for sure, definitely not 8144 byte 100%
        firstFreeBlock->body.links.next = NULL;
        firstFreeBlock->body.links.prev = NULL;
        
        // Get to the next block minus 8 to get to the footer
        sf_header * footer = ((void *)firstFreeBlock) + 8144 - 8;
        *(footer) = 8144;
        
        // Add the remaining back to wilderness
        sf_free_list_heads[7].body.links.next = firstFreeBlock;
        
        // printf("heap start at %p ends at %p\n", sf_mem_start(), sf_mem_end());
        // printf("prologue address is %p the length is %ld\n", prologue, prologue->header);
        // printf("epilogue address is %p the length is %ld\n", epilogue, *epilogue);
        // printf("wilderness block start at %p ends at %p\n", sf_free_list_heads[7].body.links.next, (void *)sf_free_list_heads[7].body.links.next + sf_free_list_heads[7].body.links.next->header);
        // printf("footer is at %p", footer);
    }
    
    // // If the size are less than or equal to 0 we will just return 0
    if(size <= 0)
        return NULL;
        
    // However, if the size is valid then we will have to determine which free_list we will be
    // searching from. Call helping function to do this for us
    
    
    return NULL;
}

void sf_free(void *ptr)
{
    return;
}

void *sf_realloc(void *ptr, size_t size)
{
    return NULL;
}

void *sf_memalign(size_t size, size_t align) 
{
    return NULL;
}
