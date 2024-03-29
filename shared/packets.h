#pragma once
#include "plane.h"

#define MAX_PACKET_DATA 1024

typedef enum PacketType
{
    PACKET_TYPE_EMPTY = 0,
    PACKET_TYPE_CONNECITON,
    PACKET_TYPE_DISCONNECTION,
    PACKET_TYPE_DATA,
} PacketType;

typedef union Packet
{
    PacketType type;
    struct EmptyPacket
    {
        PacketType type;
        uid_t id;
    } empty_packet;
    struct ConnectionPacket
    {
        PacketType type;
        uid_t return_uid; // server send back uid for client
    } connection_packet;
    struct DisconnectPacket
    {
        PacketType type;
        uid_t id;
    } disconnect_packet;
    struct PlanePacket
    {
        PacketType type;
        uid_t id;
        SimplePlane plane;
    } data_packet;
} Packet;
