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

// The global job array which keeps track of all the jobs


/**
 * Print a string that states the required argument and the actual number of argument given
 */
void printRequiredArgs(int required, int given, char * command, FILE * out);