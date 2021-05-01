#include "user.h"
#include <semaphore.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include "debug.h"

typedef struct user_registry_node
{
    USER * user;
    char * handle;
    struct user_registry_node * next;
    struct user_registry_node * prev;
} USER_REGISTRY_NODE;

/**
 * USER_REGISTRY will have the node scheme of being circular with a dummy node.
 * The dummy node will have next point to itself and prev also point to itself
 */
typedef struct user_registry
{
    USER_REGISTRY_NODE head; // We will just keep this in user_registry, not pointing to anywhere else
    sem_t mutex;
} USER_REGISTRY;

USER_REGISTRY *ureg_init(void)
{
    // Initialize the first USER_REGISTRY struct that is responsible for starting the link
    // list of users
    USER_REGISTRY * output = (USER_REGISTRY *)malloc(sizeof(USER_REGISTRY));

    // Make the root point back to itself
    output->head.next = &output->head;
    output->head.prev = &output->head;
    output->head.handle = NULL;
    
    // Initialize the semaphore
    sem_init(&output->mutex, 0, 1);

    // Return the pointer to the data structure
    return output;
}

void ureg_fini(USER_REGISTRY *ureg)
{
    debug("Finalize user registry");
    USER_REGISTRY_NODE * nodePtr = ureg->head.next;
    
    while(nodePtr != &ureg->head)
    {
        // Call user_unref to decrease reference count
        user_unref(nodePtr->user, "Unreferenced because of calling ureg_fini");
        free(nodePtr->handle);      // Also need to free the copy of handle stored by NODE
        USER_REGISTRY_NODE * tmp = nodePtr->next;
        free(nodePtr);
        nodePtr = tmp;
    }
    
    // Then we finally free the dummy node. No copy of a handle so no need to free handle
    sem_destroy(&ureg->mutex);  // Destroy the mutex
    free(ureg);
}

USER *ureg_register(USER_REGISTRY *ureg, char *handle)
{
    // Lock the linked list to prevent threads stepping over each other
    sem_wait(&ureg->mutex);
    
    USER * output;
    
    // Now we have to search through the linked list to see if a
    // user of the handle already exist 
    USER_REGISTRY_NODE * nodePtr = ureg->head.next;
    
    int userFound = 0;
    
    while(nodePtr != &ureg->head)
    {
        if(strcmp(nodePtr->handle, handle) == 0)
        {
            // User found
            userFound = 1;
            break;            
        }
        nodePtr = nodePtr->next;
    }
    
    // userFound just have to increment the ref count
    if(userFound == 1)
    {
        user_ref(nodePtr->user, "User already in linked_list in ureg_register. Just need to increment ref count");
        output = nodePtr->user;
    }
    // userNotFound we have to create the user and insert it into our linked list
    else
    {
        USER * newUser = user_create(handle);
        user_ref(newUser, "User not in linked_list in ureg_register. Return with ref count of 2");
        
        // Insert into linked_list
        USER_REGISTRY_NODE * newNode = (USER_REGISTRY_NODE *)malloc(sizeof(USER_REGISTRY_NODE));
        newNode->user = newUser;
        
        // Crete a copy of the handle
        char * handleCpy = (char *)malloc(strlen(handle) + 1);
        strcpy(handleCpy, handle);
        
        newNode->handle = handleCpy;
        
        // Set the links
        USER_REGISTRY_NODE * dummyNode = &ureg->head;
        USER_REGISTRY_NODE * nextNode = dummyNode->next;
        
        dummyNode->next = newNode;
        nextNode->prev = newNode;
        
        newNode->prev = dummyNode;
        newNode->next = nextNode;
        
        output=newUser;
    }
    
    // Unlock the linked list
    sem_post(&ureg->mutex);
    
    return output;
}

void ureg_unregister(USER_REGISTRY *ureg, char *handle)
{
    // Lock the linked list
    sem_wait(&ureg->mutex);
    
    USER_REGISTRY_NODE * nodePtr = ureg->head.next;
    
    int userFound = 0;
    
    while(nodePtr != &ureg->head)
    {
        if(strcmp(nodePtr->handle, handle) == 0)
        {
            userFound = 1;
            break;
        }
    }
    
    // Only remove it if it is found in the linked list
    if(userFound)
    {
        // Decrease the reference count
        user_unref(nodePtr->user, "Unregistering a user in ureg_unregister");
        
        // Remove from the linked list
        USER_REGISTRY_NODE * prevNode = nodePtr->prev;
        USER_REGISTRY_NODE * nextNode = nodePtr->next;
        
        prevNode->next = nextNode;
        nextNode->prev = prevNode;
        
        // Free the copy of handle stored by the NODE
        free(nodePtr->handle);
        
        // Free the node
        free(nodePtr);
    }
    
    // Unlock the linked list
    sem_post(&ureg->mutex);
}