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
#include <errno.h>

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
        setNewEpilogue();
        // sf_header * epilogue = (sf_header *)((char *)sf_mem_end() - 8);
        // *(epilogue) = 0 | THIS_BLOCK_ALLOCATED; // The epilogue have size of 0 and allocation status of 1
        
        // We also have to initialize the sf_free_list_heads
        for(int i=0;i<NUM_FREE_LISTS;i++)
        {
            sf_free_list_heads[i].body.links.next = &sf_free_list_heads[i];
            sf_free_list_heads[i].body.links.prev = &sf_free_list_heads[i];
        }
        
        sf_block * firstFreeBlock = (sf_block *)((char *)prologue + 32);
        firstFreeBlock->header = 8144 | PREV_BLOCK_ALLOCATED; 
        firstFreeBlock->body.links.next = NULL;
        firstFreeBlock->body.links.prev = NULL;
        
        // Get to the next block minus 8 to get to the footer
        sf_header * footer = (sf_header *)((char *)firstFreeBlock + 8144 - 8);
        *(footer) = firstFreeBlock->header;
        
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
        }
        
        // Now if i becomes 7 and if we are here
        // then that means either the allocator couldn't find a free block anywhere
        // or the wilderness is empty or is just too small to satisfy the request
        // hence we will be performing mem_grow again
        if(i == 7)
        {
            // We will have tocall mem_grow at least once for sure
            char * extendedHeap = sf_mem_grow();
            
            if(extendedHeap == NULL)
            {
                // If we couldn't grow our heap then we will set sf_errno to ENOMEM
                // and we will return NULL
                sf_errno = ENOMEM;
                return NULL;
            }
            
            // However if we didn't get an error trying to get more memory then we will try to
            // merge them together and see if we can satisfy the request
            
            // We see if we can get the previosu wilderness
            sf_block * prevWilderness = sf_free_list_heads[i].body.links.next;
            
            // This means that the wilderness is empty to begin with
            // we will merge the old epilogue with the newly allocated heap
            if(prevWilderness == &sf_free_list_heads[i])
            {
                // This gets us to the previous old epilogue
                sf_block * mergedWilderness = (sf_block *)((char *)extendedHeap - 8);
                mergedWilderness->header = PAGE_SZ | PREV_BLOCK_ALLOCATED; // The old wilderness is gone hence it must be allocated
                
                size_t length = mergedWilderness->header >> 4;
                length <<= 4;
                
                sf_footer * mergedFooter = (sf_footer *)((char *)mergedWilderness + length - 8);
                *(mergedFooter) = mergedWilderness->header;
                
                // Then we have to set the new epilogue at the end of the heap
                setNewEpilogue();
                
                // Then we put it back in the wilderness free_list
                prevWilderness->body.links.next = mergedWilderness;
                prevWilderness->body.links.prev = mergedWilderness;
                mergedWilderness->body.links.next = prevWilderness;
                mergedWilderness->body.links.prev = prevWilderness;
            }
            else
            {            
                // This means that the wilderness has a free_block but isn't big enough to satisfy the reques
                // we will have to merge the old wilderness with the new wilderness and make the new epilogue
                // All we have to do is to modify the length that is stored
                size_t wildernessLength = prevWilderness->header >> 4;
                wildernessLength <<= 4;
                
                // Get the prev_status of the previous block
                int prevAllocateStatus = prevWilderness->header & PREV_BLOCK_ALLOCATED;
                
                // We create the new header with the combined length and the inheribited previous status
                prevWilderness->header = (wildernessLength + PAGE_SZ) | prevAllocateStatus;
                
                wildernessLength = prevWilderness->header >> 4;
                wildernessLength <<= 4;
                
                // Set the footer of that block
                sf_footer * mergedFooter = (sf_footer *)((char *)prevWilderness + wildernessLength - 8);
                *(mergedFooter) = prevWilderness->header;
                
                // We also have to set the new epilogue
                setNewEpilogue();
            }
            
            // After finish merging we will have to still do some loops
            // what if one request wasn't enough to satisfy the request
            sf_block * wilderness = sf_free_list_heads[7].body.links.next;
            size_t blockSize = wilderness->header >> 4;
            blockSize <<= 4;
            
            // The wilderness is enough after just one growth we will just return
            if(adjustedSize <= blockSize)
            {
                suitableListHead = wilderness;
                
                break;
            }
            
            // However if we are here then that means that one growth wasn't enough we will
            // have to do a while loop in order to get it enough
            while(adjustedSize > blockSize)
            {
                // Call mem_growth
                char * newHeapAddress = sf_mem_grow();
                
                if(newHeapAddress == NULL)
                {
                    // Ultimately can't satisfy the request hence we will return error
                    sf_errno = ENOMEM;
                    return NULL;
                }
                
                // Then we will have to merge the wilderness together
                size_t length = wilderness->header >> 4; // Get the length of wilderness
                length <<= 4;
                
                // Get the prev status of the wilderness that we are merging with
                int prevAllocateStatus = wilderness->header & PREV_BLOCK_ALLOCATED;
                
                // Create the new header with combined size
                wilderness->header = (length + PAGE_SZ) | prevAllocateStatus;
                
                // Get the new length again in order to set the footer
                length = wilderness->header >> 4;
                length <<= 4;
                
                // Set the new footer
                sf_footer * mergedFooter = (sf_footer *)((char *)wilderness + length - 8);
                *(mergedFooter) = wilderness->header;
                
                // Then we redo the epilogue
                setNewEpilogue();
                
                // And update blockSize
                blockSize = wilderness->header >> 4;
                blockSize <<= 4;
            }
            
            // Now if we are outside then that means we ultiamtely got a block size that is big enough to
            // satisfy the user request
            suitableListHead = wilderness;
            
            break; // Then finally break
        }
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
        // The prev is inherited from the header so we don't have to explicitly do it
        
        size_t length = suitableListHead->header >> 4;
        length <<= 4;
        
        // But we have to also tell the next block that this block is allocated
        sf_block * blockAfter = (sf_block *)((char *)suitableListHead + length);
        blockAfter->header = blockAfter->header | PREV_BLOCK_ALLOCATED; // Have to set the previous block to be allocated
    }
    else
    {
        // If it doesn't leave splinter we will split the block
        size_t headerLength = suitableListHead->header >> 4;
        headerLength <<= 4;
        size_t leftOver = headerLength - adjustedSize;
        
        // This tells us the prev_allocation status of the previous block
        // which we will have to give to outputPtr
        int inheritPrevAllocate = suitableListHead->header & PREV_BLOCK_ALLOCATED;
        
        // outputPtr is what we will be returning 
        outputPtr = (char *)suitableListHead + 8;
        //Set the size of the header and allocation status as well
        suitableListHead->header = adjustedSize | THIS_BLOCK_ALLOCATED;
        suitableListHead->header = suitableListHead->header | inheritPrevAllocate;
        
        
        // To calculate where free_block of the splitted block are
        // we just need to add adjustedSize to suitableListHead
        sf_block * remainingBlock = (sf_block *)((char *)suitableListHead + adjustedSize);
        // Set the size of the remainingBlock
        remainingBlock->header = leftOver | PREV_BLOCK_ALLOCATED;
        
        // We also have to adjust the footer of the remainingBlock as well
        sf_footer * remainingBlockFooter = (sf_footer *)((char *)remainingBlock + leftOver - 8);
        *(remainingBlockFooter) = remainingBlock->header;
        
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
