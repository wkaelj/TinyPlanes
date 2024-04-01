#include "gameloop.h"
#include "plane_types.h"
#include <SDL2/SDL_scancode.h>
#include <assert.h>
#include <stdlib.h>

#include <sys/time.h>
#include <stdio.h>
#include "render/input.h"

#include <messenger.h>
#include <unistd.h>

#include <utils.h>

// busted macro should never have been done
#define SKY_COLOUR 180, 190, 230, 255
static inline f64 get_delta(size_t fps_limit);
void render_debug_text(const Render *r, const RenderFont *font, const char *t);

// init
Result init_game(GameData *g);

void destroy_game(GameData *g);

// move the client plane based on input
Result update_client_plane(GameData *g, f32 delta);

// moves all the planes in the list received from the server
// so that they do not stutter as much.
Result update_server_planes(
    const Connection *connection, struct PlaneList *plane_list);

// attempts to retrieve the entity list from the server
// if it cannot it just leaves the planes at their predicted positions
Result retrieve_server_planes();

int game_update(GameData *game, f32 delta)
{
    SimplePlane client_plane = create_simple_plane(&game->client_plane);

    // make sure bullet lists match
    assert(
        memcmp(
            client_plane.active_bullets,
            &game->client_plane.active_bullets,
            sizeof(client_plane.active_bullets)) == 0);

    // read input and move client plane
    update_client_plane(game, delta);

    // inform server of movement
    connection_send_client_plane(
        &game->multiplayer.connection, game->multiplayer.id, &client_plane);

    update_server_planes(
        &game->multiplayer.connection, &game->multiplayer.plane_list);

    // draw
    render_set_colour(game->render, SKY_COLOUR);
    render_clear(game->render);

    draw_terrain(&game->plane_render, &client_plane);

    // draw planes
    struct PlaneNode *plane;
    LIST_FOREACH(plane, &game->multiplayer.plane_list, data)
    {
        // move client plane on draw list if not already to prevent visual
        // lag
        if (plane->player_id == game->multiplayer.id)
        {
            plane->p = client_plane; // update plane
        }

        // check if planes bullets hit
        for (size_t i = 0;
             plane->player_id != game->multiplayer.id && i < MAX_BULLET_COUNT;
             i++)
        {
            if (plane->p.active_bullets[i].used)
            {
                // TODO: make own function
                Position bullet_pos = plane->p.active_bullets[i].p;
                Position client_pos = client_plane.position;

                Position diff = {
                    .x = fabsf(bullet_pos.x - client_pos.x),
                    .y = fabsf(bullet_pos.y - client_pos.y),
                };

                f32 magnitude = sqrtf(diff.x * diff.x + diff.y * diff.y);

                if (magnitude < 16)
                {
                    log_info("Plane hit!");
                    return 1;
                }
            }
        }

        draw_plane(&game->plane_render, &client_plane, &plane->p);
    }

    // draw ui
    draw_ui(&game->plane_render, &game->client_plane);

    render_submit(game->render);
    return 0;
}

