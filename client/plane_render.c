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
static const f32 PLANE_SIZE_PX = 0.2;

const f32 CLIENT_DRAWN_HEADING = M_PI_2; // client plane is drawn pointing up

/*
 * HELPERS
 */

void apply_client_transform(
    vec2 client_position, f32 client_rotation, vec2 position, vec2 dest);
f32 convert_rotation_relative(f32 input, f32 client_heading);

// calculate the section of the plane texture atlas to use for each plane
RenderRect find_plane_texture_rect(PlaneType plane_type);

// convert world coordinates to screen coordinates
void pos_to_screen(ivec2 screen_size, vec2 p, vec2 pos);

// convert a dimension in world coordinates to screen coordinates
void dimension_to_screen(ivec2 iscreen_size, vec2 dimension, vec2 dest);

RenderRect convert_ui_coordinate(i32, i32, const vec2 p);

Result draw_texture_relative(
    const Render *render,
    const RenderTexture *texture,
    const RenderRect *texture_src,
    f32 heading, // must already be relative to client
    vec2 size,
    vec2 world_position,
    vec2 client_position,
    f32 client_rotation);

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
    if (render->ui_atlas)
        render_destroy_texture(render->ui_atlas);
    if (render->main_menu_texture)
        render_destroy_texture(render->main_menu_texture);
}

Result
draw_bullet(const PlaneRender *render, Bullet *bullet, SimplePlane *client)
{
    assert(bullet->used);

    return draw_texture_relative(
        render->render,
        render->bullet_texture,
        NULL,
        bullet->heading,
        (vec2){.025, 0.05},
        bullet->p,
        client->position,
        client->heading);
}

// Draw a texture centered around a point, relative to the client
Result draw_texture_relative(
    const Render *render,
    const RenderTexture *texture,
    const RenderRect *texture_src,
    f32 rotation, // must already be relative to client
    vec2 size,
    vec2 world_position,
    vec2 client_position,
    f32 client_rotation)
{
    vec2 relative_position, screen_position;

    // translate position to be relative to client
    apply_client_transform(
        client_position, client_rotation, world_position, relative_position);

    ivec2 iscreen_size;
    if (render_get_render_size(render, &iscreen_size[0], &iscreen_size[1]) !=
        RENDER_SUCCESS)
        return RS_FAILURE;

    pos_to_screen(iscreen_size, relative_position, screen_position);

    vec2 screen_size;
    dimension_to_screen(iscreen_size, size, screen_size);

    RenderRect centered_rect = calculate_rect_centered(
        screen_position[0], screen_position[1], screen_size[0], screen_size[1]);

    return render_draw_texture(
               render,
               &centered_rect,
               texture_src,
               texture,
               NULL,
               convert_rotation_relative(rotation, client_rotation)) ==
                   RENDER_SUCCESS
               ? RS_SUCCESS
               : RS_FAILURE;
}

Result draw_plane(const PlaneRender *r, SimplePlane *client, SimplePlane *drawn)
{

    // draw bullets
    for (size_t i = 0; i < MAX_BULLET_COUNT; i++)
    {
        // TODO use new function
        if (drawn->active_bullets[i].used)
            draw_bullet(r, &drawn->active_bullets[i], client);
    } // get correct plane texture

    RenderRect plane_texture_rect = find_plane_texture_rect(drawn->plane_type);

    // TODO convert to screen coordinates / scaling -1/1 values
    vec2 plane_size = {PLANE_SIZE_PX, PLANE_SIZE_PX};

    return draw_texture_relative(
        r->render,
        r->plane_textures,
        &plane_texture_rect,
        drawn->heading + M_PI_4,
        plane_size,
        drawn->position,
        client->position,
        client->heading);
}

NONULL(1, 2)
Result draw_chunk(const PlaneRender *r, SimplePlane *client, const Chunk *c)
{
    vec2 chunk_pos = {
        c->grid_coordinate[0] * CHUNK_SIZE, c->grid_coordinate[1] * CHUNK_SIZE};
    // try and remove gap between chunks due to aliasing

    glm_vec2_scale(chunk_pos, 0.995f, chunk_pos);

    assert(c->texture);
    return draw_texture_relative(
        r->render,
        c->texture,
        NULL,
        0,
        (vec2){CHUNK_SIZE, CHUNK_SIZE},
        chunk_pos,
        client->position,
        client->heading);
}

Result draw_menu_bg(const PlaneRender *render)
{
    i32 w, h;
    i32 tw, th;
    render_get_render_size(render->render, &w, &h);
    render_get_texture_size(render->main_menu_texture, &tw, &th);

    RenderRect rect = {0, 0, w, th / (float)((float)tw / w)};
    render_draw_texture(
        render->render, &rect, NULL, render->main_menu_texture, NULL, 0);
    return RS_SUCCESS;
}

/*
 * HELPER IMPLEMENTATION
 */

f32 convert_rotation_relative(f32 input, f32 client_heading)
{
    return input - client_heading;
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

void dimension_to_screen(ivec2 iscreen_size, vec2 dimension, vec2 dest)
{
    glm_vec2_scale(dimension, 0.5f, dest);
    glm_vec2_scale(dest, (float)iscreen_size[1], dest);
}

// map coordinates from a standard cartesian plane to the sdl scree
void pos_to_screen(ivec2 iscreen_size, vec2 p, vec2 out)
{
    vec2 screen_size = {iscreen_size[0], iscreen_size[1]};

    f32 width_factor = (f32)screen_size[0] / (f32)screen_size[1];

    // move coordinates
    glm_vec2_mul(p, (vec2){0.5 / width_factor, -0.5}, out);
    glm_vec2_add(out, (vec2){0.5, 0.5}, out);
    glm_vec2_mul(out, screen_size, out);
}

void apply_client_transform(
    vec2 client_position, f32 client_rotation, vec2 position, vec2 dest)
{
    glm_vec2_sub(position, client_position, dest);
    glm_vec2_rotate(dest, client_rotation, dest);
}
