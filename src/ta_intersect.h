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
    ta_vec3 contacts[8];  // in world coordinates
    u32 contact_count;
    float e;   // Mixed restitution (0.0 = inelastic, 1.0 = perfectly elastic)
    float df;  // Mixed dynamic friction
    float sf;  // Mixed static friction
} ta_manifold;

// NOTE: All of these shapes should be in world space, manifold is currently
// optional (if not provided, will just return true/false and not fill out
// contact properties).

bool ta_ray_v_sphere(const ta_ray *ray, const ta_sphere *sphere, float *t_intersect);
bool ta_ray_v_plane(const ta_ray *ray, const ta_plane *plane, float *t_intersect);
bool ta_ray_v_quad(const ta_ray *ray, const ta_quad *quad, float *t_intersect);
bool ta_ray_v_obb(const ta_ray *ray, const ta_obb *obb, float *t_intersect);
bool ta_ray_v_aabb(const ta_ray *ray, const ta_aabb *aabb, float *t_intersect);
bool ta_aabb_v_aabb(const struct ta_aabb *a, const struct ta_aabb *b);
bool ta_plane_v_sphere(struct ta_manifold *manifold, const struct ta_plane *plane,
    const struct ta_sphere *sphere);
bool ta_plane_v_obb(struct ta_manifold *manifold, const struct ta_plane *plane,
    const struct ta_obb *obb);
bool ta_sphere_v_sphere(struct ta_manifold *manifold, const struct ta_sphere *a,
    const struct ta_sphere *b);
bool ta_sphere_v_obb(ta_manifold *manifold, const ta_sphere *sphere,
    const ta_obb *obb);