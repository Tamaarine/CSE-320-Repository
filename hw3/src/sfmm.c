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
        
    // If the previousAllocateBit is equal to 0 then we must check
    // whether or not that allocation bit in the previous block matches as well, if it doesn't
    // match then we will call abort
    if(prevAllocateBit == 0)
    {
        sf_footer * previousFooter = (sf_footer *)((char *)blockPtr - 8);
        
        size_t previousLength = getSizeFromFooter(*previousFooter);
        
        sf_block * previousBlock = (sf_block *)((char *)blockPtr - previousLength);
        
        int allocateBit = previousBlock->header & THIS_BLOCK_ALLOCATED;
        int allocateBitFromFooter = previousBlock->header & THIS_BLOCK_ALLOCATED;
        
        // The previous block's allocate bit is not 0 then we will abort
        if(allocateBit != 0 || allocateBitFromFooter != 0)
        {
            abort();
        }
    }    
    
    
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
    return;
}

void *sf_realloc(void *ptr, size_t size)
{
    // Alright realloc next let's do this
    // First we must verify the pointer that is passed is a valid pointer to free
    // which just follows the same critera in sf_free
    if(ptr == NULL)
    {
        sf_errno = EINVAL;
        return NULL;
    }
    
    if(!multipleOf16((size_t)ptr))
    {
        sf_errno = EINVAL;
        return NULL;
    }
    
    sf_block * blockPtr = (sf_block *)((char *)ptr - 8);
    
    size_t blockLength = getSizeFromHeader(blockPtr->header);
    
    int allocatedByte = blockPtr->header & THIS_BLOCK_ALLOCATED;
    int prevAllocateBit = blockPtr->header & PREV_BLOCK_ALLOCATED;
    
    char * blockPtrEnd = (char *)blockPtr + blockLength;
    
    
    // Get the valid memory regions
    char * memStart = (char *)sf_mem_start() + 40;
    char * memEnd = (char *)sf_mem_end() - 8;
    
    
    // We can get information about the nextBlock since we are guaranteed to find it
    sf_block * nextBlockPtr = (sf_block *)((char *)blockPtr + blockLength);
    
    // The block is not multiple of 16
    if(!multipleOf16(blockLength))
    {
        sf_errno = EINVAL;
        return NULL;
    }
    
    // The block is not at least 32
    if(blockLength < 32)
    {
        sf_errno = EINVAL;
        return NULL;
    }
    
    // This block is currently not allocated then why free?
    if(allocatedByte == 0)
    {
        sf_errno = EINVAL;
        return NULL;
    }
    
    // blockPtr is not in valid heap range
    if((size_t)blockPtr < (size_t)memStart || (size_t)blockPtr >= (size_t)memEnd)
    {
        sf_errno = EINVAL;
        return NULL;
    }
    // blockPtrEnd is not in valid heap range
    else if((size_t)blockPtrEnd < (size_t)memStart || (size_t)blockPtrEnd > (size_t)memEnd)
    {
        sf_errno = EINVAL;
        return NULL;
    }
    
    // If nextBlock's address is out of the current heap bound then we will abort. It's fine it is epilogue
    if((size_t)nextBlockPtr < (size_t)memStart || (size_t)nextBlockPtr > (size_t)memEnd)
    {
        sf_errno = EINVAL;
        return NULL;
    }
    
    // Then we have to check the header of the previous block if it is free
    // matches the pal of this block we are freeing. We will only check if prevAllocateBit == 0
    if(prevAllocateBit == 0)
    {
        sf_footer * previousFooter = (sf_footer *)((char *)blockPtr - 8);
        
        size_t previousLength = getSizeFromFooter(*previousFooter);
        
        sf_block * previousBlock = (sf_block *)((char *)blockPtr - previousLength);
        
        int allocateBit = previousBlock->header & THIS_BLOCK_ALLOCATED;
        int allocateBitFromFooter = previousBlock->header & THIS_BLOCK_ALLOCATED;
        
        // The previous block's allocate bit is not 0 then we will abort
        // because it doesn't match with the pal of this block
        if(allocateBit != 0 || allocateBitFromFooter != 0)
        {
            sf_errno = EINVAL;
            return NULL;
        }
    }
    
    // Finally we can begin with our algorithm because everything is good
    // this pointer is valid
    
    // If the size is 0 but the pointer is valid, we free the memory and return NULL
    if(size == 0)
    {
        sf_free(ptr);
        return NULL;
    }
    
    size_t adjustedSize = 0;
    
    // If the user want to resize the payload to anything between 0 to 24 btyes
    // we have to automatically round up to 32 to account for the header and the footer/prev/next
    if(size <= 24)
    {
        adjustedSize = 32;
    }
    else
    {
        // Else then we will compute how much the size actually is
        adjustedSize = computeMemorySize(size + 8);
    }
    
    // If the adjusted size is greater than blockLength
    // we will just allocate a new block of size adjustedSize
    if(adjustedSize > blockLength)
    {
        // First we call sf_malloc to get a new block, we don't pass it adjustedSize
        // just pass in the size. It will figure out the adjustedSize itself
        char * returnedPtr = (char *)sf_malloc(size);
        
        // We must check if sf_malloc returns NULL or not
        if(returnedPtr == NULL)
        {
            // No need to set sf_errno because sf_malloc sets it
            return NULL;
        }   
        
        // Next we will have to memcpy the data from the given block to the bigger block
        memcpy(returnedPtr, ptr, blockLength - 8); // Only need to copy blockLength - 8 bytes
        
        // Then we will call sf_free on the original block
        sf_free(ptr);
        
        // Finally return the allocated block to the client
        return returnedPtr;
    }
    else
    {
        // If we are here then that means we are going from bigger block to a smaller block
        // 64 to 48 or 64 to 32, and there are two cases we must consider
        // One case where we will get splinters, the other case where we dont' get splinters
        // First we get the size differences after splitting
        size_t diff = blockLength - adjustedSize;
        
        // Meaning that it will be a splinter 
        if(diff < 32)
        {
            // We just return the same block back to the user without any freeing
            return ptr;
        }
        // No splinter hence we split
        else
        {
            // Adding adjustedSize get us to the splitted block which we will have to free
            sf_block * nextBlockPtr = (sf_block *)((char *)blockPtr + adjustedSize);
            
            // We have to update the blockPtr's header because it is splitted
            blockPtr->header = adjustedSize | THIS_BLOCK_ALLOCATED;
            blockPtr->header = blockPtr->header | prevAllocateBit; // Also have to inherit the prev allocate bit so we don't lose it
            
            // Now we also have to update the nextBlockPtr's status to be passed into sf_free
            nextBlockPtr->header = diff | THIS_BLOCK_ALLOCATED;
            nextBlockPtr->header = nextBlockPtr->header | PREV_BLOCK_ALLOCATED; // pal is equal to 1
            
            // Now we can call sf_free on nextBlockPtr
            sf_free((char *)nextBlockPtr + 8);
            
            // Then finally we can return it to the client
            return ptr;
        }
    }
}

