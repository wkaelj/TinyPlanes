#include "gameloop.h"
#include "messenger.h"
#include "network.h"
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>

int main()
{
    Connection c;
    uid_t id = create_connection(&c, "127.0.0.1");
    assert(id != 0);

    log_info("Connected, id = %u", id);

    SimplePlane recv_plane = {};
    SimplePlane send_plane = {
        .plane_type = rand(),
        .heading    = rand(),
        .position   = {rand(), rand()},
        .velocity   = rand(),
    };

    connection_send_client_plane(&c, id, &send_plane);

    ConnectionUpdate update = {.type = CONNECTION_NO_UPDATE};
    while (update.type == CONNECTION_NO_UPDATE)
        update = connection_pump_updates(&c);

    sleep(2);

    close_connection(&c, id);
    log_info("Disconnecting");

    assert(update.type == CONNECTION_UPDATE_PLANE);
    recv_plane = update.plane_update.plane;

    assert(recv_plane.plane_type == send_plane.plane_type);
    assert(recv_plane.heading == send_plane.heading);
    assert(recv_plane.velocity == send_plane.velocity);

    log_info("Plane update passed");

    // game_loop();
    return 0;
}
