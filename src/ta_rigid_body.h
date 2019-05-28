#pragma once
#include "ta_math.h"
#include "ta_scene.h"

typedef struct {
    ta_vec3 center;
    float radius;
} ta_sphere;

typedef struct {
    ta_vec3 center;
    ta_vec3 extents;
} ta_aabb;

typedef struct {
    ta_vec3 center;
    ta_vec3 extents;
    ta_vec3 axes[3];
} ta_obb;

typedef enum {
    TA_COLLIDER_SPHERE  = 0,
    TA_COLLIDER_AABB    = 1,
    TA_COLLIDER_OBB     = 2,
} ta_collider_type;

typedef struct {
    ta_collider_type type;
    union {
        ta_sphere sphere;
        ta_aabb aabb;
        ta_obb obb;
    } data;
} ta_collider;

typedef struct ta_rigid_body_s {
    ta_scene *scene;
    const char *uid;
    ta_transform transform;
    ta_collider collider;

    // http://allenchou.net/2013/12/game-physics-introduction/
    //float mass;
    //float inv_mass;
    //ta_mat4 tensor;
    //ta_mat4 inv_tensor;
    //ta_vec3 position;
    //ta_quat rotation;
    //ta_vec3 velocity;
    //ta_vec3 ang_velocity;
} ta_rigid_body;

const char *ta_collider_type_str(int type);
bool ta_intersect_sphere_vs_sphere(const ta_sphere *a, const ta_sphere *b);
void ta_rigid_body_update(ta_rigid_body *rigid_body, double dt);