Result game_loop()
{
    GameData game = {0};
    if (init_game(&game) != RS_SUCCESS)
        return RS_FAILURE;
    bool running = true;
    input_init(game.render);

    input_start_text_input(game.render);
    while (running)
    {
        [[maybe_unused]] f64 delta = get_delta(60);

        RenderEvent e = render_poll_events(game.render);

        if (e == RENDER_EVENT_QUIT)
            break;

        switch (game.game_state)
        {
        case GAME_STATE_MAIN_MENU:
            // draw the main menu
            render_set_colour(game.render, SKY_COLOUR);
            render_clear(game.render);
            draw_menu_bg(&game.plane_render);
            render_button_draw(game.render, game.main_menu_buttons.connect);
            render_button_draw(game.render, game.main_menu_buttons.exit);
            render_submit(game.render);

            // draw list of planes to select
            for (size_t i = 0; i < PLANE_TYPE_MAX; i++)
            {
                break; // TODO
            }

            // or if button clicked, switch to respective state
            if (render_button_clicked(game.render, game.main_menu_buttons.exit))
                running = false;
            if (render_button_clicked(
                    game.render, game.main_menu_buttons.connect))
            {
                // get text input for server ip
                const char *txt = input_get_input_text(game.render);
                assert(array_length(game.multiplayer.server_ip) == 16);
                strncpy(
                    game.multiplayer.server_ip,
                    txt,
                    array_length(game.multiplayer.server_ip));

                log_info(
                    "Switching to game state CONNECTING\n, Input IP %s",
                    game.multiplayer.server_ip);

                input_stop_text_input(game.render);
                game.game_state = GAME_STATE_CONNECTING;

                render_clear(game.render); // no leftover menu items
            }
            break;
        case GAME_STATE_CONNECTING:
            // draw connecting animation
            render_clear(game.render);
            // attempt to connect to server ip
            // change state, or if failed retry set # of times
            for (size_t i = 0; i < 1; i++)
            {
                uid_t id = create_connection(
                    &game.multiplayer.connection, game.multiplayer.server_ip);
                if (id == 0)
                {
                    log_warning(
                        "Error connecting to server, attempt %zu", i + 1);
                    continue;
                }
                else
                {
                    game.multiplayer.id = id;
                    game.game_state     = GAME_STATE_IN_FLIGHT; // go to game
                    log_info("Connected to server, starting flight");
                    goto exit_connect; // exit switch statement (break only
                                       // leave for)
                }
            }
            // if id still 0
            log_error("Failed to connect to server");
            game.game_state =
                GAME_STATE_MAIN_MENU; // return to the main menu on failure
            input_start_text_input(game.render);
        exit_connect:
            break;
        case GAME_STATE_IN_FLIGHT:
            if (game_update(&game, delta) == 1)
            {
                close_connection(
                    &game.multiplayer.connection, game.multiplayer.id);
                game.game_state = GAME_STATE_MAIN_MENU;
            }
            break;
        case GAME_STATE_DIED:
            // return to main menu to replay
            game.game_state = GAME_STATE_MAIN_MENU;
            break;
        case GAME_STATE_IN_MENU:
            // somehow handle drawing menu over game
            break;
        default:
            log_fatal("Invalid game state");
            exit(1);
            break;
        }
    }

    // disconnect from server if connected
    if (game.multiplayer.id != 0)
    {
        close_connection(&game.multiplayer.connection, game.multiplayer.id);
    }

    destroy_game(&game);

    return RS_SUCCESS;
}

Result init_game(GameData *g)
{
    g->game_state = GAME_STATE_MAIN_MENU;

    // render engine
    if (!render_is_initialized())
        render_init();
    g->window = render_create_window("TinyPlane", 400, 300);
    if (g->window == NULL)
    {
        log_fatal("Failed to create window");
        render_quit();
        return RS_FAILURE;
    }
    g->render = render_create_render(g->window);
    if (g->render == NULL)
    {
        log_fatal("Failed to create render");
        render_destroy_window(g->window);
        render_quit();
        return RS_FAILURE;
    }

    // load fonts and textures
    // TODO: if font load fails use other successful font
    // TODO: when loading seperate font free it in close func
    g->fonts.menu =
        render_create_font(g->render, "fonts/Rajdhani-Regular.ttf", 128);
    if (g->fonts.menu == NULL)
        log_warning("Failed to load menu font");
    assert(g->fonts.menu);
    g->fonts.hud = g->fonts.menu;
    // menu buttons
    RenderColour b_background    = {100, 100, 110, 255};
    RenderColour b_foreground    = {130, 130, 140, 255};
    RenderRect connect_b_pos     = {400, 200, 200, 50};
    g->main_menu_buttons.connect = render_create_button(
        g->render,
        g->fonts.menu,
        "Connect",
        &connect_b_pos,
        b_background,
        b_foreground,
        5,
        30);

    RenderRect quit_b_pos     = {400, 400, 200, 50};
    g->main_menu_buttons.exit = render_create_button(
        g->render,
        g->fonts.menu,
        "Quit",
        &quit_b_pos,
        b_background,
        b_foreground,
        5,
        30);

    // plane render
    g->plane_render = create_plane_render(g->render, g->fonts.hud);

    // client plane
    g->client_plane = create_plane_type(PLANE_TYPE_FA18);
    // initialize plane list
    LIST_INIT(&g->multiplayer.plane_list);
    return RS_SUCCESS;
}

void destroy_game(GameData *game)
{
    // free plane list, as planes connected when client disconnects could leak
    struct PlaneNode *node;
    while (LIST_EMPTY(&game->multiplayer.plane_list) == false)
    {
        node = LIST_FIRST(&game->multiplayer.plane_list);
        LIST_REMOVE(node, data);
        free(node);
    }

    destroy_plane_render(&game->plane_render);
    // destroy menu buttons
    if (game->main_menu_buttons.connect != NULL)
        render_destroy_button(game->main_menu_buttons.connect);
    if (game->main_menu_buttons.exit != NULL)
        render_destroy_button(game->main_menu_buttons.exit);

    // unload fonts and textures
    if (game->fonts.menu != NULL)
        render_destroy_font(game->fonts.menu);
    // NOTE: should be removed, just because currently fonts are the same
    game->fonts.hud = NULL;
    if (game->fonts.hud != NULL)
        render_destroy_font(game->fonts.hud);
    render_destroy_render(game->render);
    render_destroy_window(game->window);
    render_quit();
}

