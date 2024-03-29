#include "plane.h"
#include "messenger.h"
#include <assert.h>
#include <math.h>
#include <string.h>
#include <sys/time.h>

#define BULLET_DRAG 20
#define BULLET_INITIAL_SPEED 800
#define BULLET_MINIMUM_SPEED 400

extern time_t get_time(void);

Plane create_plane(
    int plane_type,
    f32 min_speed,
    f32 thrust,
    f32 drag,
    f32 turn_rate,
    u8 bullet_count)
{
    Plane p = {
        .plane_type        = plane_type,
        .turn_rate         = turn_rate,
        .drag_factor       = drag,
        .thrust            = thrust,
        .min_speed         = min_speed,
        .position          = POSITION(0.f, 0.f),
        .heading           = 0.f,
        .throttle          = 1.f,
        .bullets_remaining = bullet_count,
        .fire_interval     = 100,
        .next_fire_time    = get_time() / 1000,
        .speed             = min_speed};
    return p;
}

SimplePlane create_simple_plane(Plane *p)
{
    SimplePlane out = {
        .plane_type = p->plane_type,
        .heading    = p->heading,
        .position   = p->position,
        .velocity   = p->speed,
    };
    // copy active bullets
    memcpy(out.active_bullets, p->active_bullets, sizeof(out.active_bullets));
    return out;
}

void plane_update(Plane *p, f32 delta)
{
    assert(p->throttle <= 1.f);
    p->speed += p->thrust * p->throttle * delta;
    p->speed *= 1 - p->drag_factor * delta;
    if (p->speed < p->min_speed)
        p->speed = p->min_speed;

    // calculate x and y offset for plane movement
    f32 x_offset = p->speed * sinf(p->heading);
    f32 y_offset = p->speed * cosf(p->heading);

    p->position.x += x_offset * delta;
    p->position.y += y_offset * delta;

    // update all bullets
    for (size_t i = 0; i < MAX_BULLET_COUNT; i++)
    {
        if (p->active_bullets[i].used)
            update_bullet(&p->active_bullets[i], delta);
    }
}

void plane_fire_bullet(Plane *p)
{
    if (p->bullets_remaining <= 0)
        return;

    // check allowed with fire rate
    time_t time = get_time() / 1000; // get time converted to ms
    if (p->next_fire_time > time)
    {
        return;
    }

    // find where to store bullet
    size_t bullet_index = SIZE_MAX;
    for (size_t i = 0; i < MAX_BULLET_COUNT; i++)
        if (p->active_bullets[i].used == false)
        {
            bullet_index = i;
            break;
        }
    if (bullet_index == SIZE_MAX)
    {
        log_info("No more active bullets available\n");
        return;
    }

    struct Bullet new_bullet = {
        .used    = true,
        .drag    = BULLET_DRAG,
        .heading = p->heading,
        .speed   = p->speed + BULLET_INITIAL_SPEED,
        .p       = p->position,
    };
    p->active_bullets[bullet_index] = new_bullet;
    p->next_fire_time               = time + p->fire_interval;
    p->bullets_remaining--;
}

void update_missile();
void update_bullet(Bullet *bullet, f32 delta)
{
    // do not operate on unused bullet
    assert(bullet->used == true);

    bullet->speed -= bullet->drag * delta;

    // get rid of bullet if too slow
    if (bullet->speed < BULLET_MINIMUM_SPEED)
    {
        bullet->used = false;
        return;
    }

    // calculate x and y offset for plane movement
    f32 x_offset = bullet->speed * sinf(bullet->heading);
    f32 y_offset = bullet->speed * cosf(bullet->heading);

    bullet->p.x += x_offset * delta;
    bullet->p.y += y_offset * delta;
}

void plane_turn(Plane *p, f32 delta, Direction d, f32 factor)
{
    // add negative rotation, opposite of unit circle
    p->heading += p->turn_rate * d * factor * delta;
}
