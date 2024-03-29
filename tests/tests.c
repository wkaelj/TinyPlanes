#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include "../shared/plane.h"

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

int test_count = 0;

char *test_test(void) { return NULL; }

bool compare_position(Position p1, Position p2)
{
    return p1.x == p2.x && p1.y == p2.y;
}

char *test_plane_local(void)
{
    Position input           = {0, 0};
    Position client_plane    = {0, 10};
    Position expected_output = {.x = 10.f, .y = 0.f};
    f32 heading              = 0;
    f32 expected_heading     = M_PI_2;
    Position p = convert_point_relative(input, client_plane, heading);
    printf("p = (%f, %f)\n", p.x, p.y);
    TEST_ASSERT(
        compare_position(p, expected_output), "Incorrect output position");
    TEST_ASSERT(
        convert_rotation_relative(heading, expected_heading),
        "Incorrect output rotation");

    return NULL;
}

int main(int argc, char **argv)
{
    TEST(test_test());
    TEST(test_plane_local());

    return 0;
}
