#pragma once

#include "render/render.h"
#include <SDL2/SDL.h>
#include <types.h>

#define CHUNK_RESOLUTION (8)
#define CHUNK_SIZE (0.5)

#define CHUNK_COLOUR_COUNT (512)

#define CHUNK_GEN_OCT (3)
#define CHUNK_GEN_SCALE ((float)0.5)

typedef struct Chunk
{
    ivec2 grid_coordinate;
    RenderTexture *texture;
} Chunk;

Chunk create_chunk(const Render *r, ivec2 grid_coordinate);

SDL_Surface *create_raw_chunk(ivec2 grid_coordinate);

void destroy_chunk(Chunk *c);

// find which grid coordinate point is in, stores it in dest
void chunk_containing(vec2 point, ivec2 dest);
