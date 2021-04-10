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
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>

/**
 * This function will attempt to add a given job struct into the 
 * It will return the index of the position of toAdd being added to the list
 * will return -1 if it cannot find a proper position to put the job
 */
int addJobToList(JOB toAdd);

/**
 * This function will scan the list of jobs that is queued and find one
 * that can be started
 */
void scanJobs();


int run_cli(FILE *in, FILE *out)
{
    if(in != stdin)
    {
        // This is for batch mode, when in is not equal to stdin
        printf("hahaha you failed\n");
    }
    
    int finished = 0;
    
    // While the user is not done with the program we will keep looping and display this prompt
    while(!finished)
    {
        char * userInput = sf_readline("imp> ");
        
        // When using input redirection the last line will be NULL, so have to put this condition here to signal EOF
        if(userInput == NULL)
        {
            return 0;
        }
        
        char userInputCpy[strlen(userInput) + 1];
        strcpy(userInputCpy, userInput); // Make a copy for strtok to work on
        
        char * keyword; // Store the keyword
        int numArgs = 0; // Count total of arguments after keyword
        
        // If the string we received was an empty line then we won't parse anything
        // Just present the same prompt again
        size_t strLength = strlen(userInput);
        
        keyword = strtok(userInputCpy, " "); // Keyword holds the first command that is entered by the user
        
        if(strLength == 0 || keyword == NULL)
        {
            free(userInput); // Free the string as well even though it is empty 
            continue;
        }
        
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
        // For the command 'type' which takes in one required argument
        else if(strcmp(keyword, "type") == 0)
        {
            if(numArgs != 1)
            {
                printRequiredArgs(1, numArgs, keyword, out);
                sf_cmd_error("arg count");
            }
            else
            {
                // Well we will have to do our parsing again
                strcpy(userInputCpy, userInput);
                strtok(userInput, " "); // Skip the keyword again
                char * filetype = strtok(NULL, " "); // Now we can get our filetype
                
                // We call define_type
                FILE_TYPE * returnedFileType = define_type(filetype);
                
                if(returnedFileType == NULL)
                    sf_cmd_error("lack of memory or other error"); // Don't know if this is suppose to be here
                else
                    sf_cmd_ok();
            }
        }
        // For the command 'conversion' which takes in 3 required arguments and 0 to as many optional arguments as it wants 
        else if(strcmp(keyword, "conversion") == 0)
        {
            // If it is less than 3 then we will print error
            if(numArgs < 3)
            {
                printRequiredArgs(3, numArgs, keyword, out);
                sf_cmd_error("arg count");
            }
            else
            {
                // If enough arguments are provided then we will do our parsings
                strcpy(userInputCpy, userInput);
                strtok(userInput, " "); // Skip keyword
                char * inputtype1 = strtok(NULL, " ");
                char * inputtype2 = strtok(NULL, " ");
                
                FILE_TYPE * fileType1 = find_type(inputtype1);
                FILE_TYPE * fileType2 = find_type(inputtype2);
                
                if(fileType1 == NULL)
                {
                    fprintf(out, "Unknown file type: %s\n", inputtype1);
                    sf_cmd_error("conversion");
                }
                else if(fileType2 == NULL)
                {
                    fprintf(out, "Unknown file type: %s\n", inputtype2);
                    sf_cmd_error("conversion");
                }
                else
                {
                    // However if we are able to get here then that means everything was fine
                    // all the file types are defined now we can begin inserting by calling define_conversion
                    // But we must define the array of strings that we pass in first which is just simply the program and the args it takes
                    char * cmds[numArgs - 2 + 1]; // The size is just numArgs - 2 add 1 for the null terminator element
                    
                    char * token = strtok(NULL, " ");
                    cmds[0] = token; // The conversion program name

                    for(int i=1;i<numArgs - 2;i++)
                    {
                        token = strtok(NULL, " ");
                        cmds[i] = token;
                    }
                    
                    cmds[numArgs - 2] = NULL; // Null terminator for the last element
                    
                    // Okay with all the information gathered up we can call define_conversion
                    CONVERSION * returnedConversion = define_conversion(inputtype1, inputtype2, cmds);
                    
                    if(returnedConversion == NULL)
                        sf_cmd_error("Lack of memory or other error");
                    else
                        sf_cmd_ok();
                }
            }
        }
        // For the command 'print' which will take 1 required argument
        else if(strcmp(keyword, "print") == 0)
        {
            // Only one required argument is needed
            if(numArgs < 1)
            {
                printRequiredArgs(1, numArgs, keyword, out);
                sf_cmd_error("arg count");
            }
            else
            {
                strcpy(userInputCpy, userInput);
                strtok(userInput, " "); // SKip keyword
                char * fileName = strtok(NULL, " ");
                
                char * mallocName = malloc(strlen(fileName) + 1);
                strcpy(mallocName, fileName);
                                
                // Variable to hold the pointers to the name of each printer
                char * printers = strtok(NULL, " ");
                
                FILE_TYPE * toAddType = infer_file_type(fileName);
                
                if(toAddType == NULL)
                {
                    sf_cmd_error("print can't infer file type");
                }
                // No printer is specified then all printers are eligible
                else if(printers == NULL)
                {
                    JOB toAdd;
                    toAdd.type = toAddType;
                    toAdd.eligiblePrinter = 0xffffffff;
                    toAdd.jobPositionTaken = 1;
                    toAdd.status = JOB_CREATED;
                    toAdd.filename = mallocName;
                    
                    int index = addJobToList(toAdd);
                    sf_job_created(index, fileName, toAddType->name);
                    fprintf(out, "JOB[%d]: type=%s, eligible=%x, file=%s\n", index, toAddType->name, toAdd.eligiblePrinter, fileName);
                    sf_cmd_ok();
                }
                else
                {
                    // There are specific printers that we have to add to the eligible printers
                    char * validInputPrinters[MAX_PRINTERS];
                    int i = 0;
                    
                    while(printers != NULL)
                    {  
                        if(printerExist(printers))
                        {
                            validInputPrinters[i] = printers;
                            i++;
                        }
                        printers = strtok(NULL, " ");
                    }
                    
                    unsigned int eligible = 0;
                    
                    // Now after the while loop validInputPrinters will only have names of the valid printers
                    // from the input and we will toggle those printers on
                    for(int k=0;i<MAX_PRINTERS;i++)
                    {
                        int printerIndex = getPrinterIndex(validInputPrinters[k]);
                        
                        int mask = 1 << printerIndex;
                        
                        eligible = eligible | mask;
                    }
                    
                    if(i == 0)
                    {
                        eligible = 0xffffffff;
                        printf("None of the printer specified are valid\n");
                    }
                    
                    // Finally we can make the job struct and insert it
                    JOB toAdd;
                    toAdd.eligiblePrinter = eligible;
                    toAdd.filename = mallocName;
                    toAdd.jobPositionTaken = 1;
                    toAdd.status = JOB_CREATED;
                    toAdd.type = toAddType;
                    
                    int index = addJobToList(toAdd);
                    sf_job_created(index, fileName, toAddType->name);
                    fprintf(out, "JOB[%d]: type=%s, eligible=%x, file=%s\n", index, toAddType->name, toAdd.eligiblePrinter, fileName);
                    sf_cmd_ok();
                }
            }
        }
        else if(strcmp(keyword, "jobs") == 0)
        {
            if(numArgs != 0)
            {
                printRequiredArgs(0, numArgs, keyword, out);
                sf_cmd_error("arg count");
            }
            else
            {
                // Print the jobs list
                for(int i=0;i<MAX_JOBS;i++)
                {
                    JOB currentJob = list_jobs[i];
                    
                    if(currentJob.jobPositionTaken)
                    {
                        fprintf(out, "JOB[%d]: type=%s, eligible=%x, file=%s\n", i, currentJob.type->name, currentJob.eligiblePrinter, currentJob.filename);
                        sf_job_status(i, currentJob.status);
                    }
                }
                
                sf_cmd_ok();
            }
        }
        // For the command 'printer' which will take in 2 required arguments
        else if(strcmp(keyword, "printer") == 0)
        {
            if(numArgs != 2)
            {
                printRequiredArgs(2, numArgs, keyword, out);
                sf_cmd_error("arg count");
            }
            else
            {
                strcpy(userInputCpy, userInput);
                strtok(userInput, " "); // Skip keyword
                char * printerName = strtok(NULL, " "); // Printer name
                char * fileType = strtok(NULL, " "); // File type
                
                FILE_TYPE * foundType = find_type(fileType);
                
                // We hit the maximum number of printers we cannot make anymore
                if(nextFreeIndex == MAX_PRINTERS)
                {
                    fprintf(out, "Too many printers (32 max)\n");
                    sf_cmd_error("printer");
                }
                // Must check if the fileType is declared before
                else if(foundType == NULL)
                {
                    fprintf(out, "Unknown file type: %s\n", fileType);
                    sf_cmd_error("printer");
                }
                else if(printerExist(printerName))
                {
                    fprintf(out, "Printer name already exist\n");
                    sf_cmd_error("printer");
                }
                else
                {
                    char * mallocName = malloc(strlen(printerName) + 1); // Plus 1 for the null terminator
                    strcpy(mallocName, printerName);
                    
                    // Now we are going to create a new printer by adding it to the global array list_printers
                    PRINTER toInsert = {mallocName, PRINTER_DISABLED, foundType, nextFreeIndex};
                    
                    // Insert it into the array
                    list_printers[nextFreeIndex] = toInsert;
                    nextFreeIndex++; // Increment the counter
                    
                    sf_printer_defined(toInsert.printerName, toInsert.type->name);
                    sf_printer_status(toInsert.printerName, toInsert.status);
                    fprintf(out, "PRINTER: id=%d, name=%s, type=%s, status=%s\n"
                    , toInsert.id, toInsert.printerName, toInsert.type->name, printer_status_names[toInsert.status]);
                    sf_cmd_ok();
                }
            }
        }
        // For the command 'printers'. Takes no arguments just report status of declared printers
        else if(strcmp(keyword, "printers") == 0)
        {
            if(numArgs != 0)
            {
                printRequiredArgs(0, numArgs, keyword, out);
                sf_cmd_error("arg count");
            }
            else
            {
                // We will simply loop through the printer list
                for(int i=0;i<nextFreeIndex;i++)
                {
                    PRINTER currentPrinter = list_printers[i];
                    
                    // First the status by doing the event function
                    sf_printer_status(currentPrinter.printerName, currentPrinter.status);
                    fprintf(out, "PRINTER: id=%d, name=%s, type=%s, status=%s\n", currentPrinter.id, currentPrinter.printerName,
                    currentPrinter.type->name, printer_status_names[currentPrinter.status]);
                }
                sf_cmd_ok();
            }
        }
        else if(strcmp(keyword, "enable") == 0)
        {
            if(numArgs != 1)
            {
                printRequiredArgs(1, numArgs, keyword, out);
                sf_cmd_error("arg count");
            }
            else
            {
                strcpy(userInputCpy, userInput);
                strtok(userInput, " "); // Skip keyword
                
                char * printerName = strtok(NULL, " ");
                
                if(printerExist(printerName))
                {
                    // The printer exist hence we will switch the status of the printer
                    // to be printer_idle
                    int index = getPrinterIndex(printerName);
                    
                    PRINTER * printerPtr = &list_printers[index];
                    printerPtr->status = PRINTER_IDLE;
                    fprintf(out, "status: %s\n", printer_status_names[PRINTER_DISABLED]);
                    scanJobs();
                    
                }
                else
                {
                    sf_cmd_error("enable (no printer)");
                }
            }
        }
        else if(strcmp(keyword, "test") == 0)
        {
            CONVERSION ** arr = find_conversion_path("pdf", "pdf");
            printf("first element %p\n", arr);
            
        }
        // For the command 'quit'
        else if(strcmp(keyword, "quit") == 0)
        {
            if(numArgs != 0)
            {
                printRequiredArgs(0, numArgs, keyword, out);
                sf_cmd_error("arg count");
            }
            else
            {
                // Just put ok command and return that's it
                // Don't forget to free the string that is malloc
                free(userInput);
                sf_cmd_ok();
                return -1;
            }
        }
        // Before we begin our new iteration we have to free userInput, because it
        // malloc a new string dynamically every call to sf_readline
        free(userInput);
        
    }
    
    // TODO We must free all the printer names that we have used or else it will be memory leaks
    return 0;
    
}

