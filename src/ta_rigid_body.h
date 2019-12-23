#pragma once
#include "ta_uid.h"
#include "ta_math.h"
#include "ta_collider.h"
#include "dlb/dlb_types.h"

// http://allenchou.net/2013/12/game-physics-introduction/
// https://gamedevelopment.tutsplus.com/tutorials/how-to-create-a-custom-2d-physics-engine-the-core-engine--gamedev-7493
typedef struct ta_rigid_body {
    u32 index;
    const char *name;
    const char *entity_name;
    ta_aabb aabb;
    ta_collider collider;

    // David's state variables
    // http://www.cs.cmu.edu/~baraff/sigcourse/notesd1.pdf p. D16
#if 0
    // Constant quantities
    float mass;        // M: mass
    ta_mat3 Ibody,     // Ibody
            Ibodyinv;  // I-1 body (inverse of Ibody)

    // State variables
    ta_vec3 x;     // x(t): position
    quaternion q;  // q(t): rotation
    ta_vec3 P,     // P(t): linear momentum (P = mv) (v = P * mass_inv)
            L;     // L(t): angular momentum (L = Iw) (w = L * I_inv)

    // Derived quantities (auxiliary variables)
    ta_mat3 Iinv;    // I-1(t): I-1 global (global inverse of Ibody)
    ta_vec3 v,      // v(t): linear velocity
            omega;  // w(t): angular velocity

    // Computed quantities
    ta_vec3 force,   // F(t): sum of applied forces
            torque;  // t(t): sum of applied torques
#endif

    float mass;
    float inv_mass;

    //ta_mat3 tensor;
    ta_mat3 inv_tensor_global;
    ta_mat3 inv_tensor_local;
    ta_vec4 tensor_orientation;

    ta_vec3 centroid_global;
    ta_vec3 centroid_local;

    // TODO: Relative offset (position/orientation) from transform component
    // for asymmetric collider types.
    //ta_xform offset;

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
    float e;  // Restitution

    float ks;  // Coefficient of static friction
    float kd;  // Coefficient of dynamic friction

    bool resting;

    bool trigger;     // If true, don't resolve positions
    bool no_gravity;  // If true, gravity will not affect this body
    //float gravity_scale;   // Is this useful?
    //u32 collision_groups;  // Bit flags; "layers"
} ta_rigid_body;

typedef struct ta_rigid_body_pair {
    ta_rigid_body *a;
    ta_rigid_body *b;
} ta_rigid_body_pair;

typedef struct ta_manifold {
    ta_rigid_body *a;
    ta_rigid_body *b;
    struct ta_transform *atrans;
    struct ta_transform *btrans;
    ta_vec3 normal;
    float depth;
    ta_vec3 contacts[1];  // world position
    u32 contact_count;
    float e;   // Mixed restitution
    float df;  // Mixed dynamic friction
    float sf;  // Mixed static friction
} ta_manifold;

const char *ta_collider_type_str(int type);
void ta_rigid_body_init(ta_rigid_body *body);
void ta_rigid_body_apply_force(ta_rigid_body *body, ta_vec3 force);
void ta_rigid_body_apply_force_at(ta_rigid_body *body, ta_vec3 force, ta_vec3 at);
void ta_rigid_body_apply_impulse(ta_rigid_body *body, ta_vec3 impulse, ta_vec3 at);
void ta_rigid_body_update(ta_rigid_body *body, float dt);
bool ta_aabb_v_aabb(const ta_aabb *a, const ta_aabb *b, ta_manifold *manifold);
bool ta_sphere_v_sphere(const ta_sphere *a, const ta_sphere *b,
    ta_manifold *manifold);
bool ta_plane_v_sphere(const ta_plane *plane, const ta_sphere *sphere,
    ta_manifold *manifold);
bool ta_rigid_body_intersect(ta_rigid_body *a, ta_rigid_body *b,
    ta_manifold *manifold);
void ta_rigid_body_resolve_collision(ta_manifold *manifold);
