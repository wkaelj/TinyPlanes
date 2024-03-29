#include "plane_render.h"

#include <SDL2/SDL_stdinc.h>
#include <assert.h>
#include <math.h>
#include <plane.h>
#include <stdio.h>
#include <sys/cdefs.h>
#include <types.h>

#include "messenger.h"
#include "plane_types.h"

static const size_t PLANE_SIZE_PX = 64;

const f32 CLIENT_DRAWN_HEADING = M_PI_2; // client plane is drawn pointing up

/*
 * HELPERS
 */

Position convert_point_relative(
    Position input, Position client_plane, f32 client_heading);
f32 convert_rotation_relative(f32 input, f32 client_heading);

// calculate the section of the plane texture atlas to use for each plane
RenderRect find_plane_texture_rect(PlaneType plane_type);

// convert world coordinates to screen coordinates
Position pos_to_screen(const Render *r, Position p);

RenderRect convert_ui_coordinate(i32, i32, const Position *p);

// update text to a new object
NONULL(1, 2, 3, 4)
Result update_text(
    const Render *render,
    const RenderFont *font,
    RenderText **text,
    const char *new_text,
    RenderColour colour);
/*
 * PUBLIC FN
 */

PlaneRender create_plane_render(const Render *render, RenderFont *ui_font)
{
    assert(render);
    // TODO error checking
    PlaneRender p = {
        .render = render, // epic bug prone code
        .plane_textures =
            render_create_texture(p.render, "textures/plane_atlas.png"),
        .terrain_texture =
            render_create_texture(render, "textures/terrain_forest.png"),
        .bullet_texture = render_create_texture(render, "textures/bullet.png"),
        .ui_atlas = render_create_texture(render, "textures/ui_atlas.png"),
        .main_menu_texture =
            render_create_texture(render, "textures/main_menu.png"),
        .ui_font           = ui_font,
        .ui_bullet_display = NULL,
        .ui_speed_display  = NULL,
        .ui_bullet_number  = 0,
        .ui_speed_number   = 0,
    };

    if (p.plane_textures == NULL)
        log_error("Failed to load plane textures");
    if (p.terrain_texture == NULL)
        log_error("Failed to load terrain textures");
    if (p.bullet_texture == NULL)
        log_error("Failed to load bullet texture");
    if (p.ui_atlas == NULL)
        log_error("Failed to load ui textures");
    if (p.main_menu_texture == NULL)
        log_error("Failed to load menu texture");

    return p;
}

void destroy_plane_render(PlaneRender *render)
{
    if (render->ui_bullet_display)
        render_destroy_text(render->ui_bullet_display);
    if (render->ui_speed_display)
        render_destroy_text(render->ui_speed_display);
    if (render->plane_textures)
        render_destroy_texture(render->plane_textures);
    if (render->bullet_texture)
        render_destroy_texture(render->bullet_texture);
    if (render->terrain_texture)
        render_destroy_texture(render->terrain_texture);
    if (render->ui_atlas)
        render_destroy_texture(render->ui_atlas);
    if (render->main_menu_texture)
        render_destroy_texture(render->main_menu_texture);
}

void draw_bullet(
    const PlaneRender *render, const Bullet *bullet, const SimplePlane *client)
{
    assert(bullet->used);

    Position drawn_position =
        convert_point_relative(bullet->p, client->position, client->heading);
    drawn_position = pos_to_screen(render->render, drawn_position);
    f32 rotation = convert_rotation_relative(bullet->heading, client->heading);
    rotation -= CLIENT_DRAWN_HEADING;

    RenderRect bullet_rect =
        calculate_rect_centered(drawn_position.x, drawn_position.y, 8, 16);
    render_draw_texture(
        render->render, &bullet_rect, NULL, render->bullet_texture, rotation);
}

