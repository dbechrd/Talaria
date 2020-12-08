#include "ta_intersect.h"
#include "ta_collider.h"
#include "ta_primitive.h"
#include <float.h>
#include <math.h>

bool ta_ray_v_sphere(const ta_ray *ray, const ta_sphere *sphere, float *t_intersect)
{
    ta_vec3 L = vec3_sub(sphere->center, ray->origin);
    float tca = vec3_dot(L, ray->direction);
    // if (tca < 0) return false;

    float d2 = vec3_dot(L, L) - tca * tca;
    float r2 = sphere->radius * sphere->radius;
    if (d2 > r2) {
        return false;
    }

    float thc = sqrtf(r2 - d2);
    float t0 = tca - thc;
    float t1 = tca + thc;

    if (t0 > t1) {
        swap_r32(&t0, &t1);
    }

    if (t0 < 0) {
        t0 = t1; // if t0 is negative, let's use t1 instead
        if (t0 < 0) {
            return false; // both t0 and t1 are negative
        }
    }

    if (t_intersect) {
        *t_intersect = t0;
    }
    return true;
}

bool ta_ray_v_plane(const ta_ray *ray, const ta_plane *plane, float *t_intersect)
{
    //--------------------------------------------------------------------------
    //| Random notes:
    //|
    //| float a = plane->normal.x;
    //| float b = plane->normal.y;
    //| float c = plane->normal.z;
    //| float d = vec3_dot(plane->normal, plane->center);
    //| ta_vec3 ro = ray->origin;
    //| ta_vec3 rd = ray->direction;
    //|
    //| Component-wise derivation before I realized these were just dot products
    //|
    //| Ax + By + Cz = D
    //| A(ro.x + t*rd.x) + B(ro.y + "t"rd.y) + C(ro.z + t*rd.z) = D
    //| A*ro.x + A*t*rd.x + B*ro.y + B*t*rd.y + C*ro.z + C*t*rd.z = D
    //| A*t*rd.x + B*t*rd.y + C*t*rd.z = D - A*ro.x - B*ro.y - C*ro.z
    //| t * (A*rd.x + B*rd.y + C*rd.z) = D - A*ro.x - B*ro.y - C*ro.z
    //| t = (D - A*ro.x - B*ro.y - C*ro.z) / (A*rd.x + B*rd.y + C*rd.z)
    //|
    //| Observation: dot(n, a) - dot(n, b) == dot(n, a-b)
    //|
    //| float a = vec3_dot(plane->normal, plane->center);
    //| float b = vec3_dot(plane->normal, ray->origin);
    //| float c = vec3_dot(plane->normal, vec3_sub(plane->center, ray->origin));
    //| DLB_ASSERT(a - b == c);
    //--------------------------------------------------------------------------

    // NOTE: t is the interval of the ray-to-plane intersection as a percentage
    // of the ray's direction vector
    ta_vec3 r = vec3_sub(plane->center, ray->origin);
    ta_vec3 d = ray->direction;

    float nr = vec3_dot(plane->normal, r);
    float nd = vec3_dot(plane->normal, d);
    if (nd == 0.0f) {
        return false;  // ray parallel to plane
    }
    float t = nr / nd;
    if (t < 0.0f) {
        return false;  // ray pointing away from plane
    }

    // If caller cares about intersection time, populate it
    if (t_intersect) {
        *t_intersect = t;
    }

    // ray intersects plane at:
    // vec3_add(ray->origin, vec3_scalef(ray->direction, t));
    return true;
}

bool ta_ray_v_quad(const ta_ray *ray, const ta_quad *quad, float *t_intersect)
{
    // Ray in quad local space
    ta_vec4 orient_inv = quat_inverse(quad->orientation);
    ta_ray ray_local = { 0 };
    ray_local.origin = quat_mul_vec3(orient_inv, vec3_sub(ray->origin, quad->center));
    ray_local.direction = quat_mul_vec3(orient_inv, ray->direction);

#if 0
    // TODO: Cleanup debug code
    ta_quad local = { 0 };
    local.extents = quad->extents;
    local.orientation.w = 1.0f;
    ta_primitive_push_quad(0, local, TA_COLOR_DARK_GREENA);
    ta_primitive_push_arrow(0, ray_local.origin, ray_local.direction, TA_COLOR_GREEN);
#endif

    // NOTE: Simplified version of ray_v_plane because we only care about z in
    // quad local space.
    float nr = -ray_local.origin.z;
    float nd = ray_local.direction.z;
    if (nd == 0.0f) {
        return false;  // ray parallel to plane
    }
    float t = nr / nd;
    if (t < 0.0f) {
        return false;  // ray pointing away from plane
    }

    ta_vec3 intersect = vec3_add(ray_local.origin,
        vec3_scalef(ray_local.direction, t));
    if (fabs(intersect.x) <= quad->extents.x + TA_EPSILON &&
        fabs(intersect.y) <= quad->extents.y + TA_EPSILON)
    {
        // If caller cares about intersection time, populate it
        if (t_intersect) {
            *t_intersect = t;
        }
        return true;
    }
    return false;
}

