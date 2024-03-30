#include "network.h"
#include <arpa/inet.h>
#include <asm-generic/errno.h>
#include <assert.h>
#include <errno.h>
#include <unistd.h>
#include <messenger.h>
#include <utils.h>

uid_t create_connection(Connection *c, const char *ip)
{
    int client_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    struct sockaddr_in server_addr = {
        .sin_family      = AF_INET,
        .sin_port        = htons(SERVER_PORT),
        .sin_addr.s_addr = inet_addr(ip),
    };

    // send to server and reused to store response
    struct ConnectionPacket packet = {
        .type       = PACKET_TYPE_CONNECITON,
        .return_uid = 0,
    };

    if (sendto(
            client_socket,
            &packet,
            sizeof(packet),
            0,
            (struct sockaddr *)&server_addr,
            sizeof(server_addr)) == -1)
    {
        log_error("Failed to connect to server, ip %s", ip);
        close(client_socket);
        return 0;
    }

    // recieve response, should contain a uid
    struct sockaddr addr;
    socklen_t addr_len;
    if (recvfrom(client_socket, &packet, sizeof(packet), 0, &addr, &addr_len) ==
        -1)
    {
        log_error("Failed to recieve consfirmation of connection");
        close(client_socket);
        return 0;
    }

    assert(packet.type == PACKET_TYPE_CONNECITON);

    c->server_addr     = server_addr;
    c->server_addr_len = sizeof(server_addr);
    c->client_socket   = client_socket;

    assert(packet.return_uid != 0);

    // return assigned id or 0 for failure
    return packet.return_uid;
}

Result close_connection(Connection *c, uid_t id)
{

    struct DisconnectPacket packet = {
        .type = PACKET_TYPE_DISCONNECTION,
        .id   = id,
    };

    if (sendto(
            c->client_socket,
            &packet,
            sizeof(packet),
            0,
            (struct sockaddr *)&c->server_addr,
            c->server_addr_len) == -1)
        return RS_FAILURE;

    close(c->client_socket);
    return RS_SUCCESS;
}

Result connection_send_client_plane(
    const Connection *c, uid_t id, const SimplePlane *p)
{
    struct PlanePacket packet = {
        .type        = PACKET_TYPE_PLANE,
        .id          = id,
        .update_time = get_time(),
        .plane       = *p,
    };

    if (sendto(
            c->client_socket,
            &packet,
            sizeof(packet),
            0,
            (struct sockaddr *)&c->server_addr,
            c->server_addr_len) == -1)
    {
        log_warning("Client side error sending plane packet");
        return RS_FAILURE;
    }

    return RS_SUCCESS;
}

ConnectionUpdate connection_pump_updates(const Connection *c)
{
    Packet inc_packet;
    int err = recvfrom(
        c->client_socket,
        &inc_packet,
        sizeof(inc_packet),
        MSG_DONTWAIT,
        NULL,
        NULL);
    if (err == -1)
    {
        // if error is indicating no packets, ignore, otherwise report error
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return (ConnectionUpdate){.type = CONNECTION_NO_UPDATE};
        else
        {
            log_error("Error checking for incoming packets");
            return (ConnectionUpdate){.type = CONNECTION_UPDATE_ERROR};
        }
    }

    ConnectionUpdate update;
    switch (inc_packet.type)
    {
    case PACKET_TYPE_EMPTY:
        // ignore empty packets
        // NOTE: maybe not a good idea
        return (ConnectionUpdate){.type = CONNECTION_NO_UPDATE};
    case PACKET_TYPE_CONNECITON:
        // ignore, probably not meant to recieve now
        log_warning("Recieved connection packet unexpectedly");
        return (ConnectionUpdate){.type = CONNECTION_NO_UPDATE};
    case PACKET_TYPE_DISCONNECTION:
        // note disconnected
        update.disconnect_update.type = CONNECTION_UPDATE_DISCONNECT;
        update.disconnect_update.id   = inc_packet.disconnect_packet.id;
        return update;
    case PACKET_TYPE_PLANE:
        // fill out and return plane update
        update.plane_update.type  = CONNECTION_UPDATE_PLANE;
        update.plane_update.id    = inc_packet.data_packet.id;
        update.plane_update.plane = inc_packet.data_packet.plane;
        return update;
    default:
        log_error("Invalid packet type");
        return (ConnectionUpdate){.type = CONNECTION_UPDATE_ERROR};
    }
}
