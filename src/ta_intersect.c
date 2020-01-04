#include "ta_intersect.h"
#include "ta_collider.h"
#include <float.h>
#include <math.h>

// TODO: Handle generating manifolds for AABBs. For now, just allow this ti be
//       used for broadphase collision detection.
bool ta_aabb_v_aabb(const ta_aabb *a, const ta_aabb *b)
{
    bool collided = false;
    if (a->extents.x == 0.0f || b->extents.x == 0.0f || (
        a->center.x + a->extents.x > b->center.x - b->extents.x &&
        a->center.y + a->extents.y > b->center.y - b->extents.y &&
        a->center.z + a->extents.z > b->center.z - b->extents.z &&
        b->center.x + b->extents.x > a->center.x - a->extents.x &&
        b->center.y + b->extents.y > a->center.y - a->extents.y &&
        b->center.z + b->extents.z > a->center.z - a->extents.z))
    {
        collided = true;
    }
    return collided;
}

bool ta_sphere_v_sphere(ta_manifold *manifold, const ta_sphere *a,
    const ta_sphere *b)
{
    float r = a->radius + b->radius;
    ta_vec3 n = vec3_sub(b->center, a->center);

    float d2 = vec3_len2(n);
    if (d2 > r * r) {
        return false;
    }

    if (manifold) {
        float d = sqrtf(d2);
        if (d != 0) {
            manifold->depth = r - d;
            manifold->normal = vec3_scalef(n, 1.0f / d);
        } else {
            // Edge case: Circles at same position, arbitrarily point normal up
            manifold->depth = a->radius;
            manifold->normal = VEC3_Y;
        }
        manifold->contact_count = 1;
        manifold->contacts[0] = vec3_add(a->center,
            vec3_scalef(manifold->normal, manifold->depth));
    }

    return true;
}

bool ta_plane_v_sphere(ta_manifold *manifold, const ta_plane *plane,
    const ta_sphere *sphere)
{
    float r = sphere->radius;
    ta_vec3 n = vec3_sub(sphere->center, plane->center);

    float d = vec3_dot(n, plane->normal);
    if (d > r) {
        return false;
    }

    if (manifold) {
        manifold->depth = r - d;
        manifold->normal = plane->normal;
        manifold->contact_count = 1;
        manifold->contacts[0] = vec3_add(sphere->center,
            vec3_scalef(manifold->normal, -d));
    }

    return true;
}

bool ta_plane_v_obb(ta_manifold *manifold, const ta_plane *plane,
    const ta_obb *obb)
{
    ta_vec3 p[8] = { 0 };
    p[0].x = -obb->extents.x;
    p[0].y = -obb->extents.y;
    p[0].z = -obb->extents.z;
    p[1].x = -obb->extents.x;
    p[1].y = -obb->extents.y;
    p[1].z = +obb->extents.z;
    p[2].x = -obb->extents.x;
    p[2].y = +obb->extents.y;
    p[2].z = -obb->extents.z;
    p[3].x = -obb->extents.x;
    p[3].y = +obb->extents.y;
    p[3].z = +obb->extents.z;
    p[4].x = +obb->extents.x;
    p[4].y = -obb->extents.y;
    p[4].z = -obb->extents.z;
    p[5].x = +obb->extents.x;
    p[5].y = -obb->extents.y;
    p[5].z = +obb->extents.z;
    p[6].x = +obb->extents.x;
    p[6].y = +obb->extents.y;
    p[6].z = -obb->extents.z;
    p[7].x = +obb->extents.x;
    p[7].y = +obb->extents.y;
    p[7].z = +obb->extents.z;

    for (int i = 0; i < 8; ++i) {
        p[i] = vec3_rotate_quat(p[i], obb->orientation);
        p[i] = vec3_add(p[i], obb->center);
    }

    // HACK: This doesn't work for anything with > 1kg of mass.. so it's a
    // pretty useless hack.
    const float resting_fudge = 0.02f;

    float dists[8];
    float d_min = FLT_MAX / 2.0f;
    for (int i = 0; i < 8; ++i) {
        ta_vec3 n = vec3_sub(p[i], plane->center);
        float d = vec3_dot(n, plane->normal);
        if (d < resting_fudge) {
            dists[i] = d;
            d_min = (d < d_min) ? d : d_min;
        } else {
            dists[i] = -FLT_MAX / 2.0f;
        }
    }

    int contact_count = 0;
    for (int i = 0; i < 8; ++i) {
        if (fabs(dists[i] - d_min) <= resting_fudge) {
            if (manifold) {
                manifold->contacts[contact_count] =
                    vec3_add(p[i], vec3_scalef(manifold->normal, -dists[i]));
            }
            contact_count++;
        }
    }
    if (!contact_count) {
        return false;
    }

    if (manifold) {
        manifold->depth = -d_min;
        manifold->normal = plane->normal;
        manifold->contact_count = contact_count;
    }

    return true;
}