bool ta_ray_v_obb(const ta_ray *ray, const ta_obb *obb, float *t_intersect)
{
    // Ray in obb local space
    ta_vec4 orient_inv = quat_inverse(obb->orientation);
    ta_ray ray_local = { 0 };
    ray_local.origin = quat_mul_vec3(orient_inv, vec3_sub(ray->origin, obb->center));
    ray_local.direction = quat_mul_vec3(orient_inv, ray->direction);

    // Calculate plane for each face of OBB
    ta_plane planes[6] = { 0 };
    planes[0].center.x += obb->extents.x;
    planes[1].center.y += obb->extents.y;
    planes[2].center.z += obb->extents.z;
    planes[3].center.x -= obb->extents.x;
    planes[4].center.y -= obb->extents.y;
    planes[5].center.z -= obb->extents.z;
    planes[0].normal.x = obb->extents.x;
    planes[1].normal.y = obb->extents.y;
    planes[2].normal.z = obb->extents.z;
    planes[3].normal.x = -obb->extents.x;
    planes[4].normal.y = -obb->extents.y;
    planes[5].normal.z = -obb->extents.z;

    float t_min = FLT_MAX;
    float t = 0.0f;
    ta_vec3 intersect;

    for (int i = 0; i < 6; ++i) {
        planes[i].normal = vec3_normalize(planes[i].normal);

        if (ta_ray_v_plane(&ray_local, &planes[i], &t)) {
            intersect = vec3_add(ray_local.origin, vec3_scalef(ray_local.direction, t));
            if (fabs(intersect.x) <= obb->extents.x + TA_EPSILON &&
                fabs(intersect.y) <= obb->extents.y + TA_EPSILON &&
                fabs(intersect.z) <= obb->extents.z + TA_EPSILON)
            {
                t_min = MIN(t_min, t);
            }
        }
    }

#if 0
    ta_line_3d ray_line = { 0 };
    ray_line.p0 = ray->origin;
    ray_line.p1 = vec3_add(ray->origin, ray->direction);
    ta_primitive_push_line_3d(ray_line, TA_COLOR_WHITE, TA_COLOR_RED);
    ta_primitive_push_obb(*obb, TA_COLOR_RED);

    ta_line_3d ray_local_line = { 0 };
    ray_local_line.p0 = ray_local.origin;
    ray_local_line.p1 = vec3_add(ray_local.origin, ray_local.direction);
    ta_primitive_push_line_3d(ray_local_line, TA_COLOR_WHITE, TA_COLOR_GREEN);

    float radius = 0.2f;
    ta_primitive_push_plane(planes[0], radius, TA_COLOR_RED);
    ta_primitive_push_plane(planes[1], radius, TA_COLOR_GREEN);
    ta_primitive_push_plane(planes[2], radius, TA_COLOR_BLUE);
    ta_primitive_push_plane(planes[3], radius, TA_COLOR_CYAN);
    ta_primitive_push_plane(planes[4], radius, TA_COLOR_MAGENTA);
    ta_primitive_push_plane(planes[5], radius, TA_COLOR_YELLOW);
#endif

    if (t_min < FLT_MAX) {
        if (t_intersect) {
            *t_intersect = t_min;
        }
        return true;
    }
    return false;
}

bool ta_ray_v_aabb(const ta_ray *ray, const ta_aabb *aabb, float *t_intersect)
{
    ta_obb obb = { 0 };
    obb.center = aabb->center;
    obb.extents = aabb->extents;
    obb.orientation = QUAT_IDENT;
    return ta_ray_v_obb(ray, &obb, t_intersect);
}

