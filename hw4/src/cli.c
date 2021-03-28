/*
 * Imprimer: Command-line interface
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "imprimer.h"
#include "conversions.h"
#include "sf_readline.h"
#include "helpingFunction.h"

int run_cli(FILE *in, FILE *out)
{
    // TO BE IMPLEMENTED. Please kill me
    // fprintf(stderr, "You have to implement run_cli() before the application will function.\n");
    // abort();
    int finished = 0;
    
    // While the user is not done with the program we will keep looping and display this prompt
    while(!finished)
    {
        char * userInput = sf_readline("imp> ");
        
        char * keyword; // Variable used for storing the keyword command. 20 character is more than enough
        int numArgs = 0; // Variable used to count the number of arguments we have encountered
        
        // If the string we received was an empty line then we won't parse anything
        // Just present the same prompt again
        size_t strLength = strlen(userInput);
        if(strLength == 0)
        {
            continue;
        }
        
        // We begin parsing
        keyword = strtok(userInput, " "); // Keyword holds the first command that is entered by the user
        
        // Then we parse the subsequent arguments
        char * token = strtok(NULL, " ");
        
        // This loop count the number of arguments that is passed
        while(token != NULL)
        {
            numArgs++;
            
            token = strtok(NULL, " ");
        }
        
        // For the command 'help'
        if(strcmp(keyword, "help") == 0)
        {
            if(numArgs != 0)
            {
                printRequiredArgs(0, numArgs, keyword, out);
                sf_cmd_error("arg count");
            }
            else
            {
                fprintf(out, "Commands are: help quit type printer conversion printers jobs print cancel disable enable pause resume\n");
                sf_cmd_ok();
            }
        }
        // For the command 'quit'
        else if(strcmp(userInput, "quit") == 0)
        {
            if(numArgs != 0)
            {
                printRequiredArgs(0, numArgs, keyword, out);
                sf_cmd_error("arg count");
            }
            else
            {
                // Just put ok command and return that's it
                sf_cmd_ok();
                return;
            }
        }
    
    }
    
    

    
    
    
}
