#include "server.h"
#include "stddef.h"
#include "protocol.h"
#include <stdlib.h>
#include <stdio.h>
#include "debug.h"
#include "globals.h"
#include <pthread.h>

void chla_mailbox_service()
{
    
}

void *chla_client_service(void *arg)
{
    int connFd = *((int *)arg);     // Convert into an int * pointer and dereference it
    free(arg);      // Then free it after we retrive the file descriptor
    
    CHLA_PACKET_HEADER packet;      // To be used in recv_packet
    void * payloadPtr = NULL;       // To be used in recv_packet to store the payload location
    
    // Register the client once they are connected
    CLIENT * theClient = creg_register(client_registry, connFd);
    
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
                requestRet = client_login(theClient, (char *)payloadPtr);
                if(requestRet == 0)
                    client_send_ack(theClient, packet.msgid, NULL, 0);
                else
                    client_send_nack(theClient, packet.msgid);
                free(payloadPtr);
                break;
            case CHLA_LOGOUT_PKT:
                debug("LOGOUT");
                requestRet = client_logout(theClient);
                if(requestRet == 0)
                    client_send_ack(theClient, packet.msgid, NULL, 0);
                else
                    client_send_nack(theClient, packet.msgid);
                free(payloadPtr);
                break;
            case CHLA_USERS_PKT:
                debug("USERS");
                CLIENT ** clients = creg_all_clients(client_registry);
                CLIENT ** clientPtr = clients;
                for(; *clientPtr; clientPtr++)
                {
                    client_send_packet(theClient, )
                }
                
                
                break;
            case CHLA_SEND_PKT:
                debug("Send");
                break;
        }
        
    }
    
    
    
    
    
    
    return NULL;
}