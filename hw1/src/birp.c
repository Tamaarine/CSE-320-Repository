/*
 * BIRP: Binary decision diagram Image RePresentation
 */

#include "image.h"
#include "bdd.h"
#include "const.h"
#include "debug.h"
#include "helpingFunctions.h"

int pgm_to_birp(FILE *in, FILE *out) {
    // Let's do this one next oh god.
    // We have to read the pgm file from stdin using the function that is made for us already
    int width = 0;
    int height = 0;
    int size = 0;
    
    int result = img_read_pgm(in, &width, &height, raster_data, RASTER_SIZE_MAX);
    
    // If the reading of pgm went wrong we return -1 
    if(result == -1)
    {
        return -1;
    }

    // If we are here then that means reading of pgm file was sucess now we can
    // begin turning the raster data into birp
    // The first step is definitely calling bdd_from_raster to get the BDD diagram created
    BDD_NODE * rootNode = bdd_from_raster(width, height, raster_data);
    
    // We havbe to call img_write_birp which in term will call bdd_serialize    
    int result2 = img_write_birp(rootNode, width, height, out);
    
    // If result2 is -1 that means something went wrong when writing hence we return -1
    if(result2 == -1)
    {
        return -1;
    }
    else
    {
        return 0;
    }
}

int birp_to_pgm(FILE *in, FILE *out) {
    // TO BE IMPLEMENTED
    return -1;
}

int birp_to_birp(FILE *in, FILE *out) {
    // TO BE IMPLEMENTED
    return -1;
}

int pgm_to_ascii(FILE *in, FILE *out) {
    // Try this one first to see if I can get a hang of this
    // Call img_read_header to get the information about the pgm first
    int width = 0;
    int height = 0;
    int size = 0;
    
    int result = img_read_pgm(in, &width, &height, raster_data, RASTER_SIZE_MAX);
    // printf("Result from reading pgm %d\n", result);
    
    // If reading the pgm went wrong -> giving us -1 then we have to also return -1 without
    // printing anything from the raster_data
    if(result == -1)
    {
        return -1;
    }
    
    // Then we have to print the raster_data to get it to an ascii art
    // here is the folowing conversion
    // 0 - 63 will be ' ' (space)
    // 64 - 127 will be '.'
    // 128 - 191 will be '*'
    // 192 - 255 will be '@'
    int totalSize = width * height; // The total size of the array
    
    // We will loop through the entire raster array
    // then for every row we haev printed we will print the data to a new line
    for(int i=0;i<totalSize;i++)
    {
        // Getting the currentChar that we are reading
        unsigned char currentChar = *(raster_data + i);
        
        // We also have to print newline if we finish printing one row
        // meaning that i is a multiple of 8 hence we have to print a newline
        if(i != 0 && i % width == 0)
        {
            // printf("\n");
            fputc('\n', out);
        }
        
        // Then we must compare to see which mapping we are doing
        if(currentChar >= 0 && currentChar <= 63)
        {
            // Print a white space
            // printf(" ");
            fputc(' ', out);
        }
        else if(currentChar >= 64 && currentChar <= 127)
        {
            // Print a period
            // printf(".");
            fputc('.', out);
        }
        else if(currentChar >= 128 && currentChar <= 191)
        {
            // Print an asterisk
            // printf("*");
            fputc('*', out);
        }
        else if(currentChar >= 192 && currentChar <= 255)
        {
            // Print @
            // printf("@");
            fputc('@', out);
        }
    }
    
    // 2 new line to end it off
    fputc('\n', out);
    
    // If we got here then we have printed everything hence we can return 0 for sucessful
    return 0;
}

int birp_to_ascii(FILE *in, FILE *out) {
    // TO BE IMPLEMENTED
    return -1;
}

/**
 * @brief Validates command line arguments passed to the program.
 * @details This function will validate all the arguments passed to the
 * program, returning 0 if validation succeeds and -1 if validation fails.
 * Upon successful return, the various options that were specifed will be
 * encoded in the global variable 'global_options', where it will be
 * accessible elsewhere int the program.  For details of the required
 * encoding, see the assignment handout.
 *
 * @param argc The number of arguments passed to the program from the CLI.
 * @param argv The argument strings passed to the program from the CLI.
 * @return 0 if validation succeeds and -1 if validation fails.
 * @modifies global variable "global_options" to contain an encoded representation
 * of the selected program options.
 */
