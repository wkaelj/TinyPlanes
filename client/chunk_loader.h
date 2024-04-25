#pragma once

// multithreaded chunk loading

#include "render/render.h"
#include <types.h>

#include "chunk.h"

// maybe not the best but at least it's cross platform
#include <SDL2/SDL_thread.h>
#include <SDL2/SDL_mutex.h>

#define CHUNK_RADIUS (1)
#define _SQR(x) ((x) * (x))
#define CHUNK_COUNT (_SQR(CHUNK_RADIUS * 2 + 1))
static_assert(CHUNK_COUNT == 9);

typedef struct ChunkList
{
    Chunk chunks[CHUNK_COUNT];
    SDL_mutex *sync_mutex;
    ivec2 player_chunk;
} ChunkList;

ChunkList create_chunk_list(const Render *r);

void destroy_chunk_list(ChunkList *l);

// run when player changes chunks
// TODO include render
// thread loader relies on the fact that if the render is in use chunks
// mutex is locked slighty busted hack that should work
void update_chunk_list(const Render *r, ChunkList *l, vec2 player_pos);

void chunk_list_lock(ChunkList *l);
void chunk_list_unlock(ChunkList *l);
