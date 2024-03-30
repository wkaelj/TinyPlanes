#pragma once

/*
 * This header defines the network functions used
 * to communicate with the game server. There are functions
 * to create and close connections, and to send and recieve data on those
 * connections
 */

#include "types.h"

#include <netinet/in.h>
#include <sys/socket.h>

#include <packets.h>

#define SERVER_PORT 8080

typedef struct Connection
{
    int client_socket;
    struct sockaddr_in server_addr;
    socklen_t server_addr_len;
} Connection;

typedef enum ConnectionUpdateType
{
    CONNECTION_NO_UPDATE,
    CONNECTION_UPDATE_PLANE,
    CONNECTION_UPDATE_DISCONNECT,
    CONNECTION_UPDATE_ERROR,
} ConnectionUpdateType;

typedef union ConnectionUpdate
{
    ConnectionUpdateType type;
    struct
    {
        ConnectionUpdateType type;
        uid_t id;
    } disconnect_update;
    struct
    {
        ConnectionUpdateType type;
        uid_t id;
        SimplePlane plane;
    } plane_update;
} ConnectionUpdate;

uid_t create_connection(Connection *c, const char *ip);

// must be retried if fails, otherwise socket will leak
Result close_connection(Connection *, uid_t);

// a successful result is no garuntee that the packet reached the server,
// only that it was sent properly
Result connection_send_client_plane(
    const Connection *c, uid_t id, const SimplePlane *p);

// check if packets are in queue, if so read them and report the data
// should be called until there are no incoming packets
ConnectionUpdate connection_pump_updates(const Connection *c);
