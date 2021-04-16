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
#include <fcntl.h>
#include <signal.h>
#include <time.h>

volatile sig_atomic_t startReaping;
int reapedChild;
volatile sig_atomic_t doneReapingAll;
volatile sig_atomic_t errorChild;
volatile int childstatus;

volatile int masterChildCount = 0;

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

void masterSigHandler()
{
    sigset_t signalMask;
    sigset_t prevMask;
    sigfillset(&signalMask);
    
    sigprocmask(SIG_BLOCK, &signalMask, &prevMask);
    
    startReaping = 1;
    if(startReaping)
    {
        int childstatus = 0;
        pid_t masterProcessId;
        
        // Start reaping the childs
        while((masterProcessId = waitpid(-1, &childstatus, WNOHANG | WCONTINUED | WSTOPPED)) > 0)
        {
            int jobIndex = findJobIndex(masterProcessId);
            // Then job should enter state JOB_FINISHED state
            JOB * jobPtr = &list_jobs[jobIndex];
            
            // If the child was signal by SIGTERM to stop
            if(WIFSIGNALED(childstatus) != 0)
            {
                jobPtr->status = JOB_ABORTED;
                jobPtr->wait4TenSecond = time(NULL);
                
                sf_job_status(jobIndex, jobPtr->status);
                sf_job_aborted(jobIndex, childstatus);
                
                // Printer should enter PRINTER_IDLE, or stays at PRINTER_DISABLED if it was disabled
                int printerIndex = findPrinterByJobIndex(jobIndex);
                
                PRINTER * printerPtr = &list_printers[printerIndex];
                printerPtr->jobIndex = -1;
                
                if(printerPtr->status != PRINTER_DISABLED)
                    printerPtr->status = PRINTER_IDLE;
                    
                sf_printer_status(printerPtr->printerName, printerPtr->status);
                masterProcessId --;
            }
            // If the master process was stopped then we will set the job status to be
            // PAUSED and the child is not reaped
            else if(WIFSTOPPED(childstatus) != 0)
            {
                jobPtr->status = JOB_PAUSED;
                sf_job_status(jobIndex, jobPtr->status);
            }
            else if(WIFCONTINUED(childstatus) != 0)
            {
                jobPtr->status = JOB_RUNNING;
                sf_job_status(jobIndex, jobPtr->status);
            }
            // The master process exited normally hence we can
            // set the job status to be finished, the wait 10 second and the printer status
            else if(WIFEXITED(childstatus) != 0)
            {
                jobPtr->status = JOB_FINISHED;
                jobPtr->wait4TenSecond = time(NULL);
                
                sf_job_status(jobIndex, jobPtr->status);
                sf_job_finished(jobIndex, childstatus);
                
                // Printer should enter PRINTER_IDLE, or stays at PRINTER_DISABLED if it was disabled
                int printerIndex = findPrinterByJobIndex(jobIndex);
                
                PRINTER * printerPtr = &list_printers[printerIndex];
                printerPtr->jobIndex = -1;
                
                if(printerPtr->status != PRINTER_DISABLED)
                    printerPtr->status = PRINTER_IDLE;
                
                sf_printer_status(printerPtr->printerName, printerPtr->status);
                masterChildCount--;
                
            }
            // However if the child exited abnormally then we will have to set
            // the job status to be JOB_ABORTED, and still be deleted
            else if(WIFEXITED(childstatus) == 0)
            {
                jobPtr->status = JOB_ABORTED;
                jobPtr->wait4TenSecond = time(NULL);
                
                sf_job_status(jobIndex, jobPtr->status);
                sf_job_aborted(jobIndex, childstatus);
                
                // Printer should enter PRINTER_IDLE, or stays at PRINTER_DISABLED if it was disabled
                int printerIndex = findPrinterByJobIndex(jobIndex);
                
                PRINTER * printerPtr = &list_printers[printerIndex];
                printerPtr->jobIndex = -1;
                
                if(printerPtr->status != PRINTER_DISABLED)
                    printerPtr->status = PRINTER_IDLE;
                
                sf_printer_status(printerPtr->printerName, printerPtr->status);
                masterChildCount--;
            }
        }
        
        startReaping = 0;
    } 
    
    sigprocmask(SIG_SETMASK, &prevMask, NULL);
}

