#include <string.h>

#define LENGTH_MASK 0xfffffff0

/**
 * Given a size find the index which tells us which free_list to start to search from
 */
int computeMemoryIndex(size_t size);

/**
 * Raise 2 to the power of a given exponent
 */
int pow2(int exp);

/**
 * Given a size, find the nearest multiple of 16 
 */
size_t computeMemorySize(size_t size);

/**
 * Given a free list head with headers and a wantedSize in bytes
 * determine whether or not the allocation of a wantedSize byte block in suitableListHead
 * will leave a splinter or not. Return 1 if it will, 0 if it won't
 */
int leaveSplinter(sf_block * suitableListHead, size_t wantedSize);