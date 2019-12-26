#include "ta_collider.h"
#include "ta_primitive.h"
#include "ta_transform.h"
#include <math.h>

bool ta_intersect_ray_sphere(ta_ray ray, ta_sphere sphere, float *t)
{
    ta_vec3 L = vec3_sub(sphere.center, ray.origin);
    float tca = vec3_dot(L, ray.direction);
    // if (tca < 0) return false;

    float d2 = vec3_dot(L, L) - tca * tca;
    float r2 = sphere.radius * sphere.radius;
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

    if (t) {
        *t = t0;
    }
    return true;
}

static ta_aabb sphere_to_world_aabb(ta_sphere sphere, ta_xform *xform)
{
    ta_aabb result = { 0 };

    result.center = sphere.center;
    result.center = vec3_rotate_quat(result.center, xform->orientation);
    result.center = vec3_add(result.center, xform->position);

    result.extents.x = sphere.radius;
    result.extents.y = sphere.radius;
    result.extents.z = sphere.radius;

    return result;
}

static ta_aabb aabb_to_world_aabb(ta_aabb aabb, ta_xform *xform)
{
    // TODO: Stop being lazy and implement this
    ta_aabb result = { 0 };
    result.extents = VEC3_ONE;
    return result;
}

static ta_aabb obb_to_world_aabb(ta_obb obb, ta_xform *xform)
{
    // TODO: Stop being lazy and implement this
    ta_aabb result = { 0 };
    result.extents = VEC3_ONE;
    return result;
}

ta_aabb ta_collider_world_aabb(ta_collider *collider, ta_xform *xform)
{
    ta_aabb result = { 0 };

    switch (collider->type) {
        case TA_COLLIDER_PLANE: {
            // Return empty AABB, planes always pass broadphase for now
            result.extents = VEC3_ONE;
            break;
        } case TA_COLLIDER_SPHERE: {
            result = sphere_to_world_aabb(collider->data.sphere, xform);
            break;
        } case TA_COLLIDER_AABB: {
            result = aabb_to_world_aabb(collider->data.aabb, xform);
            break;
        } case TA_COLLIDER_OBB: {
            result = obb_to_world_aabb(collider->data.obb, xform);
            break;
        } default: {
            DLB_ASSERT(!"Don't know how to bound this collider type");
            break;
        }
    }
    return result;
}

void ta_collider_render(ta_collider *collider, ta_rgba color)
{
    switch (collider->type) {
        case TA_COLLIDER_PLANE: {
            // TODO: When would we ever actually need an infinite plane collider?
            // These should probably be OBBs or quads (mesh colliders) instead.
            ta_primitive_push_plane(collider->data.plane, 2.0f, color);
            break;
        } case TA_COLLIDER_SPHERE: {
            ta_primitive_push_sphere(collider->data.sphere, color);
            break;
        } case TA_COLLIDER_AABB: {
            ta_primitive_push_aabb(collider->data.aabb, color);
            break;
        } case TA_COLLIDER_OBB: {
            //ta_primitive_push_obb(collider->data.aabb, color);
            break;
        } default: {
            DLB_ASSERT(!"Don't know how to render this collider type");
            break;
        }
    }
}