#include "client.h"

typedef struct client_registry 
{
    
} CLIENT_REGISTRY;

CLIENT_REGISTRY *creg_init()
{
    
}

void creg_fini(CLIENT_REGISTRY *cr)
{
    
}

CLIENT *creg_register(CLIENT_REGISTRY *cr, int fd)
{
    
}

int creg_unregister(CLIENT_REGISTRY *cr, CLIENT *client)
{
    
}

CLIENT **creg_all_clients(CLIENT_REGISTRY *cr)
{
    
}

void creg_shutdown_all(CLIENT_REGISTRY *cr)
{
    
}