void readlineCallback()
{
    scanJobs();
    deleteJobs();
}

void mastersigchld_handler()
{
    sigset_t mask;
    sigset_t prev;
    sigfillset(&mask);
    
    int childStatus;
    pid_t childId;
    
    // Block singal
    sigprocmask(SIG_BLOCK, &mask, &prev);
    while((childId = waitpid(-1, &childStatus, WNOHANG)) > 0)
    {
        // Reap the child
        // The child did not terminate normally, errorChild set to 1 but still reaps it
        // so we decrease reapedChild
        if(WIFEXITED(childStatus) == 0)
        {
            errorChild = 1;
            reapedChild --;
        }
        // The child did terminate normally, reap and decrease reapedChild
        else if(WIFEXITED(childstatus) != 0)
        {
            reapedChild --;
        }
    }
    
    sigprocmask(SIG_SETMASK, &prev, NULL);
    if(reapedChild == 0)
    {
        doneReapingAll = 1;
    }
}

int run_cli(FILE *in, FILE *out)
{
    // First we install the signal handler for the SIGCHID
    signal(SIGCHLD, masterSigHandler);
    sf_set_readline_signal_hook(readlineCallback);
    
    int finished = 0;
    int suppressPrompt = 0;
    size_t bufferSize = 64;
    int byteRead = 0;
    int batchMode = 0;
    
    if(in == NULL)
        return -1;
    
    if(in != stdin)
    {
        batchMode = 1;
    }
    
    if(out != stdout)
    {
        suppressPrompt = 1;
    }
    
    // While the user is not done with the program we will keep looping and display this prompt
    while(!finished)
    {
        char * userInput;
        
        // Run in batch mode
        if(batchMode && byteRead != EOF)
        {
            userInput = (char *)malloc(bufferSize); // Hopefully 64 bytes is enough
            
            if(userInput == NULL)
            {
                fprintf(out, "Error with mallocing\n");
                exit(1);
            }
            
            byteRead = getline(&userInput, &bufferSize, in);
            
            if(byteRead != EOF && byteRead != 0)
            {
                removeNewline(userInput, byteRead);
            }
            else
            {
                free(userInput);    
                break;
                // return 0; // EOF but no quit is encountered hence return 0
            }
        }
        else
        {
            if(!suppressPrompt)
                userInput = sf_readline("imp> ");
            else
                userInput = sf_readline(NULL);
            
            // Done collecting input from stdin, hence we will just return 0
            if(userInput == NULL)
            {
                break;
                // return -1;
            }
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
                
                if(mallocName == NULL)
                {
                    fprintf(out, "Error with mallocing\n");
                    exit(1);
                }
                
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
                    toAdd.wait4TenSecond = -1; // Not finshed yet so the time is -1
                    
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
                    }
                    
                    // Finally we can make the job struct and insert it
                    JOB toAdd;
                    toAdd.eligiblePrinter = eligible;
                    toAdd.filename = mallocName;
                    toAdd.jobPositionTaken = 1;
                    toAdd.status = JOB_CREATED;
                    toAdd.type = toAddType;
                    toAdd.wait4TenSecond = -1;
                    
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
                        fprintf(out, "JOB[%d]: type=%s, eligible=%x, file=%s status=%s\n", i, currentJob.type->name, currentJob.eligiblePrinter, currentJob.filename, job_status_names[currentJob.status]);
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
                    
                    if(mallocName == NULL)
                    {
                        fprintf(out, "Error with mallocing\n");
                        exit(1);
                    }
                    
                    strcpy(mallocName, printerName);
                    
                    // Now we are going to create a new printer by adding it to the global array list_printers
                    PRINTER toInsert = {mallocName, PRINTER_DISABLED, foundType, nextFreeIndex, -1};
                    
                    // Insert it into the array
                    list_printers[nextFreeIndex] = toInsert;
                    nextFreeIndex++; // Increment the counter
                    
                    sf_printer_defined(toInsert.printerName, toInsert.type->name);
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
                    sf_printer_status(printerPtr->printerName, printerPtr->status);
                    sf_cmd_ok();
                    
                }
                else
                {
                    sf_cmd_error("enable (no printer)");
                }
            }
        }
        // Command for disabling the printer, let it finishes its job then make it disable
        else if(strcmp(keyword, "disable") == 0)
        {
            if(numArgs != 1)
            {
                printRequiredArgs(0, numArgs, keyword, out);
                sf_cmd_error("arg count");
            }
            else
            {
                strcpy(userInputCpy, userInput);
                strtok(userInputCpy, " "); // Skip keyword again
                
                char * printerName = strtok(NULL, " ");
                
                if(printerExist(printerName))
                {
                    // Set the status of the printer to disable
                    int index = getPrinterIndex(printerName);
                    
                    PRINTER * printerPtr = &list_printers[index];
                    printerPtr->status = PRINTER_DISABLED;
                    sf_printer_status(printerPtr->printerName, printerPtr->status);
                    sf_cmd_ok();
                }
                else
                {
                    sf_cmd_error("disable (no printer)");
                }
            }
        }
        else if(strcmp(keyword, "pause") == 0)
        {
            if(numArgs != 1)
            {
                printRequiredArgs(0, numArgs, keyword, out);
                sf_cmd_error("arg count");
            }
            else
            {
                strcpy(userInputCpy, userInput);
                strtok(userInputCpy, " "); // Skip keyword again
                
                char * stringJobNumber = strtok(NULL, " ");
                int jobNumber = string2Integer(stringJobNumber);
                
                if(jobNumber == -1)
                {
                    sf_cmd_error("pause (invalid position of job given)");
                }
                else
                {
                    JOB * jobPtr = &list_jobs[jobNumber];
                    
                    if(jobPtr->jobPositionTaken == 1 && jobPtr->status == JOB_RUNNING)
                    {
                        // Sents a SIGSTOP signal to the entire process group but don't update it immediately
                        killpg(getpgid(jobToMasterId[jobNumber]), SIGSTOP);
                        sf_cmd_ok();
                    }
                    else
                    {
                        sf_cmd_error("pause (no job or job is not currently running");
                    }
                }
            }
        }
        else if(strcmp(keyword, "resume") == 0)
        {
            if(numArgs != 1)
            {
                printRequiredArgs(0, numArgs, keyword, out);
                sf_cmd_error("arg count");
            }
            else
            {
                strcpy(userInputCpy, userInput);
                strtok(userInputCpy, " "); // Skip keyword again
                
                char * stringJobNumber = strtok(NULL, " ");
                int jobNumber = string2Integer(stringJobNumber);
                
                if(jobNumber == -1)
                {
                    sf_cmd_error("resume (invalid position of job)");
                }
                else
                {
                    JOB * jobPtr = &list_jobs[jobNumber];
                    
                    if(jobPtr->jobPositionTaken == 1 && jobPtr->status == JOB_PAUSED)
                    {
                        // Sents a SIGCONT signal to the entire process group but don't update it immediately
                        killpg(getpgid(jobToMasterId[jobNumber]), SIGCONT);
                        sf_cmd_ok();
                    }
                    else
                    {
                        sf_cmd_error("resume (no job or job is not currently paused)");
                    }
                }
            }
        }
        else if(strcmp(keyword, "cancel") == 0)
        {
            if(numArgs != 1)
            {
                printRequiredArgs(0, numArgs, keyword, out);
                sf_cmd_error("arg count");
            }
            else
            {
                strcpy(userInputCpy, userInput);
                strtok(userInputCpy, " "); // Skip keyword again
                
                char * stringJobNumber = strtok(NULL, " ");
                int jobNumber = string2Integer(stringJobNumber);
                
                if(jobNumber == -1)
                {
                    sf_cmd_error("cancel (invalid position of job)");
                }
                else
                {
                    JOB * jobPtr = &list_jobs[jobNumber];
                    
                    if(jobPtr->jobPositionTaken == 1 && jobPtr->status == JOB_RUNNING)
                    {
                        // Sent the SIGTERM signal to the job
                        killpg(getpgid(jobToMasterId[jobNumber]), SIGTERM);
                        sf_cmd_ok();
                    }
                    else if(jobPtr->jobPositionTaken == 1 && jobPtr->status == JOB_PAUSED)
                    {
                        // If the paused, then after sending SIGTERM it should be sent SIGCONT to be
                        // able to respond to the SIGTERM signal
                        killpg(getpgid(jobToMasterId[jobNumber]), SIGTERM);
                        killpg(getpgid(jobToMasterId[jobNumber]), SIGCONT);
                        sf_cmd_ok();
                    }
                    else
                    {
                        sf_cmd_error("cancel (no job)");
                    }
                }
            }
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
                finished = 1;
                sf_cmd_ok();
            }
        }
        // Before we begin our new iteration we have to free userInput, because it
        // malloc a new string dynamically every call to sf_readline
        free(userInput);
    }
    
    // Batchmode and a quit is read, hence we must just return -1 here
    // no second call to run_cli. All the child must be finished before we exit
    if(batchMode && finished)
    {
        sigset_t suspendMask;
        sigfillset(&suspendMask);
        sigdelset(&suspendMask, SIGCHLD);
        
        while(masterChildCount != 0)
        {
            sigsuspend(&suspendMask);
        }
        
        freeAllJobs();
        freeAllPrinters();
        
        return -1; // In batch mode and quit was read hence return -1
    }
    // Batchmode and no quit was read, just return 0
    else if(batchMode && !finished)
    {
        // A second call to run_cli with interactive mode. Don't need to free anything
        // Don't have to wait for master process to finish because we are not quitting the program yet
        return 0;
    }
    
    sigset_t suspendMask;
    sigemptyset(&suspendMask);
    sigdelset(&suspendMask, SIGCHLD);
    
    // However if we are here then that means a quit command was executed in interactive mode
    // we must free the printers, the jobs, and wait for all master process to exit
    while(masterChildCount != 0)
    {
        sigsuspend(&suspendMask);
    }
    
    // If we are here then that means a quit command was executed in interactive mode
    // That means we can free the printers and the jobs we did
    freeAllJobs();
    freeAllPrinters();
    return -1;
    
}

