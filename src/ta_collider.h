#pragma once
#include "ta_math.h"
#include "dlb/dlb_types.h"

typedef enum ta_collider_type {
    TA_COLLIDER_PLANE   = 0,
    TA_COLLIDER_SPHERE  = 1,
    TA_COLLIDER_OBB     = 2,
    TA_COLLIDER_CAPSULE = 3,
    TA_COLLIDER_COUNT
} ta_collider_type;

typedef struct ta_collider {
    ta_collider_type type;      // collider type (plane, sphere, obb, etc.)
    float            mass;      // mass of collider, in kg

    // TODO: compute tensor/centroid of rigid body by accumulating all colliders:
    // http://allenchou.net/2013/12/game-physics-motion-dynamics-implementations/
    ta_mat3          tensor;    // inertia tensor of collider (3D)
    union {
        ta_vec3      center;    // Offset relative to parent rigid body
        // NOTE: Must all start with ta_vec3 center
        ta_sphere    sphere;
        ta_obb       obb;
        ta_plane     plane;      // NOTE: This is technically a half-space, not a true plane
        ta_capsule   capsule;
    } data;
} ta_collider;

const char *ta_collider_type_str    (int type);

void ta_collider_init               (ta_collider *collider);
ta_mat3 ta_collider_inv_tensor      (ta_collider *collider, float mass);
ta_aabb ta_collider_world_bounds    (ta_collider *collider, const ta_xform *xform);
void ta_collider_push               (ta_collider *collider, ta_rgba color);