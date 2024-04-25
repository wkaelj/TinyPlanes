#include "packets.h"
#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
const short MAX_CLIENTS = 256;

int main();
void print_nonvoid_bullets(struct Bullet *bullets);

void crash_handler(int)
{
    // NOTE: this function causes massive memory leaks
    log_error("Auto restarting server after crash");
    main();
}

int main()
{
    signal(SIGSEGV, crash_handler);

    // list of connected clients
    LIST_HEAD(ConnectionList, Connection) connection_list;
    LIST_INIT(&connection_list);

    size_t client_count = 0;

    // start listening for connections
    struct sockaddr_in server_addr = {
        .sin_family      = AF_INET,
        .sin_port        = htons(SERVER_PORT),
        .sin_addr.s_addr = INADDR_ANY,
    };
    int server_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    assert(server_socket != -1);
    // TODO error check
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
        Packet recieved_packet;
        if (recvfrom(
                server_socket,
                &recieved_packet,
                sizeof(recieved_packet),
                0,
                &client_addr,
                &client_addr_size) < 0)
        {
            log_warning("Error recieving packet");
            continue;
        }
        struct Connection *c; // store the connection node when relevant
        switch (recieved_packet.type)
        {
        case PACKET_TYPE_EMPTY:
            // send same packet back
            sendto(
                server_socket,
                &recieved_packet,
                sizeof(recieved_packet),
                0,
                &client_addr,
                client_addr_size);
            break;
        case PACKET_TYPE_CONNECITON:
            if (client_count >= MAX_CLIENTS)
            {
                log_warning("Connection denied, too many players");
                break;
            }
            log_info("New connection");
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
                // add new client node
                struct Connection *new_node = malloc(sizeof(struct Connection));
                assert(new_node);
                *new_node = (struct Connection){
                    .client_addr     = client_addr,
                    .client_addr_len = client_addr_size,
                    .id              = cpack.return_uid,
                };
                LIST_INSERT_HEAD(&connection_list, new_node, data);
                client_count++;
            }
            break;
        case PACKET_TYPE_DISCONNECTION:
            log_info("A client has disconnected");
            // send disconnect and id to all clients
            // and delete the node when encountered
            struct Connection *removed = NULL;
            LIST_FOREACH(c, &connection_list, data)
            {
                if (c->id == recieved_packet.disconnect_packet.id)
                {
                    removed = c; // set so it can be removed after loop
                }
                sendto(
                    server_socket,
                    &recieved_packet,
                    sizeof(recieved_packet),
                    0,
                    &c->client_addr,
                    c->client_addr_len);
            }
            LIST_REMOVE(removed, data);
            if (removed)
                free(removed);
            client_count--;
            break;
        case PACKET_TYPE_PLANE:
            // update all clients with plane info
            LIST_FOREACH(c, &connection_list, data)
            {
                // debug bulet list not empty
                if (sendto(
                        server_socket,
                        &recieved_packet,
                        sizeof(recieved_packet),
                        0,
                        &c->client_addr,
                        c->client_addr_len) == -1)
                    log_error("Local error sending plane packet");
            }
            break;
        }
    }
}

void print_nonvoid_bullets(struct Bullet *bullets)
{
    char str[MAX_BULLET_COUNT + 1]; // space for null char
    memset(str, '\0', sizeof(str));
    bool used = false;
    for (size_t i = 0; i < MAX_BULLET_COUNT; i++)
    {
        if (bullets[i].used)
        {
            used   = true;
            str[i] = 'X';
        }
        else
        {
            str[i] = '0';
        }
    }
    if (!used)
        return; // don't print empty list prevent spam
    else
    {
        printf("Bullets array: %s\n", str);
    }
}
