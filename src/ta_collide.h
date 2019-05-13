#pragma once
#include "ta_math.h"
#include "ta_scene.h"

typedef struct {
    ta_vec3 center;
    ta_vec3 extents;
} ta_aabb;

typedef struct {
    ta_vec3 center;
    ta_vec3 extents;
    ta_vec3 axes[3];
} ta_obb;

typedef enum ta_rigid_body_type {
    TA_COLLIDER_AABB,
    TA_COLLIDER_OBB,
} ta_collider_type;

typedef struct {
    ta_collider_type type;
    union {
        ta_aabb aabb;
        ta_obb obb;
    } data;
} ta_collider;

typedef struct ta_rigid_body_s {
    ta_scene *scene;
    const char *uid;
    ta_transform transform;
    ta_collider collider;
} ta_rigid_body;

const char *ta_collider_type_str(int type);