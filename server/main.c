#include "packets.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <messenger.h>

#include <sys/queue.h>

// generate a uid for new clients
uid_t gen_uid(void)
{
    static uid_t id = 99;
    return id++;
}

struct Connection
{
    uid_t id;
    struct sockaddr client_addr;
    socklen_t client_addr_len;

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
        struct Connection *c; // store the connection node when relevant
        switch (p.type)
        {
        case PACKET_TYPE_EMPTY:
            // send same packet back
            sendto(
                server_socket,
                &p,
                sizeof(p),
                0,
                &client_addr,
                client_addr_size);
            break;
        case PACKET_TYPE_CONNECITON:
            // send back uid
            {
                struct ConnectionPacket cpack = {
                    .return_uid = gen_uid(),
                    .type       = PACKET_TYPE_CONNECITON,
                };
                sendto(
                    server_socket,
                    &cpack,
                    sizeof(cpack),
                    0,
                    &client_addr,
                    client_addr_size);
                // add node
                struct Connection *new_node = malloc(sizeof(struct Connection));
                *new_node                   = (struct Connection){
                                      .client_addr     = client_addr,
                                      .client_addr_len = client_addr_size,
                                      .id              = cpack.return_uid,
                };
                LIST_INSERT_HEAD(&connection_list, new_node, data);
            }
            break;
        case PACKET_TYPE_DISCONNECTION:
            // send disconnect and id to all clients
            // and delete the node when encountered
            LIST_FOREACH(c, &connection_list, data)
            {
                if (c->id != p.disconnect_packet.id)
                {
                    // resend incoming packet to all connected, and remove them
                    sendto(
                        server_socket,
                        &p,
                        sizeof(p),
                        0,
                        &c->client_addr,
                        c->client_addr_len);
                }
                else
                {
                    LIST_REMOVE(c, data);
                }
            }

        case PACKET_TYPE_PLANE:
            // update all clients with plane info
            LIST_FOREACH(c, &connection_list, data)
            {
                if (sendto(
                        server_socket,
                        &p,
                        sizeof(p),
                        0,
                        &c->client_addr,
                        c->client_addr_len) == -1)
                    log_error("Local error sending plane packet");
            }
            break;
        }
    }
}