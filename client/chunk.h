#pragma once

// define chunks
// The 9 closest chunks need to be drawn each frame
// [][][]
// []XX[]
// [][][]

#include "render/render.h"
#include <types.h>

#define CHUNK_RESOLUTION (1080)
#define CHUNK_SIZE (2.0)

#define CHUNK_COLOUR_COUNT (512)

#define CHUNK_GEN_OCT (5)
#define CHUNK_GEN_SCALE ((float)1 * CHUNK_SIZE)

typedef struct Chunk
{
    ivec2 grid_coordinate;
    RenderTexture *texture;
} Chunk;

typedef struct ChunkList
{
    Chunk *chunks;
    size_t chunk_count;

    f32 load_radius;

    ivec2 player_chunk;
} ChunkList;

Chunk create_chunk(const Render *r, ivec2 grid_coordinate);

void destroy_chunk(Chunk *c);

// recreate chunk with a new location
// this moves the chunk and changes it's texture,
// equivelant to destroying and creating a new chunk
void move_chunk(const Render *r, Chunk *c, ivec2 new_coordinate);

// find which grid coordinate point is in, stores it in dest
void chunk_containing(vec2 point, ivec2 dest);

ChunkList create_chunk_list(const Render *r, f32 load_radius);

void destroy_chunk_list(ChunkList *l);

void update_chunk_list(const Render *r, ChunkList *l, vec2 player_pos);
