#pragma once

#include "plane.h"
#include <math.h>

typedef enum PlaneType
{
    PLANE_TYPE_F14 = 0,
    PLANE_TYPE_FA18,
    PLANE_TYPE_F16,
    PLANE_TYPE_F4,
    PLANE_TYPE_F22,
    PLANE_TYPE_F35,
    PLANE_TYPE_MAX,
} PlaneType;

__always_inline Plane create_plane_type(PlaneType type)
{
    switch (type)
    {

        // PLANE_TYPE, MIN_SPEED, THRUST, DRAG, TURN_RATE, BULLET_COUNT
    case PLANE_TYPE_F14:
        return create_plane(type, 0.12, 0.10, 0.07, 0.2 * M_PI, 128);
    case PLANE_TYPE_FA18:
        return create_plane(type, 0.05, 0.05, 0.05, 0.25 * M_PI, 128);
    case PLANE_TYPE_F16:
        return create_plane(type, 0.06, 0.5, 0.04, 0.4 * M_PI, 75);
    case PLANE_TYPE_F4:
        return create_plane(type, 0.20, 0.15, 0.10, 0.15 * M_PI, 50);
    case PLANE_TYPE_F22:
        return create_plane(type, 0.08, 0.70, 0.04, 0.3 * M_PI, 50);
    case PLANE_TYPE_F35:
        return create_plane(type, 0.15, 0.65, 0.03, 0.2 * M_PI, 50);
    case PLANE_TYPE_MAX: break;
    }
}
