#include <string.h>
#include "sfmm.h"
#include "helpingFunction.h"

int computeMemoryIndex(size_t size)
{
    for(int i=0;i<7;i++)
    {
        // Got to the last list then we will look at the index 6 free_list
        if(i == 6)
            return i;
        if(i == 0 && size <= 32)
            return i;
        // Else if the index is < 6 we will do the boundary checks to determine which
        // free_list we start searching from
        else if(size > 32 * pow2(i - 1) && size <= 32 * pow2(i))
            return i;
    }
    // We will never get here just return the 6th index if we ever got here
    return 6;
}

int pow2(int exp)
{
    int sum = 1;
    
    for(int i=0;i<exp;i++)
    {
        sum = sum * 2;
    }
    
    return sum;
}

size_t computeMemorySize(size_t size)
{
    // If the size is already a multiple of 16 we just return the same size
    if(size % 16 == 0)
    {
        return size;
    }
    else
    {
        // We compute multiple by dividing the size by 16
        int multiple = size / 16 + 1;
        return multiple * 16;
    }
}

int leaveSplinter(sf_block * suitableListHead, size_t wantedSize)
{
    // Get the length of suitableListHead
    size_t blockLength = suitableListHead->header >> 4; // Shifts to the right 4
    blockLength = blockLength << 4; // Shifts to the left 4 to reset the 4 bits
    
    // Then we get the diff
    size_t diff = blockLength - wantedSize;
    
    if(diff < 32)
    {
        // If the left over byte is less than 32, 0 - 31 we don't want to create
        // splitner hence we will return 1
        return 1;
    }
    else
    {
        // However if the left over is 32 byte or more then it is fine, it won't
        // be creating splinter if we split
        return 0;
    }
}

void setNewEpilogue()
{
    // Make the header reference to the new heap and set the header to 1
    sf_header * newEpilogue = (sf_header *)((char *)sf_mem_end() - 8);
    *(newEpilogue) = 0 | THIS_BLOCK_ALLOCATED;
}