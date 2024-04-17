#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include <unistd.h>
#include "../client/perlin_noise.h"

#include <SDL2/SDL.h>

// local access headers
#include "_plane_render.h"

#define TEST_ASSERT(statement, msg) \
    do                              \
    {                               \
        if ((statement) == false)   \
            return (msg);           \
    } while (0)

#define TEST(call)                                                \
    do                                                            \
    {                                                             \
        char *m = (call);                                         \
        if (m != NULL)                                            \
            printf("Test %u FAILED, error: %s\n", test_count, m); \
        else                                                      \
            (printf("Test %u PASSED\n", test_count));             \
        ++test_count;                                             \
    } while (0)

static int test_count = 0;

bool compare_position(vec2 p1, vec2 p2) { return glm_vec2_eqv(p1, p2); }

char *test_plane_local(void)
{
    {
        vec2 input           = {0, 0};
        vec2 client_plane    = {0, 10};
        vec2 expected_output = {0, -10};
        f32 heading          = 0;
        vec2 p;
        apply_client_transform(client_plane, heading, input, p);
        printf("p = (%f, %f)\n", p[0], p[1]);
        TEST_ASSERT(
            compare_position(p, expected_output),
            "Incorrect output position 1");
    }

    {
        vec2 input           = {10, 0};
        vec2 client_plane    = {0, 0};
        vec2 expected_output = {0, -10};
        f32 heading          = M_PI_2;
        vec2 p;
        apply_client_transform(client_plane, heading, input, p);
        printf("p = (%f, %f)\n", p[0], p[1]);
        TEST_ASSERT(
            compare_position(p, expected_output),
            "Incorrect output position 2");
    }

    return NULL;
}

char *test_rotation_local()
{
    f32 input_heading  = M_PI;
    f32 client_heading = M_PI_2;
    f32 output_heading =
        convert_rotation_relative(input_heading, client_heading);

    f32 expected_heading = M_PI_2;

    TEST_ASSERT(output_heading == expected_heading, "Incorrect heading");

    return NULL;
}

char *test_pos_to_screen(void)
{
    vec2 out;
    ivec2 screen = {1920, 1080};
    pos_to_screen(screen, (vec2){-1, 1}, out);
    TEST_ASSERT(glm_vec2_eqv(out, (vec2){0, 0}), "Incorrect position");
    pos_to_screen(screen, (vec2){1, -1}, out);
    TEST_ASSERT(glm_vec2_eqv(out, (vec2){1920, 1080}), "Incorrect position");

    return NULL;
}

char *test_perlin_noise(void)
{
    SDL_Init(SDL_INIT_VIDEO);

    int w = 0, h = 600;

    SDL_Window *window = SDL_CreateWindow(
        "Perlin Demo",
        200,
        500,
        w,
        h,
        SDL_WINDOW_FULLSCREEN | SDL_WINDOW_ALWAYS_ON_TOP);

    SDL_Renderer *render = SDL_CreateRenderer(
        window, 0, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    SDL_SetRenderDrawColor(render, 0, 0, 0, 255);
    SDL_RenderClear(render);
    SDL_RenderPresent(render);

    SDL_GetWindowSizeInPixels(window, &w, &h);

    SDL_Texture *texture = SDL_CreateTexture(
        render, SDL_PIXELFORMAT_RGB888, SDL_TEXTUREACCESS_TARGET, 1000, 1000);

    int tw = 1000, th = 1000;

    SDL_SetRenderTarget(render, texture);
    const i32 oct         = 5;
    const f64 persistance = 0.5;

    for (int x = 0; x < tw; x++)
    {
        for (int y = 0; y < th; y++)
        {
            f64 n = octave_perlin_noise(
                (float)x / 100, (float)y / 100, 1, oct, persistance);

            n += 1;
            n /= 2;

            SDL_SetRenderDrawColor(render, 255 * n, 255 * n, 255 * n, 255);
            SDL_RenderDrawPoint(render, x, y);
            SDL_SetRenderDrawColor(render, 0, 0, 0, 255);
        }
    }

    SDL_SetRenderTarget(render, NULL);
    SDL_Rect out_rect = {0, 0, w, h};
    SDL_RenderCopy(render, texture, NULL, &out_rect);
    SDL_RenderPresent(render);
    sleep(2);

    SDL_DestroyTexture(texture);

    SDL_DestroyRenderer(render);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return NULL;
}

int main([[maybe_unused]] int argc, [[maybe_unused]] char **argv)
{
    TEST(test_plane_local());
    TEST(test_pos_to_screen());
    TEST(test_rotation_local());
    TEST(test_perlin_noise());

    return 0;
}
