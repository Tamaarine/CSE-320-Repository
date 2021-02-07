#include "const.h"

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

void printRasterArray(unsigned char * raster, size_t size);