Result draw_plane(
    const PlaneRender *r, const SimplePlane *client, const SimplePlane *drawn)
{
    // get correct plane texture
    RenderRect plane_texture_rect = find_plane_texture_rect(drawn->plane_type);

    // convert plane position to screen coordinates
    Position drawn_position = convert_point_relative(
        drawn->position, client->position, client->heading);
    drawn_position = pos_to_screen(r->render, drawn_position);

    // find texture rotation, so heading is correct when rotates relative to
    // client
    f32 rotation = convert_rotation_relative(drawn->heading, client->heading);

    // apply rotation to account for textures being diagonal
    rotation -= M_PI_4;

    RenderRect plane_rect = calculate_rect_centered(
        drawn_position.x, drawn_position.y, PLANE_SIZE_PX, PLANE_SIZE_PX);

    // draw bullets
    for (size_t i = 0; i < MAX_BULLET_COUNT; i++)
    {
        if (client->active_bullets[i].used)
            draw_bullet(r, &client->active_bullets[i], client);
    }

    // draw texture
    assert(r->plane_textures != NULL);
    if (render_draw_texture(
            r->render,
            &plane_rect,
            &plane_texture_rect,
            r->plane_textures,
            rotation) != RENDER_SUCCESS)
        return RS_FAILURE;

    return RS_SUCCESS;
}

Result draw_terrain(const PlaneRender *r, const SimplePlane *client)
{
    assert(r->terrain_texture);
    // always draw terrain centered about 0, 0
    Position terrain_pos = convert_point_relative(
        POSITION(0, 0), client->position, client->heading);
    terrain_pos = pos_to_screen(r->render, terrain_pos);

    RenderRect out_rect = calculate_rect_centered(
        terrain_pos.x, terrain_pos.y, 1080 * 2, 1080 * 2);

    assert(r->terrain_texture != NULL);

    if (render_draw_texture(
            r->render,
            &out_rect,
            NULL,
            r->terrain_texture,
            convert_rotation_relative(0.f, client->heading)))
    {
        log_error("Failed to draw terrain");
        return RS_FAILURE;
    }

    return RS_SUCCESS;
}

void draw_throttle(
    const Render *render,
    i32 w,
    i32 h,
    f32 throttle_factor,
    const RenderTexture *ui_atlas)
{
    if (throttle_factor < 0.f)
        log_debug("Invalid throttle factor %f", throttle_factor);
    assert(throttle_factor <= 1.f && throttle_factor >= 0.f);
    // draw slider

    // slider position
    const f32 throttle_top_pos = 0.5f;
    const f32 throttle_x_pos   = 0.8f;

    const Position throttle_coordinate =
        POSITION(throttle_x_pos, throttle_top_pos);
    RenderRect throttle_out_rect =
        convert_ui_coordinate(w, h, &throttle_coordinate);

    throttle_out_rect.w = h / 16;
    throttle_out_rect.h = h / 2 + 20;

    RenderRect throttle_slider_src = {0, 0, 7, 37};
    render_draw_texture(
        render, &throttle_out_rect, &throttle_slider_src, ui_atlas, 0.f);

    // draw slider

    f32 handle_y = (throttle_top_pos - 1) + throttle_factor;

    const Position handle_position = POSITION(throttle_x_pos, handle_y);
    RenderRect handle_out_rect = convert_ui_coordinate(w, h, &handle_position);
    handle_out_rect.x -= w / 32;
    handle_out_rect.w = h / 8;
    handle_out_rect.h = h / 12;

    RenderRect throttle_handle_src = {8, 0, 10, 7};
    render_draw_texture(
        render, &handle_out_rect, &throttle_handle_src, ui_atlas, 0.f);
}

