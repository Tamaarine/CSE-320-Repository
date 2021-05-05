#include "mailbox.h"
#include <semaphore.h>
#include <string.h>
#include "debug.h"

typedef struct mailbox_node
{
    MAILBOX_ENTRY * mailboxEntry;
    struct mailbox_node * next;
} MAILBOX_NODE;

typedef struct mailbox 
{
    char * handle;      // Private copy of the given handle
    MAILBOX_DISCARD_HOOK * hookFunc;        // Pointer to the discard_hook functions to be called
    MAILBOX_NODE * head;    // Head of the linked list queue. Should have a dummy node
    MAILBOX_NODE * tail;    // Tail of the linked list queue
    sem_t items;    // A semaphore for keeping track of number of items in the queue
    sem_t mutex;    // A mutex for locking the queue when doing operations    
    int referenceCount;             // A referenceCount for pointing to MAILBOX
    int defuncted;  // A boolean flag for whether or not this mailbox is defunct
} MAILBOX;

MAILBOX *mb_init(char *handle)
{
    MAILBOX * output = (MAILBOX *)malloc(sizeof(MAILBOX));
    
    if(output == NULL)
        return NULL;
    
    // No need for a dummy node, just set head and tail to both NULL
    // Let the enqueue and dequeue function handle the nodes
    output->head = NULL;
    output->tail = NULL;        
        
    output->hookFunc = NULL;            // No discard function initially
    output->referenceCount = 1;         // 1 referenceCount when being returned
    output->defuncted = 0;              // 0 for no initially defunct
    
    sem_init(&output->mutex, 0, 1);     // mutex is set to 1
    sem_init(&output->items, 0, 0);     // items sempahore is set to 0 for empty queue
    
    char * handleCpy = (char *)malloc(strlen(handle) + 1);      // Add 1 for the null terminator
    strcpy(handleCpy, handle);      // Copy the handle for a private copy
    output->handle = handleCpy;
    
    return output;
}

void mb_set_discard_hook(MAILBOX *mb, MAILBOX_DISCARD_HOOK * hookFunction)
{
    sem_wait(&mb->mutex);       // Lock the entire mailbox
    mb->hookFunc = hookFunction;
    sem_post(&mb->mutex);       // Unlock the entire mailbox
}

void mb_ref(MAILBOX *mb, char *why)
{
    sem_wait(&mb->mutex);       // Lock the mailbox
    
    mb->referenceCount++;       // Increment the referenceCount
    debug("Increase reference count on mailbox %p (%d -> %d) for %s", mb, mb->referenceCount - 1, mb->referenceCount, why);
    
    sem_post(&mb->mutex);       // Unlock the mailbox
}

void mb_unref(MAILBOX *mb, char *why)
{
    sem_wait(&mb->mutex);       // Lock the mailbox
    
    mb->referenceCount--;
    debug("Decrease reference count on mailbox %p (%d -> %d) for %s", mb, mb->referenceCount + 1, mb->referenceCount, why);

    // Don't free the mailbox, just need to sem_post the mutex that's it
    if(mb->referenceCount != 0)
    {
        sem_post(&mb->mutex);   // Unlock the mailbox
    }
    else
    {
        // Need to free everything for this mailbox because referenceCount is 0
        debug("Finalizing the mailbox since referenceCount is 0");
        
        // First have to clean up the linked list first
        MAILBOX_NODE * nodePtr = mb->head; 
        while(nodePtr != NULL)
        {
            // If the mailboxEntry is a MESSAGE, must free the body
            if(nodePtr->mailboxEntry->type == MESSAGE_ENTRY_TYPE)
            {
                mb->hookFunc(nodePtr->mailboxEntry);
                free(nodePtr->mailboxEntry->content.message.body);
            }
            free(nodePtr->mailboxEntry);    // Free the mailboxEntry that is in the node
            
            MAILBOX_NODE * tmp = nodePtr->next;
            free(nodePtr);          // Free the actual node
            nodePtr = tmp;          // Go on and free the next
        }
        
        free(mb->handle);       // Free the private copy of handle
        sem_destroy(&mb->items);     // Destroy the semaphore 
        sem_destroy(&mb->mutex);     // Destroy the mutex
        
        free(mb);               // Then just free the mailbox and we are done
        debug("Finished finalizing the mailbox");
    }
    
}

void mb_shutdown(MAILBOX *mb)
{
    sem_wait(&mb->mutex);       // Lock the mailbox for editing the fields
    mb->defuncted = 1;          // It has become defuncted
    debug("mailbox %p has become defunct", mb);
    sem_post(&mb->items);
    sem_post(&mb->mutex);       // Unlock the mailbox 
}

