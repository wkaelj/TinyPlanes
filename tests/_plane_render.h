#pragma once
// define internal functions of plane_render.c for unit testing
#include <types.h>

f32 convert_rotation_relative(f32 input, f32 client_heading);
void pos_to_screen(ivec2 screen_size, vec2 p, vec2 out);

void apply_client_transform(
    vec2 client_position, f32 client_rotation, vec2 position, vec2 dest);
