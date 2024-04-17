#pragma once

/*
 * Functions to generate perlin noise
 */

#include <types.h>

// create perlin noise given the coordinate x and y
// returns a value between -1 and 1
f64 perlin_noise(f64 x, f64 y, f64 z);
// O(octaves)
f64 octave_perlin_noise(f64 x, f64 y, f64 z, int octaves, f64 persistence);

// convert noise from -1/1 to 0/1
static inline f64 normalize_noise(f64 n) { return (n + 1) / 2; }
