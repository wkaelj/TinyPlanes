#pragma once

/*
 * This header defines the network functions used
 * to communicate with the game server. There are functions
 * to create and close connections, and to send and recieve data on those
 * connections
 */

#include "types.h"

#include <sys/socket.h>

#include <packets.h>

#define SERVER_PORT 8080

typedef struct Connection
{
    int socket;
} Connection;

Result create_connection(Connection *c, const char *ip);
Result close_connection(Connection *);

Result recieve_data();
