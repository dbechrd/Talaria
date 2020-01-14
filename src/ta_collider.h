#pragma once
#include "ta_math.h"
#include "dlb/dlb_types.h"

typedef enum ta_collider_type {
    TA_COLLIDER_PLANE   = 0,
    TA_COLLIDER_SPHERE  = 1,
    TA_COLLIDER_OBB     = 2,
    TA_COLLIDER_COUNT
} ta_collider_type;

typedef struct ta_collider {
    ta_collider_type type;
    float mass;
    ta_mat3 tensor;
    union {
        ta_vec3 center;    // Offset relative to parent rigid body
        // NOTE: Must all start with ta_vec3 center
        ta_sphere sphere;
        ta_obb obb;
        ta_plane plane;  // This is technically a half-space, not a true plane
    } data;
} ta_collider;

void ta_collider_init(ta_collider *collider);
ta_mat3 ta_collider_inv_tensor(ta_collider *collider, float mass);
ta_aabb ta_collider_world_bounds(ta_collider *collider, ta_xform *xform);
void ta_collider_render(ta_collider *collider, ta_rgba color);