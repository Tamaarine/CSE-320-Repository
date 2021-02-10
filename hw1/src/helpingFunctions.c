#include "helpingFunctions.h"
#include <stdio.h>

int free_node_counter = 256;

/**
 * This function will take two Strings and compare them and see if they are equal or not
 * 
 * Return 0 if not equal
 * Return 1 if they are equal
 */
int compareString(char * str1, char * str2)
{
    int length1 = findLength(str1);
    int length2 = findLength(str2);
    
    // Length is not the same then it cannot be equal
    if(length1 != length2)
    {
        return 0;
    }
    // The lengths are equal hence we will further check the individual chars
    else
    {
        // Since str1 and str2 just decays to pointer of the first element
        // we can just assign the char pointer to the variable name
        char * pointer1 = str1;
        char * pointer2 = str2;
        
        // Using a for loop to do the comparsion of char by char
        for(int i=0;i<length1;i++)
        {
            // If any of the chars of both String is not equal return false
            if(*pointer1 != *pointer2)
            {
                return 0;
            }
            
            // If we are here then we increment the char pointer
            pointer1 ++;
            pointer2 ++;
        }
        
        // Now if we are here then that means that we are done comparing the two Strings
        // and they are equal
        return 1;
    }
}

/**
 * This function will find the length of the given String and return it
 */
int findLength(char * str)
{
    int lengthCounter = 0;

    // We set currentChar to point to the first char
    char * currentChar = str;

    // So while the currentChar is not yet a null terminator we will keep incrementing
    while(*(currentChar) != '\0')
    {
        // Increment the counter
        lengthCounter ++;
        
        // Increment the pointer
        currentChar ++;
    }
    
    // If we are here then that means the length counting is done hence we can return
    return lengthCounter;
}

/**
 * This function will convert a given number String to a integer and return it
 * 
 * Assume the given String will definitely be a valid numerical String and just convert it
 */
int stringToInteger(char * str)
{
    // Find the length of the given String first
    int length = findLength(str);
    
    // sum will keep track of the current sum
    int sum = 0;
    
    // multiplier will be used to multiply each digit
    int multiplier = 1;
    
    // Then use a for loop to calculate the numerical value of the numerical String
    for(int i=length-1;i>=0;i--)
    {
        // Get the current char digit first
        char currentCharDigit = *(str + i);
        
        // Subtract the ascii value by 48 to get the interger value
        int currentDigit = currentCharDigit - 48;
        
        // Then we multiply multiplier with currentDigit
        currentDigit = multiplier * currentDigit;
        
        // And add it with sum
        sum += currentDigit;
        
        // Increment the multiplier by multiplying it by 10
        multiplier = multiplier * 10;
    }
    
    // Then we can just return the sum here as the converted number from String
    return sum;
}

/**
 * This function will validate a String to determine whether or not it is a
 * valida number String to be put through stringToInteger
 * 
 * Returns 0 if the String is not a numerical String
 * Returns 1 if the String is a numerical String
 */
int validateNumberString(char * str)
{
    // Find the length of the String first
    int length = findLength(str);
    
    char * charPtr = str;
    
    for(int i=0;i<length;i++)
    {
        // // Print each character's ascii value
        // printf("%d ", *(charPtr));
        
        // If any one of the character is not a numerical char then
        // we return 0
        if(*(charPtr) < '0' || *(charPtr) > '9')
        {
            return 0;
        }
        
        // If the character is indeed a numerical character we have to increment
        // the char pointer
        charPtr ++;
        
    }
    
    // However, if we are here then all of the chars in the String are numerical
    // hence we can return 1
    return 1;
}

/**
 * Given an integer turn it into a two's complement number 
 */
int integerTo2Complement(int num)
{
    // The procedure is simple, negate all the bits and just add 1
    // Negating the number
    int output = ~num;
    
    // Add 1 to the number to get to two's complement
    output ++;
    
    // And donne return the output
    return output;
}

/**
 * This function will be used for mapping the given node specifications to an index
 * in the hash table. That's it nothin else we will handle the linear probing
 * in the returned function after
 */
int hashFunction(int left, int right)
{
    // The hash function is simple we take left and we take right
    // add 48 to both of them and multiply by 997 add them together as a sum
    int leftValue = (left + 48) * 997;
    int rightValue = (right + 48) * 997;
    
    // Then we add them together as a sum
    int sum = leftValue + rightValue;
    
    // Finally the output of the function is just sum mod by the hashTable size
    return sum % BDD_HASH_SIZE;
}

void printRasterArray(unsigned char * raster, size_t size)
{
    for(int i=0;i<size;i++)
    {
        printf("%c ", *(raster + i));
    }
}


void initialize_bdd_hash_map()
{
    for(int i=0;i<BDD_HASH_SIZE;i++)
    {
        *(bdd_hash_map + i) = NULL;
    }
}

/**
 * Returns the log of 2 of a number
 */
int log_of_2(int num)
{
    int counter = 0;
    
    while(num > 1)
    {
        num = num / 2;
        counter ++;
    }
    
    return counter;
}