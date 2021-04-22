#include "protocol.h"
#include <unistd.h>
#include <stdio.h>
#include "debug.h"
#include <stdlib.h>

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
        
        while(writtenByte != sizeof(CHLA_PACKET_HEADER))
        {
            // convert to void * cuz we don't care the buffer type, and the amount we write is just size
            // of the struct 
            int ret = write(fd, (void *)hdr, sizeof(CHLA_PACKET_HEADER));
            
            if(ret == -1)
            {
                return -1;
            }
            
            writtenByte += ret;
            debug("writing header %d", writtenByte);
        }
    }
    
    // Now we have to write the payload which we also have to handle short write
    if(payload != NULL)
    {
        int writtenByte = 0;
        
        // Need to convert the length to host byte order from network byte order
        uint32_t hostLength = ntohl(hdr->payload_length);
        
        while(writtenByte != hostLength)
        {
            int ret = write(fd, payload, hostLength);
            
            if(ret == -1)
            {
                return -1;
            }
            
            writtenByte += ret;
            debug("writing payload %d", writtenByte);
        }
    }
    
    // If we are here then everything is fine just return 0
    return 0;
}

int proto_recv_packet(int fd, CHLA_PACKET_HEADER *hdr, void **payload)
{
    // First we will just read data into the hdr
    int readByte = 0;
    
    // Since we don't want t overwrite the original header if there is any error
    // we will use a tmp packet header struct
    CHLA_PACKET_HEADER tmp;
    
    // We will keep reading until we have finished reading everything about the 
    while(readByte != sizeof(CHLA_PACKET_HEADER))
    {
        // We only want to read the header part and not read into the payload area
        // since we will be reading it into the heap not the variable hdr.
        // The subtraction is there so we don't read into the payload area in case of short read
        int ret = read(fd, (void *)(&tmp), sizeof(CHLA_PACKET_HEADER) - readByte);
        
        if(ret == -1)
        {
            return -1;
        }
        
        readByte += ret;
        debug("reading header %d", readByte);
    }
    
    // After reading the header we will have to read into the payload area
    // which we will dynamically allocate using malloc
    uint32_t hostLength = ntohl(tmp.payload_length);
    
    // hostLength is not 0 that means there are payload that need to be read
    if(hostLength != 0)
    {
        readByte = 0;
        
        // Allocate space on the heap to store the payload
        char * heapPayload = (char *)malloc(hostLength);
        
        while(readByte != hostLength)
        {
            // Read into the heap
            int ret = read(fd, (void *)heapPayload, hostLength);
            
            if(ret == -1)
            {
                return -1;
            }
            
            readByte += ret;
            debug("reading payload %d", readByte);
        }
        
        // If we are here then everything is alright we can set the struct and the length
        *(hdr) = tmp;
        *(payload) = heapPayload;    
    }
    else
    {
        // No payload we just have to assign the packet header struct
        *(hdr) = tmp;
    }
    
    // Then we can just return since everything is successful
    return 0;
}