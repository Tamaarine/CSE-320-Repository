#include <string.h>

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

/**
 * This function will set a new epilogue after the heap being extended
 */
void setNewEpilogue();

/**
 * Given s size_t variable that can represent the pointer address or
 * the size of the data type. Return 1 if it is multiple of 16. Return 0 otherwise
 */
int multipleOf16(size_t num);

/**
 * Returns the size from the header after shifting 4 bits right and 4 bits back
 */
size_t getSizeFromHeader(sf_header header);

size_t getSizeFromFooter(sf_footer footer);