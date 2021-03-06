#include "imprimer.h"
#include <time.h>

// Index for the next free position
extern int nextFreeIndex;

/**
 * Define the printer struct that wasn't defined in conversions
 */
typedef struct printer
{
    char * printerName;
    int status;
    FILE_TYPE * type;
    int id; // Id for the printer in the array list
    int jobIndex; // The index of the job that this printer is working on, -1 if it is idle
} PRINTER;

// The global printer array which keeps track of all the declared printer
PRINTER list_printers[MAX_PRINTERS];


/**
 * Define the job struct that wasn't defined in conversions
 */
typedef struct job
{
    FILE_TYPE * type; // Type for the file
    char * filename;
    int status;
    unsigned int eligiblePrinter; // 32 bitmap that will tell which printer is elibile to be used to print this job. Read right to left
    char jobPositionTaken; // A 1 or 0 to indicate if this job in the list is occupied already or not
    time_t wait4TenSecond; // The current time when this job's status is set to JOB_FINISHED
} JOB;

// The global job array which keeps track of all the jobs, gotta implement it like a queue i suppose
JOB list_jobs[MAX_JOBS];

// An array which maps the job index to the master process id which processes it
pid_t jobToMasterId[MAX_JOBS];


/**
 * Print a string that states the required argument and the actual number of argument given
 */
void printRequiredArgs(int required, int given, char * command, FILE * out);

/**
 * This function will attempt to see if a printer of given name already exist in the global array list
 * Return 1 if the printer of given name exist
 * Return 0 if the printer of given name doesn't exist
 */
int printerExist(char * printerName)
{
    for(int i=0;i<nextFreeIndex;i++)
    {
        PRINTER currentPrinter = list_printers[i];
        
        if(strcmp(printerName, currentPrinter.printerName) == 0)
        {
            return 1;
        }
    }
    
    return 0;
}

/**
 * This function will find the index of the printer of the given name in the array
 * Return -1 if it couldn't find it
 */
int getPrinterIndex(char * printerName)
{
    for(int i=0;i<nextFreeIndex;i++)
    {
        PRINTER currentPrinter = list_printers[i];
        
        if(strcmp(printerName, currentPrinter.printerName) == 0)
        {
            return i;
        }
    }
    return -1;
}

/**
 * Function which will be called to free all of the printers
 */
void freeAllPrinters()
{
    for(int i=0;i<nextFreeIndex;i++)
    {
        PRINTER * printerPtr = &list_printers[i];
        free(printerPtr->printerName);
    }
}

/**
 * Function which is called to free all of the jobs in the list
 */
void freeAllJobs()
{
    for(int i=0;i<MAX_JOBS;i++)
    {
        JOB * jobPtr = &list_jobs[i];
        
        if(jobPtr->jobPositionTaken == 1)
            free(jobPtr->filename);
    }
}

/**
 * Returns the number of conversion that is required in the CONVERSION ** path
 */
int lengthOfConversionPath(CONVERSION ** path)
{
    if(path == NULL)
        return 0;
    
    // If you are going to make a pointer to the array for poitner arithemtic, it should
    // have the same time as the array that decays into
    // int arr[2];, the pointer should be int * ptr because arr decays to pointer in the end
    CONVERSION ** start = path; // This is the correct way of doing it, just make it equal to address of path
    
    int counter = 0;
    // CONVERSION * ptr1 = *(path); // This is wrong because I'm getting the element of path array directly
    //                             // Which gives you an CONVERSION ptr, and when you increment you add the size of CONVERSION struct
    // CONVERSION * ptr2 = *(path + 1);
    
    
    // Then dereference and read it in the loop condition
    while(*(start) != NULL)
    {
        counter++;
        start ++;
    }
    
    // int i = 0;
    
    // while(path[i] != NULL)
    // {
    //     counter ++;
    //     i ++;
    // }
    
    return counter;
}

/**
 * This function removes the \n from a string
 */
void removeNewline(char * str, int byteRead);

/**
 * Given a master process id find the job index that appears in the jobToMasterId array
 */
int findJobIndex(pid_t masterId)
{
    for(int i=0;i<MAX_JOBS;i++)
    {
        if(jobToMasterId[i] == masterId)
        {
            return i;
        }
    }
    
    return -1;
}

/**
 * Find the printer index that is working on the job based on a job index
 * Return -1 if it cannot find it
 */
int findPrinterByJobIndex(int jobIndex)
{
    for(int i=0;i<nextFreeIndex;i++)
    {
        PRINTER * printerPtr = &list_printers[i];

        if(printerPtr->jobIndex != -1 && printerPtr->jobIndex == jobIndex)
            return i;
    }
    
    return -1;
}

/**
 * This function will look through the job list to delete all the jobs that is FINISHED
 * and 10 seconds have elapsed since wait4TenSecond
 */
void deleteJobs()
{
    for(int i=0;i<MAX_JOBS;i++)
    {
        JOB * jobPtr = &list_jobs[i];
        
        // If the job is finished and 10 seconds have passed already then update the status
        if((jobPtr->status == JOB_FINISHED || jobPtr->status == JOB_ABORTED) && time(NULL) >= jobPtr->wait4TenSecond + 10)
        {
            jobPtr->jobPositionTaken = 0;
            jobPtr->status = JOB_DELETED;
            sf_job_deleted(i);
        }
    }
}

/**
 * A function that turns str into an integer between range 0 and 63 inclusive
 */
int string2Integer(char * str);