#include <semaphore.h>
#include "client_registry.h"
#include <pthread.h>
#include "debug.h"

typedef struct client_registry 
{
    CLIENT ** clients;      // Use an array to store list of clients
    sem_t mutex;            // For protecting the access to the array of clients 
} CLIENT_REGISTRY;

static sem_t shutdownMutex;
static sem_t shutdownSemaphore;

CLIENT_REGISTRY *creg_init()
{
    CLIENT_REGISTRY * output = (CLIENT_REGISTRY *)malloc(sizeof(CLIENT_REGISTRY));

    if(output == NULL)
        return NULL;
    
    // Malloc 64 CLIENT pointers
    CLIENT ** mallocClients = (CLIENT **)malloc(MAX_CLIENTS * sizeof(CLIENT *));
    
    if(mallocClients == NULL)
        return NULL;
    
    // Initialize the array to all be NULL poitners
    for(int i=0;i<MAX_CLIENTS;i++)
    {
        mallocClients[i] = NULL;
    }
    
    output->clients = mallocClients;        // Set the client lists
    sem_init(&output->mutex, 0, 1);         // Initialize the semaphore
    
    return output;
}

void creg_fini(CLIENT_REGISTRY *cr)
{
    debug("Finalize client registry");
    // Unreference all the clients first
    for(int i=0;i<MAX_CLIENTS;i++)
    {
        CLIENT * clientPtr = cr->clients[i];
        
        if(clientPtr != NULL)
        {
            client_unref(clientPtr, "Client_registry is finished with the clients, hence unreference");
        }
    }
    
    free(cr->clients);      // Free the internal array
    sem_destroy(&cr->mutex);    // Destroy the mutex
    free(cr);               // Free the client_registry struct
}

CLIENT *creg_register(CLIENT_REGISTRY *cr, int fd)
{
    sem_wait(&cr->mutex);       // Lock the CLIENT_REGISTRY array
    
    int insertIndex = -1;
    
    // Go through the array to find a spot for inserting the CLIENT object
    // If cannot find it then NULL
    for(int i=0;i<MAX_CLIENTS;i++)
    {
        CLIENT * clientPtr = cr->clients[i];
        if(clientPtr == NULL)
        {
            insertIndex = i;
            break;
        }
    }
    
    if(insertIndex == -1)
    {
        sem_post(&cr->mutex);       // Unlock the CLIENT_REGISTRY array
        return NULL;          // Did not find an empty space in the CLIENT_REGISTRY
    }
    else
    {
        // We were able to find a spot to insert the CLIENT
        CLIENT * clientPtr = client_create(cr, fd);
        
        // Cannot create a new CLIENT object
        if(clientPtr == NULL)
        {
            sem_post(&cr->mutex);       // Unlock the CLIENT_REGISTRY array
            return NULL;
        }
        else
        {
            // Need to increment the referenceCount cuz we are returning one to the caller
            client_ref(clientPtr, "Pointer being returned by creg_register()");
            cr->clients[insertIndex] = clientPtr;       // Insert it into the CLIENT_REGISTRY array
            sem_post(&cr->mutex);       // Unlock the CLIENT_REGISTRY array
            return clientPtr;
        }
    }
}

int creg_unregister(CLIENT_REGISTRY *cr, CLIENT *client)
{
    sem_wait(&cr->mutex);       // Lock the array
    
    for(int i=0;i<MAX_CLIENTS;i++)
    {
        CLIENT * clientPtr = cr->clients[i];
        
        // Same CLIENT pointer means that we have found the CLIENT in the
        // registry hence we will decrement referenceCount
        if(clientPtr == client)
        {
            client_unref(client, "Unregistering the client from the client_registry");
            cr->clients[i] = NULL;      // Then we NULL out the entry in the array
            sem_post(&cr->mutex);   // Unlock the array
            sem_post(&shutdownSemaphore);   // Put back the marble
            return 0;               // 0 for success operation
        }
    }
    
    // If somehow we are out here then that means we never find the CLIENT in the registry
    // Hence return -1
    sem_post(&cr->mutex);       // Unlock the array
    return -1;
}

CLIENT **creg_all_clients(CLIENT_REGISTRY *cr)
{
    sem_wait(&cr->mutex);        // Lock the array from further modfication
    
    int clientCount = 0;        // Used for counting how many clients are currently connected
    
    for(int i=0;i<MAX_CLIENTS;i++)
    {
        CLIENT * clientPtr = cr->clients[i];
        if(clientPtr != NULL)
            clientCount++;        
    }
    
    // Malloc clientCount amount of pointers plus 1 for null terminate pointers
    CLIENT ** output = (CLIENT **)malloc((clientCount + 1) * sizeof(CLIENT *));
    
    int outputCounter = 0;
    
    for(int i=0;i<MAX_CLIENTS;i++)
    {
        CLIENT * clientPtr = cr->clients[i];
        if(clientPtr != NULL)
        {
            output[outputCounter] = clientPtr;
            client_ref(clientPtr, "for reference being added to clients list");
            outputCounter++;
        }
    }
    
    // Last entry is NULL
    output[outputCounter] = NULL;
    
    sem_post(&cr->mutex);       // Unlock the array
    return output;
}

void init_shutdown_mutex()
{
    sem_init(&shutdownMutex, 0, 1);
}

void creg_shutdown_all(CLIENT_REGISTRY *cr)
{
    static pthread_once_t once = PTHREAD_ONCE_INIT;
    pthread_once(&once, init_shutdown_mutex);
    
    sem_wait(&shutdownMutex);       // Lock the function, so only one can be called
    
    // Count the number of connected CLIENTS
    int clientCount = 0;
    for(int i=0;i<MAX_CLIENTS;i++)
    {
        CLIENT * clientPtr = cr->clients[i];
        if(clientPtr != NULL)
            clientCount++;
    }
    
    sem_init(&shutdownSemaphore, 0, clientCount);
    
    for(int i=0;i<MAX_CLIENTS;i++)
    {
        CLIENT * clientPtr = cr->clients[i];
        
        if(clientPtr != NULL)
        {
            debug("Shutting down client %d", client_get_fd(clientPtr));
            sem_wait(&shutdownSemaphore);   // Take the marble
            shutdown(client_get_fd(clientPtr), SHUT_RDWR);     // Call shutdown on the socket connection
        }
    }    
    
    while(clientCount != 0)
    {
        sem_wait(&shutdownSemaphore);
        clientCount--;
    }
    // debug("finished you fuck");
    
    sem_post(&shutdownMutex);
}