int validargs(int argc, char **argv) {
    
    // Let's just get the simpliest case out of the way which is when no flag is passed through
    // the commandline we will just return with -1 
    // After returning -1 it will go back to main and will exit with failure
    if(argc == 1)
    {
        return -1;
    }
    
    // Then if we are here then that means there is at least 2 or more arguments
    // Then we will handle the case where we detect -h as the first flag we encountered
    // at index 1 of argv. REMEMBER NO ARRAY INDEXING! Must use dereferencing!
    if(compareString(*(argv + 1), "-h"))
    {
        // We also have to set the global_option's MSB to 1
        global_options = 0x80000000;
        
        printf("global_options is 0x%08x\n", global_options);
        
        // Then we return success so it won't be a EXIT_FAILURE
        return 0;
    }
    
    // Variable that is used to help only have one -i and -o and later on to check if optional variable are after positional argument
    int inputSet = 0;
    int outputSet = 0;
    int transformationSet = 0;
    
    // Variable to use for setting the input and output format parameter
    int inputValue = 0x2; // Default is birp which is 2
    int outputValue = 0x2; // Default is birp which is 2
    
    // Also two more variable for storing transformation and parameter for the transformation
    int transformationValue = 0; // Default is 0 for identity transformation
    int parameterValue = 0; // Default to 0
    
    // Now if we are here then that means there is 2 or more argument and 
    // the first argument is not -h now we have to start the big boy parsing
    // we start the index at 1 because we don't need to care about bin/birp
    for(int i=1;i<argc;i++)
    {
        // If we are encountering -h then it is defintely not the first option
        // and should result in invalid argument returning -1 
        // because it is not the first parameter
        if(compareString(*(argv + i), "-h"))
        {
            return -1;
        }
        // This option is for input
        else if(compareString(*(argv + i), "-i"))
        {
            // So no duplicates are allowed we must check it with inputSet        
            if(inputSet)
            {
                // This means that the input has been set before already hence we return error
                return -1;
            }
            
            // If we are here then that means the input hasn't been set yet so we can set it here
            // we read the next argv but we also have to make sure that the next argument does in fact exist
            if(i + 1 >= argc)
            {
                // This means that we don't have a FORMAT for -i hence it is invalid
                return -1;
            }
            else
            {
                // But if we are here then that means we do have a FORMAT argument hence we have to compare
                // and see if it is 'birp' or 'pgm'
                if(compareString(*(argv + i + 1), "birp"))
                {
                    // FORMAT is birp for input hence we set it which is 0x2
                    inputValue = 0x2;
                    
                    // We also have to set inputSet flag to true because we just set it
                    inputSet = 1;
                    
                    // And also increment the i to skip to the next argument
                    i++;
                }
                else if(compareString(*(argv + i + 1), "pgm"))
                {
                    // FORMAT is pgm for input hence we set it to 0x1
                    inputValue = 0x1;
                    
                    // Set inputSet flag to true
                    inputSet = 1;
                    
                    // INcrement i to skip to next argument
                    i++;
                }
                else
                {
                    // If we are here then that means the FORMAT argument is not birp or pgm hence we have to return -1
                    return -1;
                }
            }            
        }
        // This option is for the output
        else if(compareString(*(argv + i), "-o"))
        {
            // If the output has already been set and we are doing it again
            // return error
            if(outputSet)
            {
                return -1;
            }
            
            // Before we check for parameter we have to check if there is a argument for us to check else will be indexing error
            if(i + 1 >= argc)
            {
                return -1;
            }
            else
            {
                // Now we begin checking what to set outputValue to depending on the argument
                if(compareString(*(argv + i + 1), "birp"))
                {
                    // FORMAT is birp for output hence we set outputValue which is 0x2
                    outputValue = 0x2;
                    
                    // We also have to set outputSet flag to true because we just set it
                    outputSet = 1;
                    
                    // And also increment the i to skip to the next argument
                    i++;
                }
                else if(compareString(*(argv + i + 1), "pgm"))
                {
                    // FORMAT is pgm for output we set outputValue to 0x1
                    outputValue = 0x1;
                    
                    // We also have to set outputSet flag to true because we just set it
                    outputSet = 1;
                    
                    // And also increment the i to skip to the next argument
                    i++;
                }
                else if(compareString(*(argv + i + 1), "ascii"))
                {
                    // FORMAT is ascii for output hence we set outputValue to 0x3
                    outputValue = 0x3;
                    
                    // We also have to set outputSet flag to true because we just set it
                    outputSet = 1;
                    
                    // And also increment the i to skip to the next argument
                    i++;
                }
                else
                {
                    // If it is any other value for FORMAT it is an error
                    return -1;
                }
            }
        }
        // This option is for negative
        else if(compareString(*(argv + i), "-n"))
        {
            // If we encountered any optional argument that means we have to set
            // inputSet and outputSet all to 1 since that means we are done with positional argument
            inputSet = 1;
            outputSet = 1;
            
            // Before we can set the variables we must check that
            // transformationSet hasn't been set yet
            if(transformationSet)
            {
                // If transformationSet has able been used then we must return -1
                return -1;
            }
            
            // However if transformationSet hasn't been set yet then we will set it now
            transformationSet = 1;
            
            // And we have to set transformationValue for negative which is 0x1
            // no argument is needed for negative 
            transformationValue = 0x1;
        }
        // This option is for rotate
        else if(compareString(*(argv + i), "-r"))
        {
            // If we encountered any optional argument that means we have to set
            // inputSet and outputSet all to 1 since that means we are done with positional argument
            inputSet = 1;
            outputSet = 1;
            
            // If there is already a transformation set before this optional argument
            // we have to return -1 because we can't do multiple optional argument
            if(transformationSet)
            {
                // If transformationSet has able been used then we must return -1
                return -1;
            }
            
            // If we are here then we have to set transformationSet to 1
            transformationSet = 1;
            
            // Then for rotation the code value is 0x4 so we can just set transformationValue to 4
            transformationValue = 0x4;
        }
        // This option is for threshold
        else if(compareString(*(argv + i), "-t"))
        {
            // If we encountered any optional argument that means we have to set
            // inputSet and outputSet all to 1 since that means we are done with positional argument
            inputSet = 1;
            outputSet = 1;
            
            // Again if the transformation has been set before we return -1 as error
            if(transformationSet)
            {
                // If transformationSet has able been used then we must return -1
                return -1;
            }
            
            // Set transformationSet to 1 because we got a brand new optional argument
            transformationSet = 1;
            
            // Then since -t is for threshold we have to set transformationValue to 0x2
            transformationValue = 0x2;
            
            // Anddd -t takes in a THRESHOLD an argument that must be non-negative integer
            // that is in the integer range [0, 255]
            // So we have to first valid the next argument that it is in fact
            // a number and that number must be non-negative
            
            // Of course we perform the argument check to see if there is a 
            // next argument for us first if not then we return -1
            if(i + 1 >= argc)
            {
                return -1;
            }
            
            // However, if we are here then we can take in the next argument
            // but we must do a check to see if it is a valid numerical String first
            if(validateNumberString(*(argv + i + 1)))
            {
                // If it is a valid number String then we can proceed to convert it into number
                parameterValue = stringToInteger(*(argv + i + 1));
                
                // However if we have check the range to make sure it is between [0,255] for THRESHOLD
                if(parameterValue < 0 || parameterValue > 255)
                {
                    // Error if it is outside of the valid range
                    return -1;
                }
                
                // But if we are here then that's it the THRESHOLD variable is SET
                // increment the i just in case because we read an extra parameter
                i ++;
            }
            else
            {
                // If the given THRESHOLD is not a number then we return -1
                return -1;
            }
        }
        // This option is for zoom out
        else if(compareString(*(argv + i), "-z"))
        {
            // If we encountered any optional argument that means we have to set
            // inputSet and outputSet all to 1 since that means we are done with positional argument
            inputSet = 1;
            outputSet = 1;
            
            if(transformationSet)
            {
                // If transformationSet has able been used then we must return -1
                return -1;
            }
            
            // Now if we are here then that means this is the first time encountering a transformation argument
            // hence we set the transformationSet to true
            transformationSet = 1;
            
            // Then we set the transformationValue for -z which is just 0x3
            transformationValue = 0x3;
            
            // Then we will begin our checking for the extra argument of FACTOR
            if(i + 1 >= argc)
            {
                // If this is true then that means there is no argument supplied to -z hence we return error
                return -1;
            }
            
            // However if we are here then that means there is indeed an extra argument for us to read
            // and it must be a number hence we validate whether it is a numerical String first
            if(validateNumberString(*(argv + i + 1)))
            {
                // If it is a valid number string we have to convert it into number
                // and store that into parameter value
                parameterValue = stringToInteger(*(argv + i + 1));
                
                // Then we must check if the given FACTOR is between [0,16]
                if(parameterValue < 0 || parameterValue > 16)
                {
                    // Return -1 if it is not between [0,16]
                    return -1;
                }
                
                // However if we are here then that means the parameterValue is indeed between 0 and 16 
                // but keep in mind that because this is argument for -z we have to convert this into a negative number
                // we wrote a method to convert the positive number into negative number so we just have to call it
                parameterValue = integerTo2Complement(parameterValue);
                
                // However, also keep in mind that parameterValue is 32 bit while the we are only putting 8 bits into
                // the global_options therefore we have to remove the extra 24 bits to the left using bitwise operator
                // we will just use mask to only keep the 8 bits on the right side
                int mask = 0x000000ff;
                
                // Keeping only the rightmost 8 bits
                parameterValue = parameterValue & mask;
                
                // Now we only have the 8 bit relevant part that we care to put into the global_option and we are done!
                // just increment i just in case
                i ++;
            }
            
        }
        // This option is for zoom in
        else if(compareString(*(argv + i), "-Z"))
        {
            // If we encountered any optional argument that means we have to set
            // inputSet and outputSet all to 1 since that means we are done with positional argument
            inputSet = 1;
            outputSet = 1;
            
            if(transformationSet)
            {
                // If transformationSet has able been used then we must return -1
                return -1;
            }
            
            // However if we are here then there is no previous transformation being set
            // let's just set it here first before we start our big bois work
            transformationSet = 1;
            
            // We also set the transformationValue for -Z which is 0x3
            transformationValue = 0x3;
            
            // Now because -Z also takes in a parameter we have to check if there is
            // more parameter for us to take if not then we will return error
            if(i + 1 >= argc)
            {
                return -1;
            }
            
            // But if we are here then that means we do have another argument to read
            // Check if it is a valid number string thought
            if(validateNumberString(*(argv + i + 1)))
            {
                // If it is a valid number string we would convert it into number
                // and put it into parameterValue
                parameterValue = stringToInteger(*(argv + i + 1));
                
                // Here we must check if it is between [0, 16]
                if(parameterValue < 0 || parameterValue > 16)
                {
                    // If it is not then we return error
                    return -1;
                }
                
                // If it is between 0 and 16 then that's it since zoom factor is positive
                // just increment i just in case again
                i ++;
            }
            else
            {
                // However if the given FACTOR is not a number we return -1
                return -1;
            }
            
        }
        // Any other argument then we would just return -1 as invalid because they are not following usage
        else
        {
            return -1;
        }
        
    }
    
    // Now if we are here then that means -i, -o and transformation along with parameter is set in its corresponding variable
    // we can begin assembling the global_options variable by doing bitwise operation
    // printf("Inputvalue is 0x%08x\n", inputValue); // Print inputValue in hexadecimal to see the result
    // printf("Outputvalue is 0x%08x\n", outputValue); // Print outputValue in hexadecimal to see the result
    // printf("TransforamtionValue is 0x%08x\n", transformationValue); // Print transformationValue in hexadecimal to see the result
    // printf("parameterValue is 0x%08x\n", parameterValue); // Print parameterValue in hexadecimal to see the result
    
    // Now we must shift each value correctly and ORing them together to create the final global_option
    // parameterValue is shifted left 16 bits
    // transformationValue is shifted left 8 bits
    // outputValue is shifted left 4 bits
    // inputValue don't need any shifts stay where it is
    parameterValue = parameterValue << 16;
    transformationValue = transformationValue << 8;
    outputValue = outputValue << 4;
    
    // Then we should initialize global_options
    global_options = 0;
    
    // And we can start ORing everything together!
    global_options = global_options | parameterValue;
    global_options = global_options | transformationValue;
    global_options = global_options | outputValue;
    global_options = global_options | inputValue;
    
    // Return 0 because if we are here then everything is validated
    return 0;

}