int addJobToList(JOB toAdd)
{
    // Use a for loop to scan the list of jobs and find the first one that is not occupied
    for(int i=0;i<MAX_JOBS;i++)
    {
        // Gets a copy of the i-th job
        JOB * currentJob = &list_jobs[i];
        
        // If the job position is not taken then we will insert toAdd to that position
        if(currentJob->jobPositionTaken == 0 || currentJob->status == JOB_DELETED)
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
    sigset_t sigmask;
    sigset_t sigprev;
    sigfillset(&sigmask);
    
    sigprocmask(SIG_BLOCK, &sigmask, &sigprev);
    
    for(int i=0;i<MAX_JOBS;i++)
    {
        JOB * jobPtr = &list_jobs[i];
        
        if(jobPtr->jobPositionTaken && jobPtr->status == JOB_CREATED)
        {
            // This means that this job is queued, we will see if the any of the eligible printer can be used
            // to print this file
            int mask = 1;
            
            for(int k=0;k<nextFreeIndex;k++)
            {
                mask <<= k;
                int result = jobPtr->eligiblePrinter & mask;
                
                PRINTER * printerPtr = &list_printers[k];
                
                // If the result is not 0 then that means index i of list_printers is an eligible printer that can be used
                if(result && printerPtr->status == PRINTER_IDLE && printerPtr->jobIndex == -1)
                {
                    // Use bin/cat to do the conversion because that's the same format
                    if(formatMatch(jobPtr->type->name, printerPtr->type->name))
                    {
                        // Printer and job are set to busy and put to work
                        printerPtr->status = PRINTER_BUSY;
                        printerPtr->jobIndex = i;
                        jobPtr->status = JOB_RUNNING;      
                        
                        sf_job_status(i, jobPtr->status);
                        sf_printer_status(printerPtr->printerName, printerPtr->status);        
                        
                        pid_t id = fork();
                        jobToMasterId[i] = id;
                        masterChildCount ++;
                        
                        if(id == 0)
                        {
                            sigprocmask(SIG_SETMASK, &sigprev, NULL);
                            
                            // Child process the master process
                            setpgid(0, 0);
                            
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
                                // printf("reaped in master\n");
                                exit(0);
                            }
                        }
                        else
                        {
                            setpgid(id, id);
                            char * pathArg[] = {"/bin/cat", NULL};
                            sf_job_started(i, printerPtr->printerName, getpgid(id), pathArg);
                        }
                        
                        break; // Break to go to next job
                        // Main program will continue. The reap will occur when child is finished
                    }
                    else
                    {
                        // Need to check if there is a conversion path from fileType to printerType
                        // Must call find_conversion_path
                        CONVERSION ** path = find_conversion_path(jobPtr->type->name, printerPtr->type->name);
                        
                        // There is a path from the job type to the printer type
                        if(path != NULL)
                        {
                            int conversionLength = lengthOfConversionPath(path);
                            
                            // pid_t masterid = fork();
                            int printerFd = imp_connect_to_printer(printerPtr->printerName, printerPtr->type->name, PRINTER_NORMAL);
                            
                            int fd[2]; // For storing the file descriptor
                            int prevFd; // For storing the read end of the previous pipe fd value
                            
                            jobPtr->status = JOB_RUNNING;
                            printerPtr->status = PRINTER_BUSY;
                            printerPtr->jobIndex = i;
                            
                            sf_job_status(i, jobPtr->status);
                            sf_printer_status(printerPtr->printerName, printerPtr->status);
                            
                            masterChildCount ++;
                            
                            int masterId = fork();
                            jobToMasterId[i] = masterId;
                            
                            if(masterId == 0) // The master process for forking all the other child for conversion pipeline
                            {
                                signal(SIGCHLD, mastersigchld_handler); // Signal handler
                                sigprocmask(SIG_SETMASK, &sigprev, NULL);
                                setpgid(0, 0);

                                reapedChild = conversionLength;
                                
                                // int childCount = conversionLength;
                                // int childStatus[conversionLength];
                                
                                // sigset_t mask;
                                // sigset_t prev;
                                // sigfillset(&mask);
                                
                                // sigprocmask(SIG_BLOCK, &mask, &prev);
                                
                                // Then we must set up the pipeline for it if it is 2 or more conversion required
                                // Need a for loop to go through each CONVERSION
                                for(int i=0;i<conversionLength;i++)
                                {
                                    // Only one process is needed hence we just need to set up one fork no need for pipeline
                                    if(i == 0 && i == conversionLength - 1)
                                    {
                                        if(fork() == 0)
                                        {
                                            // Child process that will be doing the work
                                            int openedFile = open(jobPtr->filename, O_RDONLY);
                                            
                                            dup2(openedFile, 0); // Replace stdin
                                            dup2(printerFd, 1); // Repalce stdout
                                            
                                            char * cmd = *(path[0]->cmd_and_args); // Get the command
                                            
                                            execvp(cmd, path[0]->cmd_and_args);
                                        }
                                        else
                                        {
                                            close(printerFd);
                                        }
                                    }
                                    else if(i == 0)
                                    {
                                        int pipeStatus = pipe(fd);
                                        prevFd = fd[0];
                                        
                                        if(pipeStatus != 0)
                                        {
                                            fprintf(stderr, "Pipe messed up\n");
                                            exit(1);
                                        }
                                        
                                        // If it first one then we will just open the file, call pipe, redirect stdout to the pipe
                                        if(fork() == 0)
                                        {
                                            
                                            close(fd[0]); // Close the read side of the pipe since the child won't use it
                                            int openedFile = open(jobPtr->filename, O_RDONLY);
                                            
                                            dup2(openedFile, 0); // Replace stdin for the first process because it takes in the file
                                            dup2(fd[1], 1); // Replace stdout with the write side of the pipe
                                            
                                            char * cmd = *(path[i]->cmd_and_args);
                                            
                                            execvp(cmd, path[i]->cmd_and_args);
                                        }
                                        else
                                        {
                                            // Close the write side for the master process since it is not using it
                                            close(fd[1]);
                                        }
                                    }
                                    else if(i == conversionLength - 1)
                                    {
                                        // Finally this is the final process, it doesn't need to make a new pipeline
                                        // just need to take in the stdout of the previous pipeline as stdin
                                        // stdout will just be the printer file descriptor
                                        if(fork() == 0)
                                        {
                                            dup2(prevFd, 0);
                                            dup2(printerFd, 1);
                                            
                                            char * cmd = *(path[i]->cmd_and_args);
                                            
                                            execvp(cmd, path[i]->cmd_and_args);
                                        }
                                        else
                                        {
                                            close(printerFd); // Master process close printerFd
                                            close(prevFd); // Close the read side of the pipe as well because it doesn't need it
                                        }
                                    }
                                    else
                                    {
                                        int pipeStatus = pipe(fd);
                                        
                                        if(pipeStatus != 0)
                                        {
                                            fprintf(stderr, "Pipe messed up\n");
                                            exit(1);
                                        }
                                        
                                        // Now in the else case we will be connecting the pipe and making the pipe as well
                                        if(fork() == 0)
                                        {
                                            
                                            close(fd[0]); // Close the read side of the new pipe, because it reads from prevFd
                                            
                                            dup2(prevFd, 0); // Replace stdin with the read side of the pipe
                                            dup2(fd[1], 1); // Replace stdout with write side of the pipe
                                            
                                            char * cmd = *(path[i]->cmd_and_args);
                                            
                                            execvp(cmd, path[i]->cmd_and_args);
                                        }
                                        else
                                        {       
                                            // Master should close prevFd because it isn't using it on its own
                                            // the forked child is using it not the master process
                                            close(prevFd);
                                            
                                            // Then it should set prevFd to be fd[0] after pipe call
                                            prevFd = fd[0];
                                            
                                            // Then master should close write side because it is not writing
                                            close(fd[1]);
                                        }
                                    }
                                }   
                                close(printerFd); // Close the printer file descriptor because the masster process doesn't use it
                                
                                sigset_t suspendMask;
                                sigfillset(&suspendMask);
                                sigdelset(&suspendMask, SIGCHLD);
                                
                                while(!doneReapingAll)
                                {
                                    sigsuspend(&suspendMask);
                                }
                                
                                
                                // printf("child count now %d\n", childCount);
                                
                                // Have to check all the child's exit status before exiting with normal
                                if(errorChild)
                                {
                                    exit(1);
                                }
                                else
                                {
                                    exit(0);
                                }
                            }
                            else
                            {
                                setpgid(masterId, masterId);
                                // Printer and job are set to busy and put to work
                                
                                close(printerFd);
                                
                                char * pathArg[conversionLength + 1];
                                
                                for(int i=0;i<conversionLength;i++)
                                {
                                    pathArg[i] = path[i]->cmd_and_args[0];
                                }
                                
                                pathArg[conversionLength] = NULL;
                                
                                sf_job_started(i, printerPtr->printerName, getpgid(masterId), pathArg);
                            }
                            // Main program will continue and go on   
                            free(path); // Free the conversion path after being used
                              
                            break; // Break  to start next available job              
                        }
                    }
                }
            }
        }
                                
    }
    
    sigprocmask(SIG_SETMASK, &sigprev, NULL);
}