int addJobToList(JOB toAdd)
{
    // Use a for loop to scan the list of jobs and find the first one that is not occupied
    for(int i=0;i<MAX_JOBS;i++)
    {
        // Gets a copy of the i-th job
        JOB currentJob = list_jobs[i];
        
        // If the job position is not taken then we will insert toAdd to that position
        if(currentJob.jobPositionTaken == 0)
        {
            list_jobs[i] = toAdd;
            
            return i;
        }
    }
    
    return -1;
}

int formatMatch(char * printerType, char * fileType)
{
    if(strcmp(printerType, fileType) == 0)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

void scanJobs()
{
    for(int i=0;i<MAX_JOBS;i++)
    {
        JOB * jobPtr = &list_jobs[i];
        
        if(jobPtr->jobPositionTaken && jobPtr->status == JOB_CREATED)
        {
            // This means that this job is queued, we will see if the any of the eligible printer can be used
            // to print this file
            int mask = 1;
            
            for(int i=0;i<nextFreeIndex;i++)
            {
                mask <<= i;
                int result = jobPtr->eligiblePrinter & mask;
                
                PRINTER * printerPtr = &list_printers[i];
                
                // If the result is not 0 then that means index i of list_printers is an eligible printer that can be used
                if(result)
                {
                    // Use bin/cat to do the conversion because that's the same format
                    if(formatMatch(jobPtr->type->name, printerPtr->type->name))
                    {
                        pid_t id = fork();
                        
                        if(id == 0)
                        {
                            // Child process the master process
                            // setpgid(0, 0);
                            
                            int printerFd = imp_connect_to_printer(printerPtr->printerName, printerPtr->type->name, PRINTER_NORMAL);
                            
                            if(fork() == 0)
                            {
                                int openedFileFd = open(jobPtr->filename, O_RDONLY);
                                
                                dup2(openedFileFd, 0);
                                dup2(printerFd, 1);
                                
                                char * arg[] = {"/bin/cat", NULL};
                                execvp("/bin/cat", arg);
                            }
                            else
                            {
                                pid_t returned;
                                wait(&returned);
                                printf("reaped in master\n");
                                exit(0);
                            }
                        }
                        else
                        {
                            printf("Im in the main process\n");
                            int status;
                            waitpid(id, &status, 0);
                        }
                    }
                    else
                    {
                        // Need to check if there is a conversion path from fileType to printerType
                        
                        
                    }
                }
            }
        }
    }
}