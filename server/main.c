#include "packets.h"
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <messenger.h>

#include <sys/queue.h>

struct Connection
{
    uid_t id;
    struct sockaddr client_addr;
    void *plane_data; // TODO

    LIST_ENTRY(Connection) data;
};

const short SERVER_PORT = 8080;

int main()
{
    // list of connected clients
    LIST_HEAD(ConnectionList, Connection) connection_list;
    LIST_INIT(&connection_list);

    // start listening for connections
    struct sockaddr_in server_addr = {
        .sin_family      = AF_INET,
        .sin_port        = htons(SERVER_PORT),
        .sin_addr.s_addr = inet_addr("127.0.0.1"),
    };
    int server_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (bind(
            server_socket,
            (struct sockaddr *)&server_addr,
            sizeof(server_addr)) == -1)
    {
        log_error("Failed to bind server socket");
        return 1;
    }
    // upon new connections assign id
    for (;;)
    {
        struct sockaddr client_addr;
        socklen_t client_addr_size = sizeof(client_addr);
        Packet p;
        if (recvfrom(
                server_socket,
                &p,
                sizeof(p),
                0,
                &client_addr,
                &client_addr_size) > 0)
        {
            log_warning("Error recieving packet");
            continue;
        }
        switch (p.type)
        {
        case PACKET_TYPE_EMPTY: break;
        case PACKET_TYPE_CONNECITON:
            // send back uid
            break;
        case PACKET_TYPE_DISCONNECTION:
            // send disconnect and id to all clients
        case PACKET_TYPE_DATA:
            // update all clients with plane info
            break;
        }
    }
    // listen for plane updates
    // send to all connected
}
