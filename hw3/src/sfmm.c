/**
 * Do not submit your assignment with a main function in this file.
 * If you submit with a main function in this file, you will get a zero.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "debug.h"
#include "sfmm.h"
#include "helpingFunction.h"

void *sf_malloc(size_t size) {
    
    // First before we begin we must check whether or not start and end of
    // the heap are equal to each other if they are, then we have to call
    // mem_grow because we are initializing the heap
    if(sf_mem_start() == sf_mem_end())
    {
        char * startAddress = sf_mem_grow(); 
        
        // Get us to the prologue address by adding 8 byte
        sf_block * prologue = (sf_block*)(startAddress + 8);
        prologue->header = 32 | THIS_BLOCK_ALLOCATED; // Minimal of 32 byte and allocation status of 1
        
        // Get us to the epilogue address by subtracting 8 byte
        sf_header * epilogue = (sf_header *)((char *)sf_mem_end() - 8);
        *(epilogue) = 0 | THIS_BLOCK_ALLOCATED; // The epilogue have size of 0 and allocation status of 1
        
        // We also have to initialize the sf_free_list_heads
        for(int i=0;i<NUM_FREE_LISTS;i++)
        {
            sf_free_list_heads[i].body.links.next = &sf_free_list_heads[i];
            sf_free_list_heads[i].body.links.prev = &sf_free_list_heads[i];
        }
        
        sf_block * firstFreeBlock = (sf_block *)((char *)prologue + 32);
        firstFreeBlock->header = 8144; // Need to change this later for sure, definitely not 8144 byte 100%
        firstFreeBlock->body.links.next = NULL;
        firstFreeBlock->body.links.prev = NULL;
        
        // Get to the next block minus 8 to get to the footer
        sf_header * footer = (sf_header *)((char *)firstFreeBlock + 8144 - 8);
        *(footer) = 8144;
        
        // Add the remaining back to wilderness and set the links appropriately
        sf_free_list_heads[7].body.links.next = firstFreeBlock;
        sf_free_list_heads[7].body.links.prev = firstFreeBlock;
        firstFreeBlock->body.links.next = &sf_free_list_heads[7];
        firstFreeBlock->body.links.prev = &sf_free_list_heads[7];
        
        // printf("heap start at %p ends at %p\n", sf_mem_start(), sf_mem_end());
        // printf("prologue address is %p the length is %ld\n", prologue, prologue->header);
        // printf("epilogue address is %p the length is %ld\n", epilogue, *epilogue);
        // printf("wilderness block start at %p ends at %p\n", sf_free_list_heads[7].body.links.next, (void *)sf_free_list_heads[7].body.links.next + sf_free_list_heads[7].body.links.next->header);
        // printf("footer is at %p", footer);
    }
    
    // // If the size are less than or equal to 0 we will just return 0
    if(size <= 0)
        return NULL;
        
    // First determine the adjusted size
    size_t adjustedSize = 0;
    
    if(size <= 24)
        adjustedSize = 32; // If it is just less than or equal to 24 byte, make it 32 byte
    else
    {
        // Else we have to add 8 and round it up to the nearest multiple of 16
        adjustedSize = computeMemorySize(size + 8);
    }    
    
    // Then we call computeMemoryIndex to calculate the index from where we start to look for
    // the free_lists
    int startSearchIndex = computeMemoryIndex(adjustedSize);
    
    
    // Set an output pointer
    void * outputPtr = NULL;
    
    // Will be used for finding the suitable free_list head
    sf_block * suitableListHead = NULL;
    
    // We start searching through the free list for an nonempty list
    for(int i=startSearchIndex;i<NUM_FREE_LISTS;i++)
    {
        sf_block * currentFreeList = &sf_free_list_heads[i];
        // Before sf_block currentFreeList = sf_free_list_heads[i]; and I tried to compare the address
        // of a variable with completely different address using currentFreeList.body.links.next != currentFreeList
        // which will never be true since currentFreeList is the address of a local variable
        
        // This means that the freelist is non-empty hence we will see if we can find any suitable free_list
        if(currentFreeList->body.links.next != currentFreeList)
        {
            // Then we will have to traverse the doubly-linked list to see if we can
            // find a suitable block to allocate the block
            sf_block * nodePtr = currentFreeList->body.links.next;
            
            // While the current node pointer is not equal to the dummynode we will keep search
            while(nodePtr != currentFreeList)
            {
                // Get the length of the free block by masking
                size_t blockLength = nodePtr->header >> 4; // Shift right 4
                blockLength <<= 4; // Shift left 4
                
                if(adjustedSize <= blockLength)
                {
                    // The size we need is less than or equal to this block's length, perfect
                    // we will take this one because it is first-fit policy
                    suitableListHead = nodePtr;
                    printf("found suitable at list %d with length of %ld\n", i, blockLength);
                    // And we break
                    break;
                }
                
                // Goes to the next node
                nodePtr = nodePtr->body.links.next;
            }
            
            // If we found a suitableListHead from the previous while loop then
            // that means we have found what we needed we can just break from the for loop
            // no need to look for more. But if we didn't find it then we will have to keep searching through
            // the entire 7 list
            if(suitableListHead != NULL)
            {
                break;
            }
            
            // SKip this case for now to do some unit testing
            // If we are here after looking through the wilderness this means 
            // the wilderness block that we have/not have didn't fit hence we need
            // to expand the memory until we get a big enough block which satisfy the request
            // if(i == 7 && suitableListHead == NULL)
            // {
                
            // }
        }
        
        // Do the i == 7 case right here because it will also handle the case
        // if the last free list have no wilderness at all as well
    }
    
    // Now if we are here then that means we hopefully have found the
    // free_block from one of the free_lists to allocate our wanted blocks from in suitableListHead
    // We will determine whether or not we will split the free_block
    int haveSplinter = leaveSplinter(suitableListHead, adjustedSize);
    
    // If it will leave splinter, we just take the entire block
    if(haveSplinter)
    {
        // suitableListHead is what we will be returning be we have to add 8 bytes
        // to point it to the actual payload
        outputPtr = (char *)suitableListHead + 8; 
        
        // Then we cut ties with the family/nodes
        sf_block * prevBlock = suitableListHead->body.links.prev;
        sf_block * nextBlock = suitableListHead->body.links.next;
        prevBlock->body.links.next = nextBlock;
        nextBlock->body.links.prev = prevBlock;
        
        // We have to set the allocation status to 1 on the header
        suitableListHead->header = suitableListHead->header | THIS_BLOCK_ALLOCATED;
    }
    else
    {
        // If it doesn't leave splinter we will split the block
        int leftOver = suitableListHead->header - adjustedSize;
        
        // suitableListHead is what we will be returning 
        outputPtr = (char *)suitableListHead + 8;
        //Set the size of the header and allocation status as well
        suitableListHead->header = adjustedSize | THIS_BLOCK_ALLOCATED;
        
        // To calculate where free_block of the splitted block are
        // we just need to add adjustedSize to suitableListHead
        sf_block * remainingBlock = (sf_block *)((char *)suitableListHead + adjustedSize);
        // Set the size of the remainingBlock
        remainingBlock->header = leftOver;
        
        // We also have to adjust the footer of the remainingBlock as well
        sf_footer * remainingBlockFooter = (sf_footer *)((char *)remainingBlock + leftOver - 8);
        *(remainingBlockFooter) = leftOver;
        
        // remainingBlock is what we will be adding back to the list 
        sf_block * prevBlock = suitableListHead->body.links.prev;
        sf_block * nextBlock = suitableListHead->body.links.next;
        prevBlock->body.links.next = remainingBlock;
        nextBlock->body.links.prev = remainingBlock;
        
        // Then we also have to set the links for the remainingBlock
        remainingBlock->body.links.prev = prevBlock;
        remainingBlock->body.links.next = nextBlock;
    }
    
    // Then we finally can return the pointer    
    return outputPtr;
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
