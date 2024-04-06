#pragma once

#include "types.h"
#include <sys/types.h>

// This header defines the features of an airplane, and allows airplanes
// to be created with various features, such as turn rate, top speed,
// min speed, and weapons

#define MAX_BULLET_COUNT 128

typedef enum Direction
{
    LEFT  = -1,
    RIGHT = 1
} Direction;

typedef struct Bullet
{
    bool used; // know if a bullet buffer location is in use
    vec2 p;
    f32 heading;
    f32 speed;
    f32 drag;
} Bullet;

typedef struct Missile
{
    bool used;
    vec2 p;
    f32 heading;
    f32 speed;
    f32 drag;
} Missile;

typedef struct Plane
{
    int plane_type;
    // constant terms
    f32 turn_rate; // degrees per second of rotation (change of heading)
    f32 min_speed; // minimum speed
    // thrust and drag are units/per second of acceleration
    f32 thrust;      // rate of max acceleration
    f32 drag_factor; // percentage of speed to take away each frame

    f32 speed;
    f32 throttle;
    f32 heading;

    vec2 position; // airplane position

    // ammunition
    size_t bullets_remaining;
    time_t next_fire_time;
    time_t fire_interval;
    Bullet active_bullets[MAX_BULLET_COUNT];
} Plane;

// simple plane for networking and rendering
typedef struct SimplePlane
{
    int plane_type;
    vec2 position;
    f32 heading;
    f32 velocity;

    Bullet active_bullets[MAX_BULLET_COUNT];
} SimplePlane;

// create a airplane with the given parameters. Will be used internally to
// initialize each type of airplane
Plane create_plane(
    int plane_type,
    f32 min_speed,
    f32 thrust,
    f32 drag,
    f32 turn_rate,
    u8 bullet_count);

// convert a plane to a simple plane
SimplePlane create_simple_plane(Plane *complex_plane);

// update a planes position and speed base on airplane parameters
void plane_update(Plane *plane, f32 delta);
void plane_turn(Plane *p, f32 delta, Direction d, f32 factor);

void plane_fire_bullet(Plane *p);

void update_missile(
    Missile *missile, const SimplePlane *missile_target, f32 delta);
void update_bullet(Bullet *bullet, f32 delta);

// move an airplane to position p without changing speed or heading
static inline void plane_set_position(Plane *plane, vec2 p)
{
    glm_vec2_copy(p, plane->position);
}
