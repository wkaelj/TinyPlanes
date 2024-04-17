#include "perlin_noise.h"

#include <noise1234/noise1234.h>

f64 perlin_noise(f64 x, f64 y, f64 z) { return noise3(x, y, z); }

f64 octave_perlin_noise(f64 x, f64 y, f64 z, int octaves, f64 persistence)
{
    f64 total     = 0;
    f64 freq      = 1;
    f64 amp       = 1;
    f64 max_value = 0; // Used for normalizing result to 0.0 - 1.0
    for (i32 i = 0; i < octaves; i++)
    {
        total += noise3(x * freq, y * freq, z * freq) * amp;

        max_value += amp;

        amp *= persistence;
        freq *= 2;
    }

    return total / max_value;
}