void *sf_memalign(size_t size, size_t align) 
{
    // If the requested size is 0 just return NULL
    if(size == 0)
    {
        return NULL;
    }
    else if(size < 32)
    {
        sf_errno = EINVAL;
        return NULL;
    }
    
    // If the align is not a power of 2 then we also return NULL and set sf_errno
    if(!powerOf2(align))
    {
        sf_errno = EINVAL;
        return NULL;
    }
    
    // If we are here the nthat means size and align are valid arguments hence we will do our algorithm
    // We should allocate a block that is at least size + alignment_size + 32 + 8
    // the 8 byte will be handled in sf_malloc so we don't add it
    size_t adjustedSize = size + align + 32;
    
    // Let's malloc that adjustedSize block
    char * returnedPtr = sf_malloc(adjustedSize);
    
    if(returnedPtr == NULL)
    {
        sf_errno = ENOMEM;
        return NULL;
    }
    
    // But if we are outside then we can check if it is a multiple of align
    if((size_t)returnedPtr % align == 0)
    {
        return returnedPtr;
    }
    
    // However if it is not aligned then we have to find that larger address
    // that is aligned. Just a simple for loop using another pointer to do the incrementation
    char * movingPtr = returnedPtr;
    
    sf_block * blockPtr = (sf_block *)(returnedPtr - 8);
    size_t blockLength = getSizeFromHeader(blockPtr->header);
    
    for(int i=0;i<blockLength;i++)
    {
        movingPtr = movingPtr + 1;
        
        // If we finally reached the aligned address we will break from this loop
        if((size_t)movingPtr % align == 0)
        {
            break;
        }   
    }
    
    // If we are out here then movingPtr points to the address that we actually want to be returning
    // We must free the init part of the block we cut off because we don't need it.
    // To figure out the size of that block we just have to subtract movingPtr - returnedPtr
    size_t initLength = (movingPtr - 8) - (char *)blockPtr; // Need to go back 8 bytes, because it only goes up to the header
    
    // Now we have to update the header we have this length so when we call sf_free it will free the correct amount
    int originalPalStatus = blockPtr->header & PREV_BLOCK_ALLOCATED;
    
    blockPtr->header = initLength | originalPalStatus;
    blockPtr->header = blockPtr->header | THIS_BLOCK_ALLOCATED;
    
    // Get the total length of the actual block we have
    size_t movingPtrLength = blockLength - initLength;
    
    // Now before we free we have to check whether the movingPtr is too much in length and see if we are able to split at all
    // and if we can the resulting block must not be a splinter, if it results in splinter we don't split
    // if it doesn't then we will split by doing the same strategy as in sf_realloc
    size_t headerAndSize = size + 8;
    
    size_t actualSize = 0;
    
    actualSize = computeMemorySize(headerAndSize);
    
    // Results in a splinter, just use the entire block, no need to free the block after
    if(movingPtrLength - actualSize < 32)
    {
        // Set the header of the movingPtr
        sf_block * returningPtr = (sf_block *)(movingPtr - 8);
        returningPtr->header = movingPtrLength | THIS_BLOCK_ALLOCATED;
        returningPtr->header = returningPtr->header | PREV_BLOCK_ALLOCATED;
        
        // Call sf_free on the blockPtr
        sf_free(returnedPtr);
        
        return movingPtr;
    }
    // Doesn't result in a splinter so we can split and free the block after as well
    else
    {
        sf_block * nextBlockPtr = (sf_block *)(movingPtr + actualSize - 8);
        nextBlockPtr->header = (movingPtrLength - actualSize) | THIS_BLOCK_ALLOCATED;
        nextBlockPtr->header = nextBlockPtr->header | PREV_BLOCK_ALLOCATED;
        
        // Set the header of the movingPtr
        sf_block * returningPtr = (sf_block *)(movingPtr - 8);
        returningPtr->header = actualSize | THIS_BLOCK_ALLOCATED;
        returningPtr->header = returningPtr->header | PREV_BLOCK_ALLOCATED;
        
        
        // Call sf_free on blockPtr
        sf_free(returnedPtr);
        sf_free((char *)nextBlockPtr + 8);
        
        return movingPtr;
    }
}
