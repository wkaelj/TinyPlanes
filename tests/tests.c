#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

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

int main([[maybe_unused]] int argc, [[maybe_unused]] char **argv)
{
    TEST(test_plane_local());
    TEST(test_pos_to_screen());
    TEST(test_rotation_local());

    return 0;
}
