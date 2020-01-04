#pragma once
#include "ta_math.h"
#include "dlb/dlb_types.h"

struct ta_manifold;
struct ta_aabb;
struct ta_sphere;
struct ta_plane;

typedef struct ta_manifold {
    struct ta_rigid_body *a;
    struct ta_rigid_body *b;
    ta_vec3 normal;       // normal from a to b
    float depth;          // distance along normal
    ta_vec3 contacts[8];  // world position
    u32 contact_count;
    float e;   // Mixed restitution (0.0 = inelastic, 1.0 = perfectly elastic)
    float df;  // Mixed dynamic friction
    float sf;  // Mixed static friction
} ta_manifold;

bool ta_aabb_v_aabb(const struct ta_aabb *a, const struct ta_aabb *b);
bool ta_sphere_v_sphere(struct ta_manifold *manifold, const struct ta_sphere *a,
    const struct ta_sphere *b);
bool ta_plane_v_sphere(struct ta_manifold *manifold, const struct ta_plane *plane,
    const struct ta_sphere *sphere);
bool ta_plane_v_obb(struct ta_manifold *manifold, const struct ta_plane *plane,
    const struct ta_obb *obb);