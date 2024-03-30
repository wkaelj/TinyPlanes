#include "perlin_noise.h"
#include <stdlib.h>

f32 perlin_noise(u32 seed, f32 x, f32 y)
{
    // generation code
    return 1.f / rand_r(&seed);
}
