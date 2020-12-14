#pragma once
#include "ta_schema.h"
#include "ta_math.h"
#include "ta_collider.h"
#include "dlb/dlb_types.h"

struct ta_manifold;

// http://allenchou.net/2013/12/game-physics-introduction/
// https://gamedevelopment.tutsplus.com/tutorials/how-to-create-a-custom-2d-physics-engine-the-core-engine--gamedev-7493
typedef struct ta_rigid_body {
    TA_COMPONENT_HEADER
    ta_aabb     aabb;           // axis-aligned bounding box
    ta_collider collider;       // body collider

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

    //ta_mat3 tensor;
    ta_mat3 inv_tensor_local;         // TODO(perf): Define as diagonal matrix w.r.t. rest pose such that we can use vec3
    ta_mat3 priv__inv_tensor_world;   // NOTE: Don't access this directly, use rigid_body_inv_tensor_world()
    ta_vec4 priv__tensor_orientation; // NOTE: Don't access this; internal state

    ta_vec3 centroid_local;
    //ta_vec3 centroid_world;     // TODO(perf): Could cache this, but let's calculate it every time to avoid bugs.

    ta_xform xform;       // world space
    ta_xform xform_prev;  // world space

    ta_vec3 velocity;           // Meters per second
    ta_vec3 velocity_prev;
    ta_vec3 ang_velocity;       // Radians per second
    ta_vec3 ang_velocity_prev;

    //ta_vec3 acceleration;      // Don't ever change this directly, use force/impulse
    ta_vec3 force_accum;
    ta_vec3 torque_accum;

    bool resting;

    bool sensor;             // If true, don't resolve positions
    bool no_gravity;         // If true, gravity will not affect this body
    bool no_rotation;        // If true, this rigid body will not rotate due to physics
    //float gravity_scale;   // Is this useful?
    //u32 collision_groups;  // Bit flags; "layers"

    // TODO(cleanup): Random debug shit
    bool dbg_broadphase;
    bool dbg_narrowphase;

    const char **colliding_with;    // array of rigid bodies that are colliding with this one
} ta_rigid_body;

typedef struct ta_rigid_body_pair {
    ta_rigid_body *a;
    ta_rigid_body *b;
} ta_rigid_body_pair;

void ta_rigid_body_init                         (ta_rigid_body *body);
void ta_rigid_body_init_void                    (void *body);
void ta_rigid_body_free                         (ta_rigid_body *body);
void ta_rigid_body_free_void                    (void *body);

ta_vec3 rigid_body_local_to_world               (const ta_rigid_body *body, ta_vec3 p_body);
ta_vec3 rigid_body_local_to_world_prev          (const ta_rigid_body *body, ta_vec3 p_body);
ta_vec3 rigid_body_world_to_local               (const ta_rigid_body *body, ta_vec3 p_world);
ta_vec3 rigid_body_world_to_local_prev          (const ta_rigid_body *body, ta_vec3 p_world);

ta_vec3 rigid_body_oriented_vector              (const ta_rigid_body *body, ta_vec3 v_rest);
ta_vec3 rigid_body_rest_vector                  (const ta_rigid_body *body, ta_vec3 v_world);
ta_vec4 rigid_body_oriented_quaternion          (const ta_rigid_body *body, ta_vec4 q_rest);
ta_vec4 rigid_body_rest_quaternion              (const ta_rigid_body *body, ta_vec4 q_world);

ta_vec3 rigid_body_centroid_to_body             (const ta_rigid_body *body, ta_vec3 p_centroid);
ta_vec3 rigid_body_body_to_centroid             (const ta_rigid_body *body, ta_vec3 p_body);

ta_vec3 rigid_body_centroid_to_world            (const ta_rigid_body *body, ta_vec3 p_centroid);
ta_vec3 rigid_body_centroid_to_world_prev       (const ta_rigid_body *body, ta_vec3 p_centroid);
ta_vec3 rigid_body_world_to_centroid            (const ta_rigid_body *body, ta_vec3 p_world);
ta_vec3 rigid_body_world_to_centroid_prev       (const ta_rigid_body *body, ta_vec3 p_world);

ta_vec3 rigid_body_centroid_world               (const ta_rigid_body *body);
ta_vec3 rigid_body_centroid_oriented            (const ta_rigid_body *body);
const ta_mat3 *rigid_body_inv_tensor_world      (ta_rigid_body *body);

void ta_rigid_body_apply_force                  (ta_rigid_body *body, ta_vec3 force);
void ta_rigid_body_apply_force_at               (ta_rigid_body *body, ta_vec3 force, ta_vec3 at);
void ta_rigid_body_apply_impulse                (ta_rigid_body *body, ta_vec3 impulse, ta_vec3 contact_local);
void ta_rigid_body_apply_positional_correction  (ta_rigid_body *body, ta_vec3 impulse_world, ta_vec3 r_world);
void ta_physics_apply_position_correction       (ta_rigid_body *a, ta_rigid_body *b, ta_vec3 ra_local, ta_vec3 rb_local,
                                                 ta_vec3 dx_world, float alpha, float *lambda, float dt,
                                                 bool debug_render, ta_rgba debug_color);
void ta_rigid_body_apply_velocity_correction    (ta_rigid_body *body, ta_vec3 impulse_world, ta_vec3 r_world);
bool ta_rigid_body_intersect                    (struct ta_manifold *manifold, ta_rigid_body *a, ta_rigid_body *b);
void ta_rigid_body_resolve_collision            (struct ta_manifold *manifold, float dt);