//
// System functions
//
Result update_client_plane(GameData *game, f32 delta)
{
    if (input_is_key_pressed(game->render, SDL_SCANCODE_SPACE))
    {
        plane_fire_bullet(&game->client_plane);
    }
    // move client plane
    plane_update(&game->client_plane, delta);
    if (input_is_key_pressed(game->render, SDL_SCANCODE_LEFT))
        plane_turn(&game->client_plane, delta, LEFT, 1.f);
    else if (input_is_key_pressed(game->render, SDL_SCANCODE_RIGHT))
        plane_turn(&game->client_plane, delta, RIGHT, 1.f);

    // update throttle
    const f32 THROTTLE_INCREMENT = 0.01f;
    if (game->client_plane.throttle < 1.f &&
        input_is_key_pressed(game->render, SDL_SCANCODE_UP))
        game->client_plane.throttle += THROTTLE_INCREMENT;
    if (game->client_plane.throttle > 0.f &&
        input_is_key_pressed(game->render, SDL_SCANCODE_DOWN))
        game->client_plane.throttle -= THROTTLE_INCREMENT;
    // handle whatever funky float stuff happens and creates invalid throttle
    if (game->client_plane.throttle > 1.f)
        game->client_plane.throttle = 1.f;
    if (game->client_plane.throttle < 0.f)
        game->client_plane.throttle = 0.f;

    return RS_SUCCESS;
}

Result
update_server_planes(const Connection *connection, struct PlaneList *plane_list)
{
    ConnectionUpdate update = connection_pump_updates(connection);
    struct PlaneNode *node;
    while (update.type != CONNECTION_NO_UPDATE)
    {
        switch (update.type)
        {
        case CONNECTION_UPDATE_PLANE:
            // find the plane updated and assign data if it is newer
            LIST_FOREACH(node, plane_list, data)
            {
                if (node->player_id == update.plane_update.id)
                {
                    if (node->last_updated < update.plane_update.update_time)
                    {
                        node->p            = update.plane_update.plane;
                        node->last_updated = update.plane_update.update_time;
                    }
                    goto exit_update;
                }
            }
            // assume that the plane is new, hence not in the list, and add it
            node               = malloc(sizeof(struct PlaneNode));
            node->player_id    = update.plane_update.id;
            node->last_updated = update.plane_update.update_time;
            node->p            = update.plane_update.plane;
            LIST_INSERT_HEAD(plane_list, node, data);
        exit_update:
            break;
        case CONNECTION_UPDATE_DISCONNECT:
            // find plane that is disconencting and remove it from the draw list
            LIST_FOREACH(node, plane_list, data)
            {
                // check if id matches, remove node, stop checking
                if (node->player_id == update.disconnect_update.id)
                {
                    log_info(
                        "Disconnecting plane, id %i",
                        update.disconnect_update.id);
                    LIST_REMOVE(node, data);
                    free(node);
                    break; // NOTE: will this break out of the upper while loop?
                }
            }
        case CONNECTION_UPDATE_ERROR:
            log_warning("Connection update error");
            break;
        default:
            log_warning("Unexpected connection update type %i", update.type);
            break;
        }
        update = connection_pump_updates(connection);
    }

    return RS_SUCCESS;
}

void render_debug_text(const Render *r, const RenderFont *font, const char *t)
{
    RenderText *text     = render_create_text(r, font, t, 255, 255, 255);
    RenderRect text_rect = {
        .x = 10,
        .y = 60,
        .w = 800,
        .h = 50,
    };
    render_draw_text(r, text, &text_rect);
    render_destroy_text(text);
}

static inline f64 get_delta(size_t limit)
{
    // limit framerate to 60 fps
    [[maybe_unused]] const time_t FRAMETIME = SEC_TO_MICROSEC / limit;
    static f64 t1                           = 0;
    if (t1 == 0)
        t1 = get_time(); // avoid huge moves on startup

    time_t t2    = get_time();
    time_t delta = t2 - t1;
    // wait for time left
    if (delta < FRAMETIME)
    {
        useconds_t sleep_time = FRAMETIME - delta;
        if (sleep_time > FRAMETIME)
            log_warning("Sleeping for unsually long time, %us", sleep_time);
        if (usleep(sleep_time) != 0)
            log_error("Issue sleeping for %lis", FRAMETIME - delta);
    }
    t1 = t2;
    // return delta converted from microsecs to seconds
    return (float)delta / SEC_TO_MICROSEC;
}