Result draw_ui(PlaneRender *render, const Plane *client)
{
    assert(render && render->ui_atlas);
    assert(client);
    // ui coordinates are a grid from -1 to 1, (-1, -1) is bottom left
    // (1, 1) is top right

    i32 w, h;
    render_get_render_size(render->render, &w, &h);

    // draw throttle
    draw_throttle(render->render, w, h, client->throttle, render->ui_atlas);

    const u8 ICON_SIZE    = 32; // size of each icon, used to make rows
    const u8 ICON_PADDING = 8;

    Position icon_list_position = POSITION(-0.9, 0.8);
    RenderRect icon_rect = convert_ui_coordinate(w, h, &icon_list_position);
    icon_rect.w          = ICON_SIZE;
    icon_rect.h          = ICON_SIZE;

    RenderRect display_rect = {
        .x = icon_rect.x + ICON_SIZE + ICON_PADDING,
        .y = icon_rect.y,
        .w = 50,
        .h = ICON_SIZE,
    };

    // draw bullet count
    RenderRect bullet_rect = {8, 8, 14, 10};
    render_draw_texture(
        render->render, &icon_rect, &bullet_rect, render->ui_atlas, 0.f);
    // update the bullet count if necessary
    if (render->ui_bullet_number != client->bullets_remaining)
    {

        char buf[5];
        SDL_itoa(client->bullets_remaining, buf, 10);
        render->ui_bullet_number = client->bullets_remaining;
        update_text(
            render->render,
            render->ui_font,
            &render->ui_bullet_display,
            buf,
            (RenderColour){255, 255, 255});
    }
    render_draw_text(render->render, render->ui_bullet_display, &display_rect);

    // draw speed
    RenderRect speed_rect = {8, 20, 13, 13};
    icon_rect.y += ICON_SIZE + ICON_PADDING;
    render_draw_texture(
        render->render, &icon_rect, &speed_rect, render->ui_atlas, 0.f);
    if (render->ui_speed_number != client->speed)
    {

        char buf[5];
        SDL_itoa(client->speed, buf, 10);
        render->ui_bullet_number = client->bullets_remaining;
        update_text(
            render->render,
            render->ui_font,
            &render->ui_speed_display,
            buf,
            (RenderColour){255, 255, 255});
    }
    display_rect.y += ICON_SIZE + ICON_PADDING;
    render_draw_text(render->render, render->ui_speed_display, &display_rect);

    return RS_SUCCESS;
}

Result draw_menu_bg(const PlaneRender *render)
{
    i32 w, h;
    i32 tw, th;
    render_get_render_size(render->render, &w, &h);
    render_get_texture_size(render->main_menu_texture, &tw, &th);

    RenderRect rect = {0, 0, w, th / (float)((float)tw / w)};
    render_draw_texture(
        render->render, &rect, NULL, render->main_menu_texture, 0);
    return RS_SUCCESS;
}

/*
 * HELPER IMPLEMENTATION
 */

f32 convert_rotation_relative(f32 input, f32 client_heading)
{
    return input - client_heading + CLIENT_DRAWN_HEADING;
}

Position convert_point_relative(
    Position input, Position client_plane, f32 client_heading)
{
    // client_heading += CLIENT_DRAWN_HEADING;
    // subtract client position from input position
    f32 local_x = input.x - client_plane.x;
    f32 local_y = input.y - client_plane.y;

    return (Position){
        .x = local_x * cosf(client_heading) - local_y * sinf(client_heading),
        .y = local_x * sinf(client_heading) + local_y * cosf(client_heading)};
}

RenderRect find_plane_texture_rect(PlaneType plane_type)
{
    // TODO: make PLANE_SIZE = texture height
    const size_t PLANE_SIZE = 32; // the scale of the plane textures
    RenderRect output_rect  = {.x = 0, .y = 0, PLANE_SIZE, PLANE_SIZE};
    switch (plane_type)
    {
    case PLANE_TYPE_F14: output_rect.x = PLANE_SIZE * 0; break;
    case PLANE_TYPE_FA18: output_rect.x = PLANE_SIZE * 1; break;
    case PLANE_TYPE_F16: output_rect.x = PLANE_SIZE * 2; break;
    case PLANE_TYPE_F4: output_rect.x = PLANE_SIZE * 3; break;
    case PLANE_TYPE_F22: output_rect.x = PLANE_SIZE * 4; break;
    case PLANE_TYPE_F35: output_rect.x = PLANE_SIZE * 5; break;
    case PLANE_TYPE_MAX: log_warning("Plane type not valid"); break;
    }

    return output_rect;
}

Position pos_to_screen(const Render *restrict r, Position p)
{
    int x, y;
    render_get_render_size(r, &x, &y);
    return POSITION(p.x + (float)x / 2, y - (p.y + (float)y / 2));
}

RenderRect convert_ui_coordinate(i32 window_w, i32 window_h, const Position *p)
{
    i32 x = window_w * p->x / 2;
    i32 y = window_h * p->y / 2;

    x = x + window_w / 2;
    y = window_h - (y + window_h / 2);

    return (RenderRect){x, y, 0, 0};
}

Result update_text(
    const Render *render,
    const RenderFont *font,
    RenderText **text,
    const char *new_text,
    RenderColour colour)
{
    if (*text)
    {
        render_destroy_text(*text);
    }

    *text = render_create_text(
        render, font, new_text, colour.r, colour.g, colour.b);
    if (*text)
        return RS_SUCCESS;
    else
        return RS_FAILURE;
}
