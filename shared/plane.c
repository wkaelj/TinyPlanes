#include "plane.h"
#include "messenger.h"
#include <assert.h>
#include <string.h>
#include <sys/time.h>
#include "utils.h"

#define BULLET_DRAG 0.01
#define BULLET_INITIAL_SPEED 1.75
#define BULLET_MINIMUM_SPEED 1.5

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
        .position          = GLM_VEC2_ZERO_INIT,
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
        .velocity   = p->speed,
    };
    // copy position
    glm_vec2_copy(p->position, out.position);
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

    vec2 offset = {0, p->speed * delta};
    glm_vec2_rotate(offset, -p->heading, offset);

    glm_vec2_add(p->position, offset, p->position);

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
    };
    glm_vec2_copy(p->position, new_bullet.p);

    p->active_bullets[bullet_index] = new_bullet;
    p->next_fire_time               = time + p->fire_interval;
    p->bullets_remaining--;
}

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

    vec2 offset = {0, 1};
    glm_vec2_rotate(offset, -bullet->heading, offset);
    glm_vec2_scale(offset, bullet->speed * delta, offset);

    glm_vec2_add(bullet->p, offset, bullet->p);
}

void plane_turn(Plane *p, f32 delta, Direction d, f32 factor)
{
    // add negative rotation, opposite of unit circle
    p->heading += p->turn_rate * d * factor * delta;
}
