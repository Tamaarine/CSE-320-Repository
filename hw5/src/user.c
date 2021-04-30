#include <semaphore.h>
#include <stddef.h>
#include "debug.h"
#include <string.h>
#include <stdlib.h>

typedef struct user
{
    char * handle;          // Handle for the user
    sem_t mutex;            // Binary semaphore/mutex for protecting the reference count
    int referenceCount;     // Variable that counts the number of time this USER struct is pointed
} USER;

USER *user_create(char *handle)
{
    USER * output = (USER *)malloc(sizeof(USER)); // Malloc a USER struct on the heap
    
    int semRet = sem_init(&(output->mutex), 0, 1);   // Initialize it to 1 for semaphore
    if(semRet == -1)
    {
        debug("cannot initialize semaphore");
        free(output);
        return NULL;
    }
    
    // Initialize the referenceCount to 1
    output->referenceCount = 1;
    
    // Then for the handle we also need to malloc it on the heap because it is a private copy
    // strlen(handle) + 1 for null terminator since strlen doesn't count null terminator
    char * handleHeap = (char *)malloc(strlen(handle) + 1);   
    
    // Then we will strcpy the handle for a private copy
    strcpy(handleHeap, handle); 
    
    // Set the handle pointer to the copy that is in the heap and we are done
    output->handle = handleHeap;
    
    // Return the pointer to the heap object    
    return output;
}

USER *user_ref(USER *user, char *why)
{
    sem_wait(&user->mutex);     // Lock it
    user->referenceCount ++;    // Increase the count
    debug("why in user_ref %s", why); // Print the reason why it is incremented
    sem_post(&user->mutex);     // Unlock it
    return user;
}

void user_unref(USER *user, char *why)
{
    sem_wait(&user->mutex);     // Lock it
    user->referenceCount --;    // Decrease the count
    debug("why in user_unref %s", why);
    if(user->referenceCount != 0)
        sem_post(&user->mutex); // Unlock it if the referenceCount is not 0
    else
    {
        // However if it is 0 then we will have to free the user and the handle
        free(user->handle);     // Free the handle first
        free(user);             // Then free user
    }
}

char *user_get_handle(USER *user)
{
    return user->handle;
}