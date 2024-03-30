#pragma once

/*
 * Functions to generate perlin noise
 */

#include <types.h>

// create perlin noise given the coordinate x and y
// returns a value between -1 and 1
f32 perlin_noise(u32 seed, f32 x, f32 y);
