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
    // We can get the blockPtr by going back 8 bytes from the ptr
    sf_block * blockPtr = (sf_block *)((char *)ptr - 8);
    
    size_t blockLength = getSizeFromHeader(blockPtr->header);
    
    sf_footer * blockPtrFooter = (sf_footer *)((char *)blockPtr + blockLength - 8);
    
    int allocatedByte = blockPtr->header & THIS_BLOCK_ALLOCATED;
    int prevAllocateBit = blockPtr->header & PREV_BLOCK_ALLOCATED;
    
    char * blockPtrEnd = (char *)blockPtr + blockLength;
    
    
    // Add 40 to get to the actual valid beginning of address
    char * memStart = (char *)sf_mem_start() + 40;
    
    // Subtract 8 to get to the starting address of the epilogue
    char * memEnd = (char *)sf_mem_end() - 8;
    
    // Information about the nextBlock because we can always find it
    sf_block * nextBlockPtr = (sf_block *)((char *)blockPtr + blockLength);
    
    size_t nextBlockLength = getSizeFromHeader(nextBlockPtr->header);
    
    sf_footer * nextBlockPtrFooter = (sf_footer *)((char *)nextBlockPtr + nextBlockLength - 8);
    
    int nextBlockAllocatedBit = nextBlockPtr->header & THIS_BLOCK_ALLOCATED;
    
    // Alright free() next, let's do this
    // We begin checking if the pointer is NULL
    if(ptr == NULL)
    {
        abort();
    }
    
    // Next we check whether or not the pointer is 16 byte aligned
    // if it is not then we abort
    if(!multipleOf16((size_t)ptr))
    {
        abort();
    }
    
    // Then we check if the size of the block is a multiple of 16
    // if it is not then we abort
    if(!multipleOf16(blockLength))
    {
        abort();
    }
    
    // Then we check if the blockSize is at least 32
    if(blockLength < 32)
    {
        abort();
    }
    
    // This block is currently not allocated then why are we freeing it?
    if(allocatedByte == 0)
    {
        abort();
    }
    
    // blockPtr start is not in valid heap range
    if((size_t)blockPtr < (size_t)memStart || (size_t)blockPtr >= (size_t)memEnd)
    {
        abort();
    }
    // blockPtr end is not in valid heap range
    else if((size_t)blockPtrEnd < (size_t)memStart || (size_t)blockPtrEnd > (size_t)memEnd)
    {
        abort();
    }
    
    // If the nextBlock's address is out of the current heap bounds then we will abort
    if((size_t)nextBlockPtr < (size_t)memStart || (size_t)nextBlockPtr > (size_t)memEnd)
    {
        abort();
    }
        
    // TODO There is one more case to check but I'm just not sure about it yet so we will leave it here
    
    
    
    
    
    
    // If we are here the nthat means everything is good to go we can start
    // freeing the memories
    
    // This means that the block we are currently freeing is the wilderness
    // and we will only coalesce with the previous block if it is free
    if(nextBlockLength == 0)
    {
        // If the previous block is allocated we don't coalesce
        // just set the allocated bit to 0 for this wilderness block and add it back in the free_list
        // it will also inherit the pal
        if(prevAllocateBit)
        {
            // The newHeader has everything the same except for allocate bit to be 0
            size_t newHeader = blockLength | prevAllocateBit;            
            blockPtr->header = newHeader;
            
            // Also need to set the footer as well
            *(blockPtrFooter) = newHeader;
            
            // Now we have to add this lone wilderness back into list[7] in the front
            sf_block * dummyNode = &sf_free_list_heads[7];
            sf_block * secondBlock = dummyNode->body.links.next;
            
            dummyNode->body.links.next = blockPtr;
            secondBlock->body.links.prev = blockPtr;
            
            // Set the links for the freed block too
            blockPtr->body.links.next = secondBlock;
            blockPtr->body.links.prev = dummyNode;
            
            // Reset epilogue's pal to be 0 again
            setNewEpilogue();
        }
        else
        {
            // The previous block is not allocated so we have to coalesce with the previous block
            // Get the previous block first
            sf_footer * previousFooter = (sf_footer *)((char *)blockPtr - 8);
            size_t previousLength = getSizeFromFooter(*previousFooter);
            
            sf_block * previousBlockPtr = (sf_block *)((char *)blockPtr - previousLength);
            
            // Get the prevPrev_allocate bit from previousBlock. This is what will be inherited when merging with the wilderness
            int prevPrev_allocate = previousBlockPtr->header & PREV_BLOCK_ALLOCATED;
            
            size_t mergedLength = blockLength + previousLength;
            
            size_t newHeader = mergedLength | prevPrev_allocate;
            
            // Set the header of previous block to be updated as well as the footer
            previousBlockPtr->header = newHeader;
            *(blockPtrFooter) = newHeader;
            
            // Before we add it into free_list, we must remove the old wilderness
            sf_block * dummyNode = &sf_free_list_heads[7];
            
            dummyNode->body.links.next = previousBlockPtr;
            dummyNode->body.links.prev = previousBlockPtr;
            previousBlockPtr->body.links.next = dummyNode; // Make sure only one wilderness exist at all time
            previousBlockPtr->body.links.prev = dummyNode;
            
            // Reset the epilogue again to update the pal status of wilderness
            setNewEpilogue();
        }
    }
    else
    {
        // Only this block is free. No coalescing
        if(prevAllocateBit && nextBlockAllocatedBit)
        {
            int insertIndex = computeMemoryIndex(blockLength);
            
            // Inherit the prev_allocate with allocate_bit set to 0
            size_t newHeader = blockLength | prevAllocateBit;
            
            // Set the new header and footer 
            blockPtr->header = newHeader;
            *(blockPtrFooter) = newHeader;
            
            // Then we must insert it into the appropriate list
            sf_block * dummyNode = &sf_free_list_heads[insertIndex];
            sf_block * secondBlock = dummyNode->body.links.next;
            
            dummyNode->body.links.next = blockPtr;
            secondBlock->body.links.prev = blockPtr;
            blockPtr->body.links.prev = dummyNode;
            blockPtr->body.links.next = secondBlock;
            
            // We also need to set the next block's pal status to be 0
            // but we also need to keep the allocated bit it has we don't want to mess that up
            nextBlockPtr->header = nextBlockLength | nextBlockAllocatedBit;
        }
        // Previous block is also free. Coalesce with previous block only
        else if(!prevAllocateBit && nextBlockAllocatedBit)
        {
            // Get the previous footer first
            sf_footer * previousFooter = (sf_footer *)((char *)blockPtr - 8);
            size_t previousLength = getSizeFromFooter(*previousFooter);
            
            sf_block * previousBlockPtr = (sf_block *)((char *)blockPtr - previousLength);
            
            // This is the prevPrev_allocate status which we are inheriting
            int prevPrev_allocate = previousBlockPtr->header & PREV_BLOCK_ALLOCATED;
            
            size_t mergedLength = previousLength + blockLength;
            
            size_t newHeader = mergedLength | prevPrev_allocate;
            
            // Update the previous block's header, and the footer as well
            previousBlockPtr->header = newHeader;
            *(blockPtrFooter) = newHeader;
            
            // Now we have to cut previous free block out of the original free_list it is in
            sf_block * prevNodeForPrevious = previousBlockPtr->body.links.prev;
            sf_block * nextNodeForPrevious = previousBlockPtr->body.links.next;
            
            prevNodeForPrevious->body.links.next = nextNodeForPrevious;
            nextNodeForPrevious->body.links.prev = prevNodeForPrevious;
            
            // Compute the index to where we are inserting the merged block in
            int insertIndex = computeMemoryIndex(mergedLength);
            
            // Finally we insert the merged node into the corresponding free_list
            sf_block * dummyNode = &sf_free_list_heads[insertIndex];
            sf_block * secondBlock = dummyNode->body.links.next;
            
            dummyNode->body.links.next = previousBlockPtr;
            secondBlock->body.links.prev = previousBlockPtr;
            previousBlockPtr->body.links.prev = dummyNode;
            previousBlockPtr->body.links.next = secondBlock;
            
            // Don't forget to also set nextBlock's pal to be 0
            // The allocated bit is definitely 1 because we aren't merging it with it
            nextBlockPtr->header = nextBlockLength | THIS_BLOCK_ALLOCATED;
        }
        // Next block is also free. Coalesce with next block only
        else if(prevAllocateBit && !nextBlockAllocatedBit)
        {
            // Now this case we must be careful because we might be merging with the wilderness if
            // the current block is right next to it
            // Get the nextNextBlock first
            sf_block * nextNextBlock = (sf_block *)((char *)nextBlockPtr + nextBlockLength);
            size_t nextNextLength = getSizeFromHeader(nextNextBlock->header); // If it is the epilogue its fine we are only accessing the header
            
            int nextNextAllocateBit = nextNextBlock->header & THIS_BLOCK_ALLOCATED;
            
            size_t mergedLength = blockLength + nextBlockLength;
            
            size_t newHeader = mergedLength | prevAllocateBit;
            
            // Update this block's footer and next block's footer to have the new header
            // Doesn't matter if it is for wilderness or non-wilderness this procedure is the same
            blockPtr->header = newHeader;
            *(nextBlockPtrFooter) = newHeader;
            
            // What is different is where we insert the free_block
            // We are coalescing with wilderness
            if(nextNextLength == 0)
            {
                // So we merge together all we have to do is insert it into the free_list[7]
                sf_block * dummyNode = &sf_free_list_heads[7];
                
                dummyNode->body.links.next = blockPtr;
                dummyNode->body.links.prev = blockPtr;
                blockPtr->body.links.prev = dummyNode;
                blockPtr->body.links.next = dummyNode;
                
                // Don't need to update epilogue because pal is already 0                 
            }
            // We are not coalescing with wilderness
            else
            {
                // We must remove the next block from the original free_list it is in right now
                sf_block * prevNodeForNext = nextBlockPtr->body.links.prev;
                sf_block * nextNodeForNext = nextBlockPtr->body.links.next;
                
                prevNodeForNext->body.links.next = nextNodeForNext;
                nextNodeForNext->body.links.prev = prevNodeForNext;
                
                int insertIndex = computeMemoryIndex(mergedLength);
                
                // Finally we insert the merged node into the corresponding free_list
                sf_block * dummyNode = &sf_free_list_heads[insertIndex];
                sf_block * secondBlock = dummyNode->body.links.next;
                                
                dummyNode->body.links.next = blockPtr;
                secondBlock->body.links.prev = blockPtr;
                blockPtr->body.links.prev = dummyNode;
                blockPtr->body.links.next = secondBlock;
                
                // Don't forget to set the pal of nextNext blockto 0
                nextNextBlock->header = nextNextLength | nextNextAllocateBit;
            }
        }
        // Both adjacent block are free. Coalesce with previous and next block
        else
        {
            // Finally we get to the last case we have to determine whether or not we are
            // coalescing with the wilderness so nextNextBlock is needed
            sf_block * nextNextBlock = (sf_block *)((char *)nextBlockPtr + nextBlockLength);
            size_t nextNextLength = getSizeFromHeader(nextNextBlock->header); // If it is the epilogue its fine we are only accessing the header
            
            int nextNextAllocateBit = nextNextBlock->header & THIS_BLOCK_ALLOCATED;
            
            // We also need the previous block
            sf_footer * previousFooter = (sf_footer *)((char *)blockPtr - 8);
            size_t previousLength = getSizeFromFooter(*previousFooter);
            
            sf_block * previousBlockPtr = (sf_block *)((char *)blockPtr - previousLength);
            
            // Get the prevPrev_allocate which the merged block will be inheriting
            int prevPrev_allocate = previousBlockPtr->header & PREV_BLOCK_ALLOCATED;
            
            size_t mergedLength = blockLength + nextBlockLength + previousLength;
            
            size_t newHeader = mergedLength | prevPrev_allocate;
            
            // Then we update previous block's header and footer too
            previousBlockPtr->header = newHeader;
            *(nextBlockPtrFooter) = newHeader;
            
            // Now based on whether nextNextLength is epilogue or not
            // it will determine how we cut ties
            // This means that ultimately the three blocks we are merging will become wilderness
            if(nextNextLength == 0)
            {
                // We only have to cut ties for previousBlock
                sf_block * prevNodeForPrevious = previousBlockPtr->body.links.prev;
                sf_block * nextNodeForPrevious = previousBlockPtr->body.links.next;
                
                prevNodeForPrevious->body.links.next = nextNodeForPrevious;
                nextNodeForPrevious->body.links.prev = prevNodeForPrevious;
                
                // Then we will insert it into the wilderness
                sf_block * dummyNode = &sf_free_list_heads[7];
                
                dummyNode->body.links.next = previousBlockPtr;
                dummyNode->body.links.prev = previousBlockPtr;
                previousBlockPtr->body.links.prev = dummyNode;
                previousBlockPtr->body.links.next = dummyNode;
                
                // Epilogue's pal is already 0 so there is no need to set it again
            }
            else
            {
                // Now if we are here then we have to cut ties for previousBlock as well as nextBlock
                sf_block * prevNodeForPrevious = previousBlockPtr->body.links.prev;
                sf_block * nextNodeForPrevious = previousBlockPtr->body.links.next;
                
                prevNodeForPrevious->body.links.next = nextNodeForPrevious;
                nextNodeForPrevious->body.links.prev = prevNodeForPrevious;
                
                sf_block * prevNodeForNext = nextBlockPtr->body.links.prev;
                sf_block * nextNodeForNext = nextBlockPtr->body.links.next;
                
                prevNodeForNext->body.links.next = nextNodeForNext;
                nextNodeForNext->body.links.prev = prevNodeForNext;
                
                int insertIndex = computeMemoryIndex(mergedLength);
                
                // Now we can finally insert the merged node into the corresponding free_list
                sf_block * dummyNode = &sf_free_list_heads[insertIndex];
                sf_block * secondBlock = dummyNode->body.links.next;
                
                dummyNode->body.links.next = previousBlockPtr;
                secondBlock->body.links.prev = previousBlockPtr;
                previousBlockPtr->body.links.prev = dummyNode;
                previousBlockPtr->body.links.next = secondBlock;
                
                // Then we can't forget to set nextNext block's pal to 0
                nextNextBlock->header = nextNextLength | nextNextAllocateBit;
            }
        }
    }
    

    // // This means that the block we are freeing is the wilderness
    // // We will only be coalescing with the previous block
    // if(nextBlockLength == 0)
    // {
    //     // We only coalesce if prevAllocateBit is 0    
    //     if(prevAllocateBit)
    //     {
    //         // This means that the previous block is allocated hence we don't coalesce
    //         // just set the allocated bit to 0 and add it to list[7]. But also inheriting the pal
    //         size_t newHeader = blockLength | prevAllocateBit;
    //         blockPtr->header = newHeader;
            
    //         // We also need to set the footer
    //         sf_footer * newFooter = (sf_footer *)((char *)blockPtr + blockLength - 8);
    //         *(newFooter) = newHeader;
            
    //         // Add in the new links
    //         // Cannot get the struct directly need the address since the values are copied
    //         sf_block * dummyNode = &sf_free_list_heads[7];
    //         sf_block * theSecondBlock = dummyNode->body.links.next;

    //         dummyNode->body.links.next = blockPtr;            
    //         theSecondBlock->body.links.prev = blockPtr;
            
    //         blockPtr->body.links.next = theSecondBlock;
    //         blockPtr->body.links.prev = dummyNode;
            
    //         // For consistency sake, we will reset the epilogue's pal to 0 as well by calling setNewEpilogue
    //         setNewEpilogue();
    //     }        
    //     else
    //     {
    //         // This means that the previous block is not allocated hence we have to coalesce
    //         // We have to get the footer of the previous block first which can be done
    //         // by going back another 8 bytes from blockPtr
    //         sf_footer * prevFooter = (sf_footer *)((char *)blockPtr - 8);
            
    //         // Get the footer length
    //         size_t footerLength = getSizeFromFooter(*prevFooter);
            
    //         // Then we can just subtract footerLength from blockPtr to get to the previous block
    //         sf_block * prevBlock = (sf_block *)((char *)blockPtr - footerLength);
            
            
    //         // We get the prev_allocate of the prevBlock because that is going to be what we are inheriting in our merged block
    //         int prevPrev_allocate = prevBlock->header & PREV_BLOCK_ALLOCATED;
            
    //         // Add together the combined length and inherit the prevPrev_allocate status
    //         size_t mergedLength = blockLength + footerLength;
            
    //         // Set the header now we also have to set the footer
    //         prevBlock->header = mergedLength | prevPrev_allocate;
            
            
            
    //         sf_footer * mergedFooter = (sf_footer *)((char *)prevBlock + mergedLength - 8);
    //         *(mergedFooter) = prevBlock->header;
            
            
            
    //         // Finally we have to add it back into the wilderness
    //         sf_block * dummyNode = &sf_free_list_heads[7];
    //         sf_block * theSecondBlock = dummyNode->body.links.next;

    //         dummyNode->body.links.next = prevBlock;            
    //         theSecondBlock->body.links.prev = prevBlock;
            
    //         prevBlock->body.links.next = theSecondBlock;
    //         prevBlock->body.links.prev = dummyNode;
            
    //         // Again for consistency sake we will set new epilogue because the wilderness is freed
    //         setNewEpilogue();
    //     }
    // }      
    // else
    // {
    //     // This means that the block we are freeing is not the wilderness
    //     // We will be breaking up into 4 different cases
    //     // Make sure we don't merge with the prologue and the epilogue
    //     int nextAllocatedBit = *(nextBlock) & THIS_BLOCK_ALLOCATED;
        
    //     // prev and next are both allocated so no coalescing
    //     if(prevAllocateBit && nextAllocatedBit)
    //     {
    //         // Find where are we inserting it
    //         int insertIndex = computeMemoryIndex(blockLength);
            
    //         // Make the new header with inherited prev_allocate
    //         size_t newHeader = blockLength | prevAllocateBit;
            
    //         // Set it as the new header
    //         blockPtr->header = newHeader;
            
    //         // We also have to set the footer as well
    //         sf_footer * blockFooter = (sf_footer *)((char *)blockPtr + blockLength - 8);
    //         *(blockFooter) = blockPtr->header;
            
            
            
    //         // Then we finally insert it in the approriate list
    //         sf_block * dummyNode = &sf_free_list_heads[insertIndex];
    //         sf_block * secondBlock = dummyNode->body.links.next;
            
    //         dummyNode->body.links.next = blockPtr;
    //         secondBlock->body.links.prev = blockPtr;
            
    //         blockPtr->body.links.next = secondBlock;
    //         blockPtr->body.links.prev = dummyNode;
            
    //         // Then we have to set next block's pal to be 0 and it to be allocated because next block is allocate
    //         *(nextBlock) = nextBlockLength | THIS_BLOCK_ALLOCATED;
    //     }
    //     // prev is allocated but next block is not allocated, so only coalesce with next block
    //     else if(prevAllocateBit && !nextAllocatedBit)
    //     {
    //         sf_block * nextBlockPtr = blockPtr + blockLength;
            
    //         size_t mergedLength = blockLength + nextBlockLength;
            
    //         // Set the new header. Inherit the prev allocated status
    //         blockPtr->header = mergedLength | prevAllocateBit;
            
    //         // Now we need to also set the footer as well
    //         sf_footer * mergedFooter = (sf_footer *)((char *)blockPtr + mergedLength - 8);
    //         *(mergedFooter) = blockPtr->header;
            
    //         // Determine the index of where to insert this list
    //         int insertIndex = computeMemoryIndex(mergedLength);
            
    //         // We first remove the nextBlockPtr from its list first
    //         sf_block * prevNode4Next = nextBlockPtr->body.links.prev;
    //         sf_block * nextNode4Next = nextBlockPtr->body.links.next;
            
    //         // We have to cut it out
    //         prevNode4Next->body.links.next = nextNode4Next;
    //         nextNode4Next->body.links.prev = prevNode4Next;
            
    //         // Then we can insert the merged block into the corresponding list
    //         sf_block * dummyNode = &sf_free_list_heads[insertIndex];
    //         sf_block * secondBlock = dummyNode->body.links.next;
            
    //         dummyNode->body.links.next = blockPtr;
    //         secondBlock->body.links.prev = blockPtr;
            
    //         blockPtr->body.links.prev = dummyNode;
    //         blockPtr->body.links.next = secondBlock;
            
    //         // Now we have to set next next block's pal to be 0
    //         sf_header * nextNextHeader = (sf_header *)((char *)nextBlockPtr + nextBlockLength);
    //         size_t nextNextLength = getSizeFromHeader(*nextNextHeader);
    //         int nextNextAllocate = *(nextNextHeader) & THIS_BLOCK_ALLOCATED;
            
    //         *(nextNextHeader) = nextNextLength | nextNextAllocate; 
    //     }
    //     // prev is not allocated but next block is allocated, so only coalesce with previous block
    //     else if(!prevAllocateBit && nextAllocatedBit)
    //     {
    //         // Get the previous free block's footer by going back 8 bytes
    //         sf_footer * prevFooter = (sf_footer *)((char *)blockPtr - 8);
            
    //         // Then we get the length of the previous block
    //         size_t previousLength = getSizeFromFooter(*prevFooter);
            
    //         // Get the previous block
    //         sf_block * prevBlock = (sf_block *)((char *)blockPtr - previousLength);
            
            
            
    //         // This is the prevPrev_allocate status that we are inheriting
    //         int prevPrev_allocate = prevBlock->header & PREV_BLOCK_ALLOCATED;
            
            
            
    //         size_t mergedLength = blockLength + previousLength;
            
    //         // Update the new header
    //         prevBlock->header = mergedLength | prevPrev_allocate;
            
    //         // Then we also have to update the footer as well
    //         sf_footer * mergedFooter = (sf_footer *)((char *)prevBlock + mergedLength - 8);
    //         *(mergedFooter) = prevBlock->header;
            
            
            
    //         // We have to cut prevBlock's reference out from the free_list
    //         sf_block * prevNode4Prev = prevBlock->body.links.prev;
    //         sf_block * nextNode4Prev = prevBlock->body.links.next;
            
    //         prevNode4Prev->body.links.next = nextNode4Prev;
    //         nextNode4Prev->body.links.prev = prevNode4Prev;
            
    //         // Determine which index we are inserting it in
    //         int insertIndex = computeMemoryIndex(mergedLength);
            
    //         // Then we finally insert the merged node into the corresponding free_list
    //         sf_block * dummyNode = &sf_free_list_heads[insertIndex];
    //         sf_block * secondBlock = dummyNode->body.links.next;
            
    //         dummyNode->body.links.next = prevBlock;
    //         secondBlock->body.links.prev = prevBlock;      
            
    //         // We have to set nextBlock's pal to be 0
    //         *(nextBlock) = nextBlockLength | THIS_BLOCK_ALLOCATED;
    //     }
    //     // Both are not allocated so coalesce with both
    //     else
    //     {
    //         sf_footer * prevFooter = (sf_footer *)((char *)blockPtr - 8);
            
    //         size_t previousLength = getSizeFromFooter(*prevFooter);
            
    //         // We get the previous block and the next block
    //         sf_block * previousBlockPtr = (sf_block *)((char *)blockPtr - previousLength);
    //         sf_block * nextBlockPtr = (sf_block *)((char *)blockPtr + nextBlockLength);
            
    //         // We will be ultimately inheriting the prev_allocate status from previousBlockPtr
    //         int prevPrev_allocate = previousBlockPtr->header & PREV_BLOCK_ALLOCATED;
            
    //         // Calculte the merged length
    //         size_t mergedLength = blockLength + previousLength + nextBlockLength;
            
    //         // Update the new header
    //         previousBlockPtr->header = mergedLength | prevPrev_allocate;
            
    //         // We also need to update the new footer as well
    //         sf_footer * mergedFooter = (sf_footer *)((char *)previousBlockPtr + mergedLength - 8);
    //         *(mergedFooter) = previousBlockPtr->header;
            
    //         // Then finally we take previousBlockPtr and nextBlockPtr both out of the free_list
    //         sf_block * prevNode4Prev = previousBlockPtr->body.links.prev;
    //         sf_block * nextNode4Prev = previousBlockPtr->body.links.next;
            
    //         sf_block * prevNode4Next = nextBlockPtr->body.links.prev;
    //         sf_block * nextNode4Next = nextBlockPtr->body.links.next;
            
    //         prevNode4Prev->body.links.next = nextNode4Prev;
    //         nextNode4Prev->body.links.prev = prevNode4Prev;
            
    //         prevNode4Next->body.links.next = nextNode4Next;
    //         nextNode4Next->body.links.prev = prevNode4Next;
            
    //         int insertIndex = computeMemoryIndex(mergedLength);
            
    //         // Finally we will insert the merged block back into the corresponding free_list
    //         sf_block * dummyNode = &sf_free_list_heads[insertIndex];
    //         sf_block * secondBlock = dummyNode->body.links.next;
            
    //         dummyNode->body.links.next = previousBlockPtr;
    //         secondBlock->body.links.prev = previousBlockPtr;  
            
    //         // We have to set nextNext block's pal to be 0
    //         sf_header * nextNextHeader = (sf_header *)((char *)nextBlockPtr + nextBlockLength);
    //         size_t nextNextLength = getSizeFromHeader(*nextNextHeader);
    //         int nextNextAllocate = *(nextNextHeader) & THIS_BLOCK_ALLOCATED;
            
    //         *(nextNextHeader) = nextNextLength | nextNextAllocate; 
    //     }
    // }  
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
