#include <semaphore.h>
#include <pthread.h>
#include "globals.h"
#include <string.h>
#include "debug.h"
#include <time.h>
#include <stdlib.h>

typedef struct client
{
    int connFd;
    USER * user;
    MAILBOX * mailbox;
    int loginState;     // 1 for logged in, 0 for logged out
    sem_t mutex;        // Mutex for protecting referenceCount, user, and mailbox, and login-state
    int referenceCount;
} CLIENT;

// Static mutex for client_login
static sem_t loginMutex;


CLIENT *client_create(CLIENT_REGISTRY *creg, int fd)
{
    CLIENT * output = (CLIENT *)malloc(sizeof(CLIENT));
    
    // malloc somehow went through return NULL
    if(output == NULL)
        return NULL;
    
    output->connFd = fd;
    output->loginState = 0;     // Logged out initially
    output->mailbox = NULL;
    output->user = NULL;
    output->referenceCount = 0;
    
    // Initialize the semaphore
    int ret = sem_init(&output->mutex, 0, 1);
    
    if(ret == -1)
    {
        debug("cannot initialize semaphore");
        free(output);
        return NULL;
    }
    
    // Reference it because it is newly created
    client_ref(output, "for newly created client");
    
    return output;
}

CLIENT *client_ref(CLIENT *client, char *why)
{
    // Lock the client to be only access by this thread
    sem_wait(&client->mutex); 
    
    client->referenceCount++;
    debug("Increase reference count on client %p (%d -> %d) for %s", client, client->referenceCount - 1, client->referenceCount, why);
    
    // Unlock the client to be modify by other threads
    sem_post(&client->mutex);
    
    return client;
}

void client_unref(CLIENT *client, char *why)
{
    sem_wait(&client->mutex);       // Lock the client
    
    client->referenceCount --;      // Decrease the reference count
    debug("Decrease reference count on client %p (%d -> %d) for %s", client, client->referenceCount + 1, client->referenceCount, why);
    
    if(client->referenceCount != 0)
    {
        sem_post(&client->mutex);   // Unlock client if the referenceCount is not 0 yet
    }
    else
    {
        // However if the referenceCount is 0 then we have to decrease reference
        // for the user and the mailbox that client references
        if(client->loginState == 1)
        {
            user_unref(client->user, "Unreferencing from client_unref should probably never occur");
            mb_unref(client->mailbox, "Unreferencing from client_unref should probably never occur");
        }
        
        debug("Freeing client %p", client);
        free(client);
    }
}

void init_login_mutex()
{
    sem_init(&loginMutex, 0, 1);
}

int client_login(CLIENT *client, char *handle)
{
    // Initialize the login mutex
    static pthread_once_t once = PTHREAD_ONCE_INIT;
    pthread_once(&once, init_login_mutex);
    
    
    // Lock this function
    sem_wait(&loginMutex);
    
    // Client is already logged
    if(client->loginState == 1)
    {
        sem_post(&loginMutex);
        return -1;
    }
    
    // We get the list of clients by calling creg_all_clients
    // CLIENT * loggedClients[] = creg_all_clients(client_registry);  Can't do this because creg_all_clients doesn't return a constant expression
    CLIENT ** loggedClients = creg_all_clients(client_registry);
    
    // Incorrect way
    // // Point to the first pointer
    // CLIENT * clientPtr = loggedClients;
    // int foundHandleInClient = 0;
    // // For loop that goes through all of the clients for checking
    // for(clientPtr; clientPtr; clientPtr++)
    // {
    //     debug("Searching through list of logged in clients");
    //     // handle is already registered under a client
    //     if(strcmp(handle, user_get_handle(clientPtr->user)) == 0)
    //     {
    //         debug("Handle already logged under a client");
    //         foundHandleInClient = 1;
    //         break;
    //         // sem_post(&loginMutex);
    //         // return -1;
    //     }
    // }
    
    // Point to the first pointer
    CLIENT ** clientPp = loggedClients;
    
    int foundHandleInClient = 0;
    for(; *clientPp; clientPp++)
    {
        // handle is already registered under a client
        if((*clientPp)->loginState == 1 && strcmp(handle, user_get_handle((*clientPp)->user)) == 0)
        {
            debug("Handle already logged under a client");
            foundHandleInClient = 1;
            break;
        }
    }
    
    // Finished using the list of clients
    // Need to unreference 
    clientPp = loggedClients;
    for(; *clientPp; clientPp++)
    {
        client_unref(*clientPp, "The creg_all_clients returned list are finished being used. Hence decrement");
    }
    free(loggedClients);            // And free the array
    
    if(foundHandleInClient)
    {
        // If we found a handle, we release the loginMutex and return -1
        sem_post(&loginMutex);
        return -1;
    }
    
    // Here = CLIENT not yet loggin, handle is not yet logged under a CLIENT
    sem_wait(&client->mutex);           // Lock the client object we are modifying
    
    USER * retUser = ureg_register(user_registry, handle);      // Register the user
    MAILBOX * retMailbox = mb_init(handle);                     // Register the mailbox
    
    client->user = retUser;
    client->mailbox = retMailbox;
    client->loginState = 1;
    
    sem_post(&client->mutex);           // Unlock the client object we are mofidying

    // Unlock this function
    sem_post(&loginMutex);    
    
    // Everything is fine if we got here and return 0
    return 0;
}

