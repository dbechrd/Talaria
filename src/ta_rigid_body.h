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

typedef struct {
    ta_vec3 center;
    ta_vec3 normal;
} ta_plane;

typedef enum {
    TA_COLLIDER_PLANE   = 0,
    TA_COLLIDER_SPHERE  = 1,
    TA_COLLIDER_AABB    = 2,
    TA_COLLIDER_OBB     = 3,
    TA_COLLIDER_COUNT
} ta_collider_type;

typedef struct {
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

// http://allenchou.net/2013/12/game-physics-introduction/
// https://gamedevelopment.tutsplus.com/tutorials/how-to-create-a-custom-2d-physics-engine-the-core-engine--gamedev-7493
typedef struct ta_rigid_body_s {
    ta_scene_ref ref;
    ta_collider collider;

    float mass;
    float inv_mass;
    //ta_mat3 tensor;
    ta_mat3 inv_tensor_global;
    ta_mat3 inv_tensor_local;

    ta_vec3 centroid_global;
    ta_vec3 centroid_local;

    ta_vec3 position;

    //ta_vec3 orientation_axis;
    ta_quat orientation;

    ta_vec3 velocity;
    ta_vec3 ang_velocity;

    ta_vec3 force_accum;
    ta_vec3 torque_accum;

    // Material data
    //   Rock       Density : 0.6  Restitution : 0.1
    //   Wood       Density : 0.3  Restitution : 0.2
    //   Metal      Density : 1.2  Restitution : 0.05
    //   BouncyBall Density : 0.3  Restitution : 0.8
    //   SuperBall  Density : 0.3  Restitution : 0.95
    //   Pillow     Density : 0.1  Restitution : 0.2
    //   Static     Density : 0.0  Restitution : 0.4
    float density;
    float restitution;

    float static_friction;
    float dynamic_friction;

    // float gravity_scale;     // Is this useful?
    //u32 collision_groups;       // Bit flags; "layers"
} ta_rigid_body;

typedef struct {
    ta_rigid_body *a;
    ta_rigid_body *b;
    ta_vec3 normal;
    float depth;
} ta_manifold;

const char *ta_collider_type_str(int type);
void ta_rigid_body_init(ta_rigid_body *body);
void ta_rigid_body_apply_force(ta_rigid_body *body, ta_vec3 force, ta_vec3 at);
void ta_rigid_body_update(ta_rigid_body *body, float dt);
bool ta_sphere_v_sphere(const ta_sphere *a, const ta_sphere *b,
    ta_manifold *manifold);
bool ta_plane_v_sphere(const ta_plane *plane, const ta_sphere *sphere,
    ta_manifold *manifold);
bool ta_rigid_body_intersect(const ta_rigid_body *a, const ta_rigid_body *b,
    ta_manifold *manifold);
void ta_rigid_body_resolve_collision(ta_manifold *manifold);
//void ta_rigid_body_update(ta_rigid_body *rigid_body, double dt);