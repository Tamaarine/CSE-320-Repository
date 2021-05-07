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

// void mailbox_discard_hook_function(MAILBOX_ENTRY * entry)
// {
//     entry->content.message.from
// }

void * chla_mailbox_service(void * arg)
{
    CLIENT * clientPtr = (CLIENT *)arg;
    MAILBOX * mbPtr = client_get_mailbox(clientPtr, 0);         // This thread will be resposnible for decrementing the reference
    MAILBOX_ENTRY * retEntry;
    
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
            
            client_send_packet(clientPtr, &packetToSent, retEntry->content.message.body);
            client_send_ack(clientPtr, retEntry->content.message.msgid, NULL, 0);
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
    }
    
    mb_unref(mbPtr, "finished using the mailbox in mailbox thread");
    
    // The MAILBOX is marked defunct, the mailbox thread should just termiante
    pthread_exit(NULL);
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
                // CLIENT ** clients = creg_all_clients(client_registry);
                // CLIENT ** clientPtr = clients;
                // for(; *clientPtr; clientPtr++)
                // {
                //     client_send_packet(theClient, )
                // }
                
                
                break;
            case CHLA_SEND_PKT:
                debug("SEND");
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