int client_logout(CLIENT *client)
{
    sem_wait(&client->mutex);           // Lock the client
    
    // Client is not logged in then return the marbale + return -1
    if(client->loginState != 1)
    {
        sem_post(&client->mutex);
        return -1;
    }
    
    // If the client is logged in, then log it out
    client->loginState = 0;
    
    // Unreference and delete the USER and mailbox  
    ureg_unregister(user_registry, user_get_handle(client->user));  
    // user_unref(client->user, "Logging out the client, hence unreference the USER");       // Unref the user
    mb_unref(client->mailbox, "Logging out the client, hence unreference the mailbox");      // Unref the mailbox
    mb_shutdown(client->mailbox);       // Shutdown the mailbox
    client->mailbox = NULL;
    client->user = NULL;
    
    sem_post(&client->mutex);           // Unlock the client
    
    return 0;
}

USER *client_get_user(CLIENT *client, int no_ref)
{
    sem_wait(&client->mutex);            // Lock the client
    
    // Not logged in
    if(client->loginState == 0)
    {
        sem_post(&client->mutex);
        return NULL;
    }
    
    // Don't increment the referenceCount on the return USER
    if(no_ref != 0)
    {
        sem_post(&client->mutex);       // Unlock the client again. No need to increment
        return client->user;
    }
    // Increment the referenceCount on the return USER
    // Caller accept the responsbility of decrementing the referenceCount
    else
    {
        user_ref(client->user, "client_get_user, caller will decrement the referenceCount");         // Increase the referenceCount on the user
        sem_post(&client->mutex);       // Unlock the client
        return client->user;
    }
}

MAILBOX *client_get_mailbox(CLIENT *client, int no_ref)
{
    sem_wait(&client->mutex);            // Lock the client
    
    // Not logged in 
    if(client->loginState == 0)
    {
        sem_post(&client->mutex);
        return NULL;
    }
    
    // Don't increment the referenceCount on the return MAILBOX
    if(no_ref != 0)
    {
        sem_post(&client->mutex);       // Unlock the client again. No need to increment
        return client->mailbox;
    }
    // Increment the referenceCount on the return MAILBOX
    // Caller accept the responsbility of decrementing the referenceCount
    else
    {
        mb_ref(client->mailbox, "client_get_mailbox, caller will decrement the referenceCount");         // Increase the referenceCount on the user
        sem_post(&client->mutex);       // Unlock the client
        return client->mailbox;
    }
}

int client_get_fd(CLIENT *client)
{
    return client->connFd;
}

int client_send_packet(CLIENT *user, CHLA_PACKET_HEADER *pkt, void *data)
{
    sem_wait(&user->mutex);         // Lock the CLIENT
    
    // Call proto_send_packet
    int ret = proto_send_packet(user->connFd, pkt, data);
    
    if(ret == -1)
    {
        sem_post(&user->mutex);     // Unlock the CLIENT
        return -1;   
    }
    
    sem_post(&user->mutex);         // Unlock the CLIENT
    return 0;                       // Everything went fine just return 0    
}

int client_send_ack(CLIENT *client, uint32_t msgid, void *data, size_t datalen)
{
    CHLA_PACKET_HEADER * packet = (CHLA_PACKET_HEADER *)malloc(sizeof(CHLA_PACKET_HEADER));
    packet->type = CHLA_ACK_PKT;     // Type no need to be converted
    packet->payload_length = htonl(datalen);
    
    // First we need to generate the system time
    struct timespec times;
    clock_gettime(CLOCK_REALTIME, &times);
    
    // Then put it into our packet 
    packet->timestamp_sec = htonl(times.tv_sec);
    packet->timestamp_nsec = htonl(times.tv_nsec);
    
    // Finally the msgid field
    packet->msgid = htonl(msgid);
    
    int ret = client_send_packet(client, packet, data);
    
    free(packet);
    
    if(ret == 0)
        return 0;
    else
        return -1;    
}

int client_send_nack(CLIENT *client, uint32_t msgid)
{
    CHLA_PACKET_HEADER * packet = (CHLA_PACKET_HEADER *)malloc(sizeof(CHLA_PACKET_HEADER));
    packet->type = CHLA_NACK_PKT;    // Type no need to be converted to network byte order
    
    // First we need to generate the system time
    struct timespec times;
    clock_gettime(CLOCK_REALTIME, &times);
    
    // Then put it into our packet 
    packet->timestamp_sec = htonl(times.tv_sec);
    packet->timestamp_nsec = htonl(times.tv_nsec);
        
    packet->msgid = htonl(msgid);
    
    // Nothing for payload
    packet->payload_length = 0;
    
    int ret = client_send_packet(client, packet, NULL);
    
    free(packet);
    
    if(ret == 0)
        return 0;
    else
        return -1;  
}


