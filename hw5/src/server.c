#include "server.h"
#include "stddef.h"
#include "protocol.h"
#include <stdlib.h>
#include <stdio.h>
#include "debug.h"
#include "globals.h"
#include <pthread.h>
#include <time.h>
#include <string.h>

void mailbox_discard_hook_function(MAILBOX_ENTRY * entry)
{
    mb_add_notice(entry->content.message.from, BOUNCE_NOTICE_TYPE, 0);
}

void * chla_mailbox_service(void * arg)
{
    CLIENT * clientPtr = client_ref((CLIENT *)arg, "receiived client pointer in mailbox");
    MAILBOX * mbPtr = client_get_mailbox(clientPtr, 0);         // This thread will be resposnible for decrementing the reference
    MAILBOX_ENTRY * retEntry;
    
    if(mbPtr == NULL)
    {
        client_unref(clientPtr, "finished using the pointer in mailbox thread");
        pthread_exit(NULL);
    }
    
    mb_set_discard_hook(mbPtr, mailbox_discard_hook_function);
    while((retEntry = mb_next_entry(mbPtr)) != NULL)
    {
        CHLA_PACKET_HEADER packetToSent;
        
        if(retEntry->type == MESSAGE_ENTRY_TYPE)
        {
            packetToSent.type = CHLA_MESG_PKT;
            packetToSent.msgid = htonl(retEntry->content.message.msgid);
            packetToSent.payload_length = htonl(retEntry->content.message.length);
            
            // Also need to set the time fields
            struct timespec times;
            clock_gettime(CLOCK_REALTIME, &times);
            
            packetToSent.timestamp_sec = htonl(times.tv_sec);
            packetToSent.timestamp_nsec = htonl(times.tv_nsec);
            
            int ret = client_send_packet(clientPtr, &packetToSent, retEntry->content.message.body);
            if(ret == 0)
            {
                // Recevied
                mb_add_notice(retEntry->content.message.from, RRCPT_NOTICE_TYPE, times.tv_sec);
            }
            else
            {
                // Bounced
                mb_add_notice(retEntry->content.message.from, BOUNCE_NOTICE_TYPE, times.tv_sec);
            }
            
        }
        else
        {
            // Depend on the NOTICE_TYPE we set the packet type
            switch(retEntry->content.notice.type)
            {
                case BOUNCE_NOTICE_TYPE:
                    packetToSent.type = CHLA_BOUNCE_PKT;
                    break;
                case RRCPT_NOTICE_TYPE:
                    packetToSent.type = CHLA_RCVD_PKT;
                    break;
                default:
                    debug("bug if reached here");
                    break; // Should never reach here
            }
            
            packetToSent.msgid = htonl(retEntry->content.notice.msgid);
            packetToSent.payload_length = 0;
            
            // Also need to set the time fields
            struct timespec times;
            clock_gettime(CLOCK_REALTIME, &times);
            
            packetToSent.timestamp_sec = htonl(times.tv_sec);
            packetToSent.timestamp_nsec = htonl(times.tv_nsec);
            
            client_send_packet(clientPtr, &packetToSent, NULL);
            client_send_ack(clientPtr, retEntry->content.message.msgid, NULL, 0);
        }
        free(retEntry);         // Free the MAILBOX_ENTRY
    }
    
    client_unref(clientPtr, "finished using the pointer in mailbox thread");
    mb_unref(mbPtr, "finished using the mailbox in mailbox thread");
    
    // The MAILBOX is marked defunct, the mailbox thread should just termiante
    pthread_exit(NULL);
}

/**
 * Return the index of where char character is found in the given String
 */
int indexOf(char * str, char character)
{
    for(int i=0;*(str + i);i++)
    {
        char currentChar = str[i];
        
        if(currentChar == character)
            return i;
    }
    return -1;
}

