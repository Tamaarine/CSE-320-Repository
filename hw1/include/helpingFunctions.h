#include "const.h"

extern int free_node_counter;

/**
 * This function will take two Strings and compare them and see if they are equal or not
 */
int compareString(char * str1, char * str2);

/**
 * This function will find the length of the given String and return it
 */
int findLength(char * str);

/**
 * THis function will convert a given number String to a integer and return it
 */
int stringToInteger(char * str);

/**
 * This function will validate a String to determine whether or not it is a
 * valida number String to be put through stringToInteger
 */
int validateNumberString(char * str);

/**
 * Given an integer turn it into a two's complement number 
 */
int integerTo2Complement(int num);

/**
 * This function will be used for mapping the given node specifications to an index
 * in the hash table. That's it nothin else we will handle the linear probing
 * in the returned function after
 */
int hashFunction(int left, int right);

void printRasterArray(unsigned char * raster, size_t size);

void initialize_bdd_hash_map();

