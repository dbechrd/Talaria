#pragma once
#include "ta_math.h"
#include "dlb/dlb_types.h"

struct ta_manifold;
struct ta_aabb;
struct ta_sphere;
struct ta_plane;

typedef struct ta_contact {
    ta_vec3 priv__ca_world;  // contact point on A in world space
    ta_vec3 priv__cb_world;  // contact point on B in world space
    ta_vec3 ra;     // contact point radius in centroid space of A
    ta_vec3 rb;     // contact point radius in centroid space of B
    float lambda;   // Lagrangian multiplier (must zero at start of every substep)
} ta_contact;

typedef struct ta_manifold {
    // TODO: Create a resource pool "lock" flag for known scopes where pointers are held for efficiency reasons
    // e.g. at beginning of game_simulate lock the rigid body pool, which allows manifolds to hold pointers, and
    // assert if anything tries to create or destroy something in that pool. Unlock after game_simulate destroys
    // the manifolds list.
    struct ta_rigid_body *a;  // TODO(DANGER): Storing a pointer, must guarantee no rigid bodies are created/destroyed!
    struct ta_rigid_body *b;  // TODO(DANGER): Storing a pointer, must guarantee no rigid bodies are created/destroyed!
    ta_vec3 normal_world;     // contact normal from a to b
    //float depth;            // contact magnitude in direction of normal
    u32 contact_count;
    ta_contact contacts[4]; // contact information
    float e;                // Mixed restitution (0.0 = inelastic, 1.0 = perfectly elastic)
    float coef_dynamic;     // Mixed dynamic friction
    float coef_static;      // Mixed static friction
} ta_manifold;

// NOTE: All of these shapes should be in world space, manifold is currently
// optional (if not provided, will just return true/false and not fill out
// contact properties).

bool ta_ray_v_sphere    (const ta_ray *ray, const ta_sphere *sphere, float *t_intersect);
bool ta_ray_v_plane     (const ta_ray *ray, const ta_plane *plane, float *t_intersect);
bool ta_ray_v_quad      (const ta_ray *ray, const ta_quad *quad, float *t_intersect);
bool ta_ray_v_obb       (const ta_ray *ray, const ta_obb *obb, float *t_intersect);
bool ta_ray_v_aabb      (const ta_ray *ray, const ta_aabb *aabb, float *t_intersect);
bool ta_aabb_v_aabb     (const struct ta_aabb *a, const struct ta_aabb *b);
bool ta_plane_v_sphere  (struct ta_manifold *manifold, const struct ta_plane *plane, const struct ta_sphere *sphere);
bool ta_plane_v_obb     (struct ta_manifold *manifold, const struct ta_plane *plane, const struct ta_obb *obb);
bool ta_sphere_v_sphere (struct ta_manifold *manifold, const struct ta_sphere *a, const struct ta_sphere *b);
bool ta_sphere_v_obb    (ta_manifold *manifold, const ta_sphere *sphere, const ta_obb *obb);