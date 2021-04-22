#include "protocol.h"
#include <unistd.h>
#include <stdio.h>
#include "debug.h"

int proto_send_packet(int fd, CHLA_PACKET_HEADER *hdr, void *payload)
{
    // Let's do this one first then, since it came first in the list
    // fd is the filedescriptor we are senting
    // hdr is the network byte order header of the packet we are sending
    // payload is the payload we are sending
    
    
    // No packet is given so return -1
    if(hdr == NULL)
    {
        return -1;
    }
    else
    {
        // This variable is used to prevent short-write
        int writtenByte = 0;
        
        while(writtenByte != sizeof(CHLA_PACKET_TYPE))
        {
            // convert to void * cuz we don't care the buffer type, and the amount we write is just size
            // of the struct 
            int ret = write(fd, (void *)hdr, sizeof(CHLA_PACKET_HEADER));
            
            if(ret == -1)
            {
                return -1;
            }
            
            debug("writing header %d", writtenByte);
            writtenByte += ret;
        }
    }
    
    // Now we have to write the payload which we also have to handle short write
    if(payload != NULL)
    {
        int writtenByte = 0;
        
        while(writtenByte != hdr->payload_length)
        {
            int ret = write(fd, payload, hdr->payload_length);
            
            if(ret == -1)
            {
                return -1;
            }
            
            debug("writing payload %d", writtenByte);
            writtenByte += ret;
        }
    }
    
    // If we are here then everything is fine just return 0
    return 0;
}

int proto_recv_packet(int fd, CHLA_PACKET_HEADER *hdr, void **payload)
{
    
    
    
    
    
}