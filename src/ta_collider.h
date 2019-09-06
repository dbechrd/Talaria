#pragma once
#include "ta_math.h"
#include "dlb/dlb_types.h"

typedef enum ta_collider_type {
    TA_COLLIDER_PLANE   = 0,
    TA_COLLIDER_SPHERE  = 1,
    TA_COLLIDER_AABB    = 2,
    TA_COLLIDER_OBB     = 3,
    TA_COLLIDER_COUNT
} ta_collider_type;

typedef struct ta_collider {
    ta_collider_type type;
    float mass;
    ta_mat3 tensor;
    ta_vec3 center_world;  // TODO: Update this whenever rigid body moves
    union {
        // NOTE: Must all start with ta_vec3 center
        ta_vec3 center;
        ta_sphere sphere;
        ta_aabb aabb;
        ta_obb obb;
        ta_plane plane;
    } data;
} ta_collider;

bool ta_intersect_ray_sphere(ta_ray ray, ta_sphere sphere, float *t);