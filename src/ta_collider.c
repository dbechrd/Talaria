#include "ta_collider.h"
#include "ta_primitive.h"
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

void ta_collider_render(ta_collider *collider)
{
    switch (collider->type) {
        case TA_COLLIDER_PLANE: {
            // TODO: When would we ever actually need an infinite plane collider?
            // These should probably be OBBs or quads (mesh colliders) instead.
            ta_primitive_push_plane(collider->data.plane, 2.0f, TA_COLOR_CYAN);
            break;
        } case TA_COLLIDER_SPHERE: {
            ta_primitive_push_sphere(collider->data.sphere, TA_COLOR_CYAN);
            break;
        } case TA_COLLIDER_AABB: {
            ta_primitive_push_aabb(collider->data.aabb, TA_COLOR_CYAN);
            break;
        } case TA_COLLIDER_OBB: {
            //ta_primitive_push_obb(collider->data.aabb, TA_COLOR_CYAN);
            break;
        } default: {
            DLB_ASSERT(!"Don't know how to render this collider type");
            break;
        }
    }
}