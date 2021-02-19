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
int hashFunction(int left, int right, int level);

/**
 * Returns the log of 2 of a number
 */
int log_of_2(int num);

/**
 * Returns the power of 2 to the n
 */
int pow2(int num);

/**
 * This function will print the corresponding 4 bytes using the given serial
 * in little-endian format
 */
void printSerialNumberChild(int serial, FILE *out);

/**
 * This function will take in 4 bytes in total representing the little endian order
 * serial number. It will take those 4 bytes and transform them into one integer that represents the serial number
 * by doing polynomial expansion for base 256
 */
int fourByteIntoInteger(int firstByte, int secondByte, int thirdByte, int fourthByte);

/**
 * Returns num divide by 2 timesToDivide times 
 */
int divideBy2(int num, int timesToDivide);

void printRasterArray(unsigned char * raster, size_t size);

void initialize_bdd_hash_map();
void initialize_bdd_index_map();
void initialize_raster();

