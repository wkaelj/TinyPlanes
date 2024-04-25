#include "chunk_loader.h"
#include "messenger.h"
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_surface.h>
#include <assert.h>

// not defined by glm for some reason
#ifndef GLM_IVEC2_ZERO
#define GLM_IVEC2_ZERO ((ivec2){0, 0})
#endif
#ifndef GLM_IVEC2_ZERO_INIT
#define GLM_IVEC2_ZERO_INIT \
    {                       \
        0, 0                \
    }
#endif

typedef struct ThreadInfo
{
    SDL_mutex *sync_mutex;
    const Render *render;
    Chunk *chunk_list;
    vec2 player_pos;
} ThreadInfo;

extern bool glm_ivec2_eqv(ivec2 a, ivec2 b);

// store chunks in surface form befor they are synced to main thread
typedef struct RawChunk
{
    SDL_Surface *surface;
    ivec2 grid_coordinate;
} RawChunk;

void create_chunks(RawChunk *raws, ivec2 center)
{
    // starting coordinate
    i32 x_start = center[0] - CHUNK_RADIUS;
    i32 y_start = center[1] - CHUNK_RADIUS;
    i32 size    = CHUNK_RADIUS * 2 + 1; // width and height to gen
    size_t i    = 0;
    log_debug("Generating chunks start (%d, %d)", x_start, y_start);
    SDL_mutex *raws_mutex = SDL_CreateMutex();
#pragma omp parallel for
    for (i32 x = x_start; x < x_start + size; x++)
    {
        for (i32 y = y_start; y < y_start + size; y++)
        {
            printf("Creating chunk (%d, %d)   \r", x, y);
            fflush(stdout);
            assert(i < CHUNK_COUNT);
            SDL_LockMutex(raws_mutex);
            raws[i].surface = create_raw_chunk((ivec2){x, y});
            glm_ivec2_copy((ivec2){x, y}, raws[i].grid_coordinate);
            SDL_UnlockMutex(raws_mutex);
            i++;
        }
    }
    SDL_DestroyMutex(raws_mutex);
    puts("");
}

static int thread_main(void *arg)
{
    ThreadInfo *info = arg;
    vec2 player_pos;
    glm_vec2_copy(info->player_pos, player_pos);

    const Render *r = info->render;

    Chunk *chunks           = info->chunk_list;
    SDL_mutex *chunks_mutex = info->sync_mutex;

    free(arg);

    ivec2 current_chunk;
    chunk_containing(player_pos, current_chunk);

    // update chunks
    RawChunk raws[CHUNK_COUNT];
    create_chunks(raws, current_chunk);

    // write chunk surfaces
    SDL_LockMutex(chunks_mutex);
    for (size_t i = 0; i < CHUNK_COUNT; i++)
    {
        render_destroy_texture(chunks[i].texture);
        chunks[i].texture =
            render_create_texture_from_surface(r, raws[i].surface);
        if (i == 0)
        {
            IMG_SaveJPG(raws[i].surface, "test.jpeg", 66);
        }
        assert(chunks[i].texture);
        glm_ivec2_copy(raws[i].grid_coordinate, chunks[i].grid_coordinate);
        SDL_FreeSurface(raws[i].surface);
    }
    SDL_UnlockMutex(chunks_mutex);

    return 0;
}

ChunkList create_chunk_list(const Render *r)
{
    ChunkList l = {
        .sync_mutex   = SDL_CreateMutex(),
        .player_chunk = GLM_IVEC2_ZERO_INIT,
    };

    // create initial chunks
    RawChunk raws[CHUNK_COUNT];
    create_chunks(raws, GLM_IVEC2_ZERO);
    SDL_LockMutex(l.sync_mutex);
    for (size_t i = 0; i < CHUNK_COUNT; i++)
    {
        l.chunks[i].texture =
            render_create_texture_from_surface(r, raws[i].surface);
        if (l.chunks[i].texture == NULL)
        {
            log_fatal(
                "Failed to create chunk %lu %s\n \t(%d, %d)",
                i,
                SDL_GetError(),
                l.chunks[i].grid_coordinate[0],
                l.chunks[i].grid_coordinate[1]);
        }
        assert(l.chunks[i].texture);
        glm_ivec2_copy(raws[i].grid_coordinate, l.chunks[i].grid_coordinate);
        SDL_FreeSurface(raws[i].surface);
    }
    SDL_UnlockMutex(l.sync_mutex);

    return l;
}
void destroy_chunk_list(ChunkList *l)
{
    for (size_t i = 0; i < CHUNK_COUNT; i++)
        destroy_chunk(&l->chunks[i]);
    SDL_DestroyMutex(l->sync_mutex);
}

void update_chunk_list(const Render *r, ChunkList *l, vec2 player_pos)
{
    // check if chunks need updating
    ivec2 current_chunk;
    chunk_containing(player_pos, current_chunk);
    // if player is in the same chunk don't bother making new ones
    if (glm_ivec2_eqv(current_chunk, l->player_chunk))
        return;

    glm_ivec2_copy(current_chunk, l->player_chunk);

    log_info(
        "Generating new chunk set (%d, %d)",
        current_chunk[0],
        current_chunk[1]);

    ThreadInfo *thread_info = malloc(sizeof(ThreadInfo));
    assert(thread_info);
    thread_info->chunk_list = l->chunks;
    thread_info->sync_mutex = l->sync_mutex;
    thread_info->render     = r;
    glm_vec2_copy(player_pos, thread_info->player_pos);
    SDL_Thread *t = SDL_CreateThread(thread_main, "world_gen", thread_info);
    SDL_DetachThread(t);
}

void chunk_list_lock(ChunkList *l) { SDL_LockMutex(l->sync_mutex); }
void chunk_list_unlock(ChunkList *l) { SDL_UnlockMutex(l->sync_mutex); }
