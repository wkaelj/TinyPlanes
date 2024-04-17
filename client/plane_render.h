#pragma once

#include "plane.h"
#include "render/render.h"
#include "types.h"

#include "chunk.h"

typedef struct PlaneRender
{
    const Render *render; // reference to a render to use

    RenderTexture *plane_textures;
    RenderTexture *bullet_texture;
    RenderTexture *terrain_texture;
    RenderTexture *ui_atlas;
    RenderTexture *main_menu_texture;

    RenderFont *ui_font; // reference to font, not owned by game

    size_t ui_bullet_number, ui_speed_number;
    RenderText *ui_bullet_display, *ui_speed_display;
} PlaneRender;

NONULL(1) PlaneRender create_plane_render(const Render *r, RenderFont *ui_font);
void destroy_plane_render(PlaneRender *render);

NONULL(1, 2, 3)
Result
draw_plane(const PlaneRender *r, SimplePlane *client, SimplePlane *drawn);

NONULL(1, 2)
Result draw_terrain(const PlaneRender *r, SimplePlane *client);

NONULL(1, 2)
Result draw_chunk(const PlaneRender *r, SimplePlane *client, const Chunk *c);

NONULL(1) Result draw_menu_bg(const PlaneRender *render);
