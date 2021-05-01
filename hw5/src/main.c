#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <signal.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "debug.h"
#include "server.h"
#include "globals.h"

#include <unistd.h>
#include <netdb.h>
#include <fcntl.h>

static void terminate(int);

// My own functions
int strToInteger(char * strNum);
int open_listenfd(char * port);

// SIGHUP Handler
void sigup_handler(int sig, siginfo_t *si, void *unused)
{
    printf("sigup is sent\n");
    terminate(EXIT_SUCCESS);
}

/*
 * "Charla" chat server.
 *
 * Usage: charla <port>
 */
int main(int argc, char* argv[]){
    // Option processing should be performed here.
    // Option '-p <port>' is required in order to specify the port number
    // on which the server should listen.
    int opt;
    char * portNumber;
    
    while(1)
    {
        opt = getopt(argc, argv, "p:");
        switch(opt)
        {
            case 'p':
                portNumber = optarg;
                debug("Port number is pulled %s", portNumber);
                break;
        }
        
        // Break out of the while loop if everything is done parsing
        if(opt == -1)
        {
            break;
        }
    }
    
    debug("port is -%s-", portNumber);
    // Perform required initializations of the client_registry and
    // player_registry.
    user_registry = ureg_init();
    client_registry = creg_init();

    // TODO: Set up the server socket and enter a loop to accept connections
    // on this socket.  For each connection, a thread should be started to
    // run function charla_client_service().  
    int listenfd = open_listenfd(portNumber);
    
    // In addition, you should install
    // a SIGHUP handler, so that receipt of SIGHUP will perform a clean
    // shutdown of the server.
    
    // Struct that is used for sigaction
    struct sigaction sa;
    
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = &sigup_handler;
    sigemptyset(&sa.sa_mask);
    
    // Setting up the SIGHUP handler
    sigaction(SIGHUP, &sa, NULL);
    
    pthread_t thread_id;
    
    // // For use to test send_packet
    // int fd = open("test_output/a.txt", O_WRONLY);
    // CHLA_PACKET_HEADER test;
    // test.type = CHLA_LOGIN_PKT;
    // test.payload_length = 999;
    // test.msgid = 0;
    // test.timestamp_nsec = 0;
    // test.timestamp_sec = 0;
    // proto_send_packet(fd, &test, NULL);
    
    
    // Then we will enter a loop which accepts connections
    while(1)
    {
        // Since we are going to be passing this to the thread, we need to malloc it
        int * mallocConnFd = (int *)malloc(sizeof(int));
        
        // We don't care about the address of the connected clients
        *(mallocConnFd) = accept(listenfd, NULL, NULL); 
        
        // Client connection failed, free and move on
        if(*(mallocConnFd) == -1)
        {
            debug("connection failed with client");
            free(mallocConnFd);
        }
        // Connection success we will spawn a new thread that will call 
        else
        {
            debug("accepted a connection");
            pthread_create(&thread_id, NULL, chla_client_service, mallocConnFd);
        }
    }
}

/*
 * Function called to cleanly shut down the server.
 */
static void terminate(int status) {
    // Shut down all existing client connections.
    // This will trigger the eventual termination of service threads.
    creg_shutdown_all(client_registry);

    // Finalize modules.
    creg_fini(client_registry);
    ureg_fini(user_registry);

    debug("%ld: Server terminating", pthread_self());
    exit(status);
}

/**
 * Returns 10^times
 */
int multiplier(int times)
{
    int output = 1;
    for(int i=0;i<times;i++)
    {
        output *= 10;
    }
    return output;
}

/**
 * This function will try to convert a given String into an integer,
 * if the given String have letters then it will just return -1
 */
int strToInteger(char * strNum)
{
    int sum = 0;
    char * ptr = strNum;
    int length = strlen(strNum);
    
    for(; *ptr; ptr++)
    {
        if(*(ptr) < '0' || *(ptr) > '9')
        {
            return -1;
        }
        // Here means that it is a digit
        else
        {
            sum += multiplier(length -1) * (*(ptr) - '0');
            length --;
        }
    }
    return sum;
}

/**
 * On succuess it will return a listening socket on the specified port
 * It is reentrant and portocol-independent. But for the purpose of this homework
 * I don't really think it needs to be portocol-independent, but oh well
 */
int open_listenfd(char * port)
{
    // Structs that we need for doing this function
    struct addrinfo hints, *listp, *p;
    int listenfd, rc, optval = 1;
    
    // We zero out the struct hints first
    memset(&hints, 0, sizeof(struct addrinfo)); 
    hints.ai_socktype = SOCK_STREAM;            // Will accept any connection
    hints.ai_flags = AI_PASSIVE | AI_ADDRCONFIG;// On any IP address
    hints.ai_flags |= AI_NUMERICSERV;           // Use port number
    if((rc = getaddrinfo(NULL, port, &hints, &listp)) != 0)
    {
        debug("getaddrinfo failed (port %s): %s\n", port, gai_strerror(rc));
        return -2;
    }
    
    // If the DNS resolved was successful then we will walk the list to see one that we can bind to
    for(p=listp;p;p=p->ai_next)
    {
        if((listenfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0)
        {
            continue; // This socket failed we will try the next one
        }
        
        // Removes the address already in use error from bind
        setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval, sizeof(int));
        
        // Then we bind the descriptor to the address
        if(bind(listenfd, p->ai_addr, p->ai_addrlen) == 0)
            break; // It is successful
        if(close(listenfd) < 0)
        {
            debug("open_listenfd close failed: %s\n", strerror(errno));
            return -1;
        }
    }
    
    // Then we do some clean ups
    freeaddrinfo(listp);
    if(!p)
    {
        return -1;
    }
    
    // Make a listening socket passive to be ready to accept connection requests
    if(listen(listenfd, 1024) < 0)
    {
        close(listenfd);
        return -1;
    }
    return listenfd;
}