// TODO: Handle generating manifolds for AABBs. For now, just allow this to be
//       used for broadphase collision detection.
bool ta_aabb_v_aabb(const ta_aabb *a, const ta_aabb *b)
{
    DLB_ASSERT(a);
    DLB_ASSERT(b);

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

bool ta_plane_v_sphere(ta_manifold *manifold, const ta_plane *plane, const ta_sphere *sphere)
{
    DLB_ASSERT(plane);
    DLB_ASSERT(sphere);

    float r = sphere->radius;
    float d = vec3_dot(vec3_sub(sphere->center, plane->center), plane->normal);
    if (d > r) {
        return false;
    }

    if (manifold) {
        manifold->normal_world = plane->normal;
        ta_vec3 sphere_contact = vec3_sub(sphere->center, vec3_scalef(manifold->normal_world, r));
        manifold->contacts[0].ca_world = vec3_add(sphere_contact, vec3_scalef(manifold->normal_world, r - d));
        manifold->contacts[0].cb_world = sphere_contact;
        manifold->contacts[0].rb_world = vec3_sub(manifold->contacts[0].cb_world, sphere->center);
        manifold->contacts[0].ra_world = vec3_neg(manifold->contacts[0].rb_world);
        manifold->contact_count = 1;
    }

    return true;
}

bool ta_plane_v_obb(ta_manifold *manifold, const ta_plane *plane, const ta_obb *obb)
{
    DLB_ASSERT(plane);
    DLB_ASSERT(obb);

    // Calculate the 8 corners of the OBB
    ta_vec3 p[8] = { 0 };
    p[0].x = -obb->extents.x; p[0].y = -obb->extents.y; p[0].z = -obb->extents.z;
    p[1].x = -obb->extents.x; p[1].y = -obb->extents.y; p[1].z = +obb->extents.z;
    p[2].x = -obb->extents.x; p[2].y = +obb->extents.y; p[2].z = -obb->extents.z;
    p[3].x = -obb->extents.x; p[3].y = +obb->extents.y; p[3].z = +obb->extents.z;
    p[4].x = +obb->extents.x; p[4].y = -obb->extents.y; p[4].z = -obb->extents.z;
    p[5].x = +obb->extents.x; p[5].y = -obb->extents.y; p[5].z = +obb->extents.z;
    p[6].x = +obb->extents.x; p[6].y = +obb->extents.y; p[6].z = -obb->extents.z;
    p[7].x = +obb->extents.x; p[7].y = +obb->extents.y; p[7].z = +obb->extents.z;
    for (int i = 0; i < 8; ++i) {
        p[i] = quat_mul_vec3(obb->orientation, p[i]);
        p[i] = vec3_add(p[i], obb->center);
    }

    // TODO: This doesn't settle anything with > 1kg of mass.. so we need to do
    // a bit more work to handle sliding due to friction on heavier objects.
    const float tolerance = 0.002f;

    float dists[8];
    float d_min = FLT_MAX / 2.0f;
    for (int i = 0; i < 8; ++i) {
        ta_vec3 n = vec3_sub(p[i], plane->center);
        float d = vec3_dot(n, plane->normal);
        if (d < tolerance) {
            dists[i] = d;
            d_min = (d < d_min) ? d : d_min;
        } else {
            dists[i] = -FLT_MAX / 2.0f;
        }
    }

    bool collided = false;
    for (int i = 0; i < 8; ++i) {
        if (fabs(dists[i] - d_min) <= tolerance) {
            if (manifold) {
                manifold->normal_world = plane->normal;
                manifold->contacts[manifold->contact_count].ca_world = vec3_add(p[i], vec3_scalef(manifold->normal_world, -dists[i]));
                manifold->contacts[manifold->contact_count].cb_world = p[i];
                manifold->contacts[manifold->contact_count].rb_world = vec3_sub(manifold->contacts[0].cb_world, obb->center);
                manifold->contacts[manifold->contact_count].ra_world = vec3_neg(manifold->contacts[0].rb_world);
                manifold->contact_count++;
            }
            collided = true;
        }
    }

    if (collided && manifold) {
        manifold->normal_world = plane->normal;
    }

    return collided;
}

bool ta_sphere_v_sphere(ta_manifold *manifold, const ta_sphere *a, const ta_sphere *b)
{
    DLB_ASSERT(a);
    DLB_ASSERT(b);

    // http://media.steampowered.com/apps/valve/2015/DirkGregorius_Contacts.pdf

    float r = a->radius + b->radius;
    ta_vec3 dp = vec3_sub(b->center, a->center);

    // delta position > sum of radii
    float dp_len_sq = vec3_len2(dp);
    if (dp_len_sq > r * r) {
        return false;
    }

    if (manifold) {
        float dp_len = sqrtf(dp_len_sq);
        if (dp_len) {
            // normalize delta position
            manifold->normal_world = vec3_scalef(dp, 1.0f / dp_len);
        } else {
            // Edge case: Circles at same position, arbitrarily point normal up
            manifold->normal_world = VEC3_Y;
        }
        // penetation distance
        float d = dp_len - r;

        // calculate contact points
        manifold->contacts[0].ca_world = vec3_add(a->center, vec3_scalef(manifold->normal_world,  a->radius));
        manifold->contacts[0].cb_world = vec3_add(b->center, vec3_scalef(manifold->normal_world, -b->radius));
        manifold->contacts[0].ra_world = vec3_sub(manifold->contacts[0].ca_world, a->center);
        manifold->contacts[0].rb_world = vec3_sub(manifold->contacts[0].cb_world, b->center);
        manifold->contact_count = 1;
    }

    return true;
}

bool ta_sphere_v_obb(ta_manifold *manifold, const ta_sphere *sphere, const ta_obb *obb)
{
    DLB_ASSERT(sphere);
    DLB_ASSERT(obb);

#if 0
    // Calculate the 8 corners of the OBB (world space)
    ta_vec3 p[8] = { 0 };
    p[0].x = -obb->extents.x; p[0].y = -obb->extents.y; p[0].z = -obb->extents.z;
    p[1].x = -obb->extents.x; p[1].y = -obb->extents.y; p[1].z = +obb->extents.z;
    p[2].x = -obb->extents.x; p[2].y = +obb->extents.y; p[2].z = -obb->extents.z;
    p[3].x = -obb->extents.x; p[3].y = +obb->extents.y; p[3].z = +obb->extents.z;
    p[4].x = +obb->extents.x; p[4].y = -obb->extents.y; p[4].z = -obb->extents.z;
    p[5].x = +obb->extents.x; p[5].y = -obb->extents.y; p[5].z = +obb->extents.z;
    p[6].x = +obb->extents.x; p[6].y = +obb->extents.y; p[6].z = -obb->extents.z;
    p[7].x = +obb->extents.x; p[7].y = +obb->extents.y; p[7].z = +obb->extents.z;
    for (int i = 0; i < 8; ++i) {
        p[i] = quat_mul_vec3(obb->orientation, p[i]);
        p[i] = vec3_add(p[i], obb->center);
    }

    // Sphere center in OBB local space
    ta_vec3 sphere_center_obb = vec3_sub(sphere->center, obb->center);
    ta_vec4 obb_orient_inv = quat_inverse(obb->orientation);
    sphere_center_obb = quat_mul_vec3(obb_orient_inv, sphere_center_obb);

    // Check easy separating axes (early out)
    if (fabs(sphere_center_obb.x) - sphere->radius > obb->extents.x ||
        fabs(sphere_center_obb.y) - sphere->radius > obb->extents.y ||
        fabs(sphere_center_obb.z) - sphere->radius > obb->extents.z)
    {
        return false;
    }

    // Find closest point on obb to sphere's center
    ta_vec3 closest_obb = { 0 };
    closest_obb.x = clampf(sphere_center_obb.x, -obb->extents.x, obb->extents.x);
    closest_obb.y = clampf(sphere_center_obb.y, -obb->extents.y, obb->extents.y);
    closest_obb.z = clampf(sphere_center_obb.z, -obb->extents.z, obb->extents.z);

    float d2 = vec3_len2(vec3_sub(closest_obb, sphere_center_obb));
    float r2 = sphere->radius * sphere->radius;
    if (d2 > r2) {
        return false;
    }

    // COLLISION!
    if (manifold) {
        ta_vec3 closest = quat_mul_vec3(obb->orientation, closest_obb);
        closest = vec3_add(closest, obb->center);
        ta_vec3 normal = vec3_sub(closest, sphere->center);
        // Edge case: Objects at same position, arbitrarily point normal up
        if (vec3_zero(normal)) {
            normal = VEC3_Y;
        }
        manifold->depth = sphere->radius - sqrtf(d2);
        manifold->normal = vec3_normalize(normal);
        manifold->contact_count = 1;
        manifold->contacts[0].world = closest;
    }

    return true;
#else
    UNUSED(manifold);
    return false;
#endif
}