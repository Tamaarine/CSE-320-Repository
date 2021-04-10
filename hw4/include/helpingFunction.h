#include "imprimer.h"

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
} JOB;

// The global job array which keeps track of all the jobs, gotta implement it like a queue i suppose
JOB list_jobs[MAX_JOBS];

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
void freeAllPrinters();

/**
 * Function which is called to free all of the jobs in the list
 */
void freeAllJobs();