void *chla_client_service(void *arg)
{
    int connFd = *((int *)arg);     // Convert into an int * pointer and dereference it
    free(arg);      // Then free it after we retrive the file descriptor
    
    CHLA_PACKET_HEADER packet;      // To be used in recv_packet
    void * payloadPtr = NULL;       // To be used in recv_packet to store the payload location
    
    // Register the client once they are connected
    CLIENT * theClient = creg_register(client_registry, connFd);
    pthread_t mailbox_thread_id = -1;
    
    // Service loop that will keep on receving request and carries it out
    while(1)
    {
        int packetRet = proto_recv_packet(connFd, &packet, &(payloadPtr));
        if(packetRet == -1)
        {
            break;
        }
        
        switch(packet.type)
        {
            int requestRet;
            // If it is a login request then we will log the user in
            case CHLA_LOGIN_PKT:
                debug("LOGIN");
                
                char * handle = (char *)malloc(ntohl(packet.payload_length) - 1);
                memcpy(handle, payloadPtr, ntohl(packet.payload_length) - 1);
                handle[ntohl(packet.payload_length) - 2] = 0;
                
                requestRet = client_login(theClient, handle);
                free(handle);
                
                if(requestRet == 0)
                {
                    // We also have to spawn the mailbox service thread
                    pthread_create(&mailbox_thread_id, NULL, chla_mailbox_service, theClient);
                    debug("client thread passed %p", theClient);
                    client_send_ack(theClient, ntohl(packet.msgid), NULL, 0);
                }
                else
                    client_send_nack(theClient, ntohl(packet.msgid));
                    
                if(payloadPtr != NULL)
                {
                    free(payloadPtr);
                    payloadPtr = NULL;
                }
                break;
            case CHLA_LOGOUT_PKT:
                debug("LOGOUT");
                requestRet = client_logout(theClient);
                if(requestRet == 0)
                    client_send_ack(theClient, ntohl(packet.msgid), NULL, 0);
                else
                    client_send_nack(theClient, ntohl(packet.msgid));
                    
                if(payloadPtr != NULL)
                {
                    free(payloadPtr);
                    payloadPtr = NULL;
                }
                break;
            case CHLA_USERS_PKT:
                debug("USERS");
                CLIENT ** clients = creg_all_clients(client_registry);
                CLIENT ** clientPtr = clients;
                
                // Output and outputLength will be gathering the user handle strings 
                int outputLength = 1;
                char * output = malloc(1);      // Initially it only contains a null terminator
                *(output) = '\0';
                
                for(; *clientPtr; clientPtr++)
                {
                    USER * userPtr = client_get_user(*clientPtr, 1); // No need to be referenceCounted
                    
                    // The client is actually logged in hence we can get the handle
                    if(userPtr != NULL)
                    {
                        char * handle = user_get_handle(userPtr);
                        outputLength += strlen(handle) + 2;         // Add to outputLength the length of handle plus 2 for \r\n
                        output = realloc(output, outputLength);
                        strcat(output, handle);     // Copy the handle to the output
                        strcat(output, "\r\n");     // Concatnate the \r\n and also the null terminator
                    }
                    client_unref(*clientPtr, "The creg_all_clients returned list are finished being used. Hence decrement");
                }
                free(clients);  // Free the array after we are finished using it
                
                // There are actually users logged in
                if(outputLength != 1)
                {
                    client_send_ack(theClient, packet.msgid, (void *)output, outputLength - 1);
                }
                // No users logged in
                else
                {
                    client_send_ack(theClient, packet.msgid, NULL, 0);
                }
                free(output);
                break;
            case CHLA_SEND_PKT:
                debug("SEND");
                
                // We check if the current client is logged in if not NACK back
                USER * userPtr = client_get_user(theClient, 1);
                
                if(userPtr != NULL)
                {
                    int recipientLength = indexOf((char *)payloadPtr, '\n');    // Find the index of \n in payload
                    
                    // Allocate recipientLength number of bytes, it will include null terminator
                    char * recipientHandle = (char *)malloc(recipientLength);
                    memcpy(recipientHandle, payloadPtr, recipientLength -1);   // Copy all of the handle characters
                    recipientHandle[recipientLength -1] = '\0';                // Null terminate it
                    
                    // Now get the list of clients see if the client is logged in under the recipientHandle
                    CLIENT ** clients = creg_all_clients(client_registry);
                    CLIENT ** clientPtr = clients;
                    CLIENT * recipientClient = NULL;
                    
                    for(; *clientPtr; clientPtr++)
                    {
                        USER * userPtr = client_get_user(*clientPtr, 1); // No need to be referenceCounted
                        
                        // If we have indeed found the same USER under a client
                        if(userPtr != NULL && strcmp(user_get_handle(userPtr), recipientHandle) == 0)
                        {
                            client_ref(*clientPtr, "for sending a message to the client");
                            recipientClient = *clientPtr;
                        }
                        client_unref(*clientPtr, "The creg_all_clients returned list are finished being used. Hence decrement");
                    }
                    free(clients);      // Finished with the array free it
                    free(recipientHandle);  // Finished checking with the recipient handle can be freed
                    
                    if(recipientClient != NULL)
                    {
                        USER * userPtr = client_get_user(theClient, 1); // No need to be referenceCounted
                        char * handle = user_get_handle(userPtr);
                        
                        // Get us to the actual message and find the length of the message first
                        char * actualPayloadPtr = payloadPtr + recipientLength + 1;
                        int messageLength = ntohl(packet.payload_length) - (recipientLength + 1); // Get us the length of the message
                        
                        // Malloc it on the heap, add in the handle, add in the \r\n
                        char * output = (char *)calloc(1, strlen(handle) + 3 + messageLength);
                        strcpy(output, handle);
                        strcat(output, "\r\n");
                        
                        // Then for the message we have to offset to the null terminator and memcpy
                        char * offsetOutput = output + (strlen(handle) + 2);
                        memcpy(offsetOutput, actualPayloadPtr, messageLength);
                        
                        CHLA_PACKET_HEADER * packetToSent = (CHLA_PACKET_HEADER *)calloc(1, sizeof(CHLA_PACKET_HEADER));
                        packetToSent->payload_length = htonl(strlen(handle) + 3 + messageLength);
                        packetToSent->type = CHLA_MESG_PKT;
                        packetToSent->msgid = htonl(packet.msgid);
                        
                        struct timespec times;
                        clock_gettime(CLOCK_REALTIME, &times);
                        
                        packetToSent->timestamp_sec = htonl(times.tv_sec);
                        packetToSent->timestamp_nsec = htonl(times.tv_nsec);
                                                
                        client_unref(recipientClient, "finished sending the pakcet can be decremented");
                        client_send_packet(recipientClient, packetToSent, output);      
                        client_send_ack(theClient, packet.msgid, NULL, 0);
                        free(packetToSent);
                        free(output);
                    }
                    else
                    {
                        // Recipient not found in the list of clients, NACK it
                        client_send_nack(theClient, packet.msgid);
                    }
                }
                else
                {
                    // user not logged in hence we will NACK it
                    client_send_nack(theClient, packet.msgid);
                }
                if(payloadPtr != NULL)
                {
                    free(payloadPtr);
                    payloadPtr = NULL;
                }
                break;
        }
        
    }
    client_logout(theClient);
    if(mailbox_thread_id != -1)
    {
        debug("Waiting for the mailbox thread to terminate");
        pthread_join(mailbox_thread_id, NULL);
    }
    client_unref(theClient, "for finished using it in the client_thread");
    creg_unregister(client_registry, theClient);
    debug("thread is joined");
    
    pthread_exit(NULL);
}