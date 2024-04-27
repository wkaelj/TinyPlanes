#include "chunk.h"
#include "messenger.h"
#include "perlin_noise.h"

#include <assert.h>
#include <cglm/vec2-ext.h>

// use lower level sdl functions to improve drawing performance
#include <SDL2/SDL.h>

static vec3 colour_map[] = {
    {38,  33,  190},
    {38,  33,  190},
    {38,  33,  190},
    {38,  33,  190},
    {38,  33,  190},
    {38,  33,  190},
    {38,  33,  190},
    {38,  33,  190},
    {38,  33,  190},
    {38,  33,  190},
    {44,  168, 56 },
    {35,  124, 44 },
    {49,  88,  52 },
    {69,  69,  69 },
    {100, 100, 100},
    {192, 192, 192},
    {192, 192, 192},
    {192, 192, 192},
    {192, 192, 192},
    {226, 226, 226},
    {226, 226, 226},
    {226, 226, 226},
    {226, 226, 226},
};

static RenderColour world_colours[CHUNK_COLOUR_COUNT] = {};

static inline void lerp_color(vec3 a, vec3 b, f32 weight, vec3 dest)
{
    glm_vec3_lerp(a, b, weight, dest);
}

static RenderColour vec3_to_colour(vec3 a)
{
    return (RenderColour){a[0] * 255, a[1] * 255, a[2] * 255, 255};
}

[[maybe_unused]] __attribute__((constructor)) static void
populate_world_colours()
{
    size_t step_size = array_length(world_colours) / array_length(colour_map);
    size_t extra_end = array_length(world_colours) % array_length(colour_map);

    for (size_t colour_chunk = 0; colour_chunk < array_length(colour_map);
         colour_chunk++)
    {
        // lerp between steps for each chunk
        for (size_t step = 0; step < step_size; step++)
        {
            // find colours and scale properly
            vec3 colour;
            glm_vec3_divs(colour_map[colour_chunk], 255, colour);
            vec3 lerp_colour;
            glm_vec3_divs(
                colour_map
                    [colour_chunk + 1 >= array_length(colour_map)
                         ? colour_chunk
                         : colour_chunk + 1],
                255,
                lerp_colour);

            lerp_color(colour, lerp_colour, (f32)step / step_size, colour);
            world_colours[colour_chunk * step_size + step] =
                vec3_to_colour(colour);
        }
    }

    for (size_t i = 1; i < extra_end; i++)
    {
        assert(array_length(world_colours) == CHUNK_COLOUR_COUNT);
        world_colours[array_length(world_colours) - i] =
            vec3_to_colour(colour_map[array_length(colour_map) - 1]);
    }
}
RenderColour generate_world_pixel(int seed, vec2 coordinate)
{

    f32 noise = normalize_noise(octave_perlin_noise(
        seed,
        coordinate[0] * CHUNK_GEN_SCALE,
        coordinate[1] * CHUNK_GEN_SCALE,
        CHUNK_GEN_OCT,
        0.5));
    return world_colours[(int)(noise * (size_t)array_length(world_colours))];
}

// find the world position for a pixel in a chunk's texture
void find_world_pos(ivec2 chunk_grid, ivec2 pixel_pos, vec2 out)
{
    vec2 chunk_center = {chunk_grid[0], -chunk_grid[1]};
    vec2 top_left;
    glm_vec2_add(
        chunk_center, (vec2){-(CHUNK_SIZE / 2), -(CHUNK_SIZE / 2)}, top_left);

    out[0] =
        top_left[0] + ((float)pixel_pos[0] / (CHUNK_RESOLUTION * CHUNK_SIZE));
    out[1] =
        top_left[1] + ((float)pixel_pos[1] / (CHUNK_RESOLUTION * CHUNK_SIZE));
}

// NOTE: DO NOT TOUCH THE BITS
static SDL_Surface *fill_chunk(ivec2 grid_coordinate)
{
    SDL_Surface *surface = SDL_CreateRGBSurfaceWithFormat(
        0, CHUNK_RESOLUTION, CHUNK_RESOLUTION, 32, SDL_PIXELFORMAT_RGBA8888);

    if (surface == NULL)
    {
        log_fatal("Cannot create chunks, SDL Error: %s", SDL_GetError());
        assert(surface);
    }

    vec2 world_pos;
    RenderColour point_colour;

    SDL_LockSurface(surface);

    // generate grid map
    for (size_t x = 0; x < CHUNK_RESOLUTION; x++)
    {
        for (size_t y = 0; y < CHUNK_RESOLUTION; y++)
        {
            // find world coordinate
            find_world_pos(grid_coordinate, (ivec2){x, y}, world_pos);
            // find colour of point
            point_colour = generate_world_pixel(2, world_pos);
            // draw colour to texture
            u32 colour = SDL_MapRGBA(
                surface->format,
                point_colour.r,
                point_colour.g,
                point_colour.b,
                point_colour.a);
            assert((colour & 0xFF) == 255);
            memcpy(
                (u8 *)surface->pixels + x * surface->pitch +
                    y * surface->format->BytesPerPixel,
                &colour,
                sizeof(colour));
        }
    }
    SDL_UnlockSurface(surface);

    return surface;
}

Chunk create_chunk(const Render *r, ivec2 grid_coordinate)
{
    Chunk c;
    glm_ivec2_copy(grid_coordinate, c.grid_coordinate);

    SDL_Surface *surface = fill_chunk(c.grid_coordinate);
    c.texture            = (RenderTexture *)SDL_CreateTextureFromSurface(
        render_internal(r), surface);

    SDL_FreeSurface(surface);

    return c;
}

SDL_Surface *create_raw_chunk(ivec2 grid_coordinate)
{
    SDL_Surface *s = fill_chunk(grid_coordinate);
    assert(s != NULL);
    return s;
}

void destroy_chunk(Chunk *c) { render_destroy_texture(c->texture); }

static inline i32 signof(f64 x) { return (x > 0) - (x < 0); }

// find which grid coordinate point is in, stores it in dest
void chunk_containing(vec2 point, ivec2 dest)
{
    dest[0] = (point[0] + signof(point[0]) * CHUNK_SIZE / 2) / CHUNK_SIZE;
    dest[1] = (point[1] + signof(point[1]) * CHUNK_SIZE / 2) / CHUNK_SIZE;
}
