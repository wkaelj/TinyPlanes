#pragma once
// define internal functions of plane_render.c for unit testing
#include <types.h>
typedef void *Render;

Position convert_point_relative(
    Position input, Position client_plane, f32 client_heading);
f32 convert_rotation_relative(f32 input, f32 client_heading);
Position pos_to_screen(const Render *restrict r, Position p);