char *mb_get_handle(MAILBOX *mb)
{
    return mb->handle;
}

void mb_add_message(MAILBOX *mb, int msgid, MAILBOX *from, void *body, int length)
{
    sem_wait(&mb->mutex);        // Lock the mailbox for inserting new entry to the queue
    
    // Making the MESSAGE struct, just pass in what we are given
    MESSAGE messageToInsert;
    messageToInsert.msgid = msgid;
    messageToInsert.from = from;
    messageToInsert.body = body;   
    messageToInsert.length = length; 
    
    // Making the MAILBOX_ENTRY
    MAILBOX_ENTRY * mailEntry = (MAILBOX_ENTRY *)malloc(sizeof(MAILBOX_ENTRY));
    mailEntry->type = MESSAGE_ENTRY_TYPE;
    mailEntry->content.message = messageToInsert;
    
    // Create the new MAILBOX_NODE to insert after tail 
    MAILBOX_NODE * newNode = (MAILBOX_NODE *)malloc(sizeof(MAILBOX_NODE));
    newNode->mailboxEntry = mailEntry;
    newNode->next = NULL;       // We are inserting after the tail, so next is NULL
    
    // Empty MAILBOX, set head and tail both to newNode
    if(mb->head == NULL)
    {
        mb->head = newNode;
        mb->tail = newNode;
    }
    else
    {
        // Can just set tail's next to be newNode
        mb->tail->next = newNode;
        mb->tail = newNode;     // Update tail to be newNode
    }
        
    // Not from the same MAILBOX, hence must increment the referenceCount on from
    if(mb != from)
        mb_ref(from, "to ensure this mailbox persist so it can receive a notification in case message bounces");
    
    sem_post(&mb->items);       // Add an item and notify mb_next_entry to go on if it was blocked
    sem_post(&mb->mutex);       // Unlock the mailbox 
}

void mb_add_notice(MAILBOX *mb, NOTICE_TYPE ntype, int msgid)
{
    sem_wait(&mb->mutex);       // Lock the queue of MAILBOX_ENTRY
    
    // Making the NOTICE struct, just take what is passed in
    NOTICE toInsertNotice;
    toInsertNotice.type = ntype;
    toInsertNotice.msgid = msgid;
    
    // Malloc an MAILBOX_ENTRY
    MAILBOX_ENTRY * mailEntry = (MAILBOX_ENTRY *)malloc(sizeof(MAILBOX_ENTRY));
    mailEntry->type = NOTICE_ENTRY_TYPE;
    mailEntry->content.notice = toInsertNotice;
    
    // Malloc the newNode
    MAILBOX_NODE * newNode = (MAILBOX_NODE *)malloc(sizeof(MAILBOX_NODE));
    newNode->mailboxEntry = mailEntry;
    newNode->next = NULL;       // Insert at the end hence NULL
    
    // Empty MAILBOX, set head and tail both to newNode
    if(mb->head == NULL)
    {
        mb->head = newNode;
        mb->tail = newNode;
    }
    else
    {
        // Can just set tail's next to be newNode
        mb->tail->next = newNode;
        mb->tail = newNode;     // Update tail to be newNode
    }
    
    sem_post(&mb->items);       // Add an item and notify mb_next_entry to go on if it was blocked
    sem_post(&mb->mutex);       // Unlock the queue
}

MAILBOX_ENTRY *mb_next_entry(MAILBOX *mb)
{
    sem_wait(&mb->items);       // Block until there is something in the queue to return
    sem_wait(&mb->mutex);       // Lock the queue for dequeueing. Have to do it in this order or else deadlock may occur
    
    // If the mailbox is defuncted, just return NULL
    if(mb->defuncted == 1)
    {
        sem_post(&mb->mutex);   // Unlock and return NULL
        debug("Mailbox %p has become defuncted return NULL", mb);
        return NULL;
    }
    
    // If it isn't defuncted, then we will actually remove the entry
    MAILBOX_NODE * outputNode = mb->head;
    MAILBOX_ENTRY * output = outputNode->mailboxEntry;
    
    // Decrement the sender's reference count
    if(output->type == MESSAGE_ENTRY_TYPE && mb != output->content.message.from)
    {
        mb_unref(output->content.message.from, "received the message now, can unreference the mailbox from sender");
    }
    
    // There is only one item in the queue
    if(mb->head == mb->tail)
    {
        mb->head = NULL;
        mb->tail = NULL;
    }
    else
    {
        mb->head = mb->head->next;
    }
    
    // We must free the outputNode because we are retriving MAILBOX_ENTRY not the node
    // So we will never see this node again hence we must free
    free(outputNode);
    sem_post(&mb->mutex);       // Unlock the queue for enqueue
    return output;
}