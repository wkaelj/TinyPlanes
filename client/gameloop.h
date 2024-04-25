#include "chunk_loader.h"
#include "network.h"
#include "plane_render.h"
#include "render/render.h"
#include "types.h"
#include <sys/types.h>
#include <sys/queue.h>

#include <plane.h>

// multiplayer plane list
struct PlaneNode
{
    uid_t player_id;
    SimplePlane p;
    time_t last_updated;

    LIST_ENTRY(PlaneNode) data;
};

typedef enum Gamestate
{
    GAME_STATE_MAIN_MENU = 0,
    GAME_STATE_CONNECTING,
    GAME_STATE_IN_FLIGHT,
    GAME_STATE_DIED,
    GAME_STATE_IN_MENU,
} GameState;

typedef struct GameData
{
    GameState game_state;

    Plane client_plane;

    RenderWindow *window;
    Render *render;

    ChunkList chunk_list;

    struct
    {
        RenderButton *exit;
        RenderButton *connect;
    } main_menu_buttons;

    struct
    {
        RenderFont *menu;
        RenderFont *hud;
    } fonts;

    struct
    {
        char server_ip[17];
        Connection connection;
        uid_t id; // this client's id, recieved from server
        size_t player_count;
        int seed; // world seed
        LIST_HEAD(PlaneList, PlaneNode) plane_list;
    } multiplayer;

    PlaneRender plane_render;
} GameData;

Result game_loop();
