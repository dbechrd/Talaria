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
        // NOTE: Must all start with ta_vec3 center
        ta_vec3 center;
        ta_sphere sphere;
        ta_aabb aabb;
        ta_obb obb;
    } data;
} ta_collider;

typedef struct {
    ta_rigid_body *a;
    ta_rigid_body *b;
    ta_vec3 normal;
    float depth;
} ta_manifold;

typedef struct ta_rigid_body_s {
    ta_scene_ref ref;
    ta_transform transform;
    ta_collider collider;

    // http://allenchou.net/2013/12/game-physics-introduction/
    float mass;
    float inv_mass;
    float restitution;
    //ta_mat3 tensor;
    //ta_mat3 inv_tensor;
    ta_vec3 velocity;
    //ta_vec3 ang_velocity;
} ta_rigid_body;

const char *ta_collider_type_str(int type);
void ta_rigid_body_init(ta_rigid_body *body);
void ta_rigid_body_update(ta_rigid_body *body, double dt);
bool ta_intersect_sphere_vs_sphere(const ta_sphere *a, const ta_sphere *b,
    ta_manifold *manifold);
bool ta_rigid_body_intersect(ta_rigid_body *a, ta_rigid_body *b,
    ta_manifold *manifold);
void ta_rigid_body_resolve_collision(ta_manifold *manifold);
//void ta_rigid_body_update(ta_rigid_body *rigid_body, double dt);