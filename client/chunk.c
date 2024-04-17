#include "chunk.h"
#include "perlin_noise.h"

#include <cglm/vec2-ext.h>

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

    for (size_t i = 0; i < extra_end; i++)
    {
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
        chunk_center, (vec2){CHUNK_SIZE / 2, -(CHUNK_SIZE / 2)}, top_left);

    out[0] =
        top_left[0] + ((float)pixel_pos[0] / CHUNK_RESOLUTION * CHUNK_SIZE);
    out[1] =
        top_left[1] + ((float)pixel_pos[1] / CHUNK_RESOLUTION * CHUNK_SIZE);
}

static void
fill_chunk(const Render *r, RenderTexture *texture, ivec2 grid_coordinate)
{
    render_set_drawing_target(r, texture);

    vec2 world_pos;
    RenderColour point_colour;

    // generate grid map
    for (size_t x = 0; x < CHUNK_RESOLUTION; x++)
    {
        for (size_t y = 0; y < CHUNK_RESOLUTION; y++)
        {

            // find world coordinate
            find_world_pos(grid_coordinate, (ivec2){x, y}, world_pos);
            // find colour of point
            point_colour = generate_world_pixel(19284, world_pos);
            // draw colour to texture
            render_set_colour(r, point_colour);
            render_draw_point(r, x, y);
        }
    }

    render_set_drawing_target(r, NULL);
}

Chunk create_chunk(const Render *r, ivec2 grid_coordinate)
{
    Chunk c;
    glm_ivec2_copy(grid_coordinate, c.grid_coordinate);

    c.texture =
        render_create_drawable_texture(r, CHUNK_RESOLUTION, CHUNK_RESOLUTION);

    fill_chunk(r, c.texture, c.grid_coordinate);

    return c;
}

void destroy_chunk(Chunk *c) { render_destroy_texture(c->texture); }

// recreate chunk with a new location
// this moves the chunk and changes it's texture,
// equivelant to destroying and creating a new chunk
void move_chunk(const Render *r, Chunk *c, ivec2 new_coordinate)
{
    glm_ivec2_copy(new_coordinate, c->grid_coordinate);

    fill_chunk(r, c->texture, c->grid_coordinate);
}

// find which grid coordinate point is in, stores it in dest
void chunk_containing(vec2 point, ivec2 dest)
{
    f32 ix = (i32)(point[0]);
    f32 iy = (i32)(point[1]);

    dest[0] = ix / (CHUNK_SIZE / 2);
    dest[1] = iy / (CHUNK_SIZE / 2);
}

ChunkList create_chunk_list(const Render *r, f32 load_radius)
{
    ChunkList l = {
        .chunk_count  = 1,
        .chunks       = malloc(sizeof(Chunk)),
        .player_chunk = {0, 0},
        .load_radius  = load_radius,
    };

    l.chunks[0] = create_chunk(r, l.player_chunk);
    return l;
}

void destroy_chunk_list(ChunkList *l)
{
    for (size_t i = 0; i < l->chunk_count; i++)
        render_destroy_texture(l->chunks[i].texture);

    free(l->chunks);
}

extern bool glm_ivec2_eqv(ivec2 a, ivec2 b);

void update_chunk_list(const Render *r, ChunkList *l, vec2 player_pos)
{
    ivec2 player_chunk;
    chunk_containing(player_pos, player_chunk);

    if (glm_ivec2_eqv(player_chunk, l->player_chunk) == false)
    {
        move_chunk(r, l->chunks, player_chunk);
        glm_ivec2_copy(player_chunk, l->player_chunk);
    }
}
