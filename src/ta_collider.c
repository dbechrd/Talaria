#include "ta_collider.h"
#include "ta_primitive.h"
#include "ta_transform.h"
#include <math.h>

void ta_collider_init(ta_collider *collider)
{
    switch (collider->type) {
        case TA_COLLIDER_PLANE: {
            if (vec3_len(collider->data.plane.normal)) {
                collider->data.plane.normal = vec3_normalize(collider->data.plane.normal);
            }
            break;
        } case TA_COLLIDER_SPHERE: {
            DLB_ASSERT(collider->data.sphere.radius > TA_EPSILON);
            break;
        } case TA_COLLIDER_OBB: {
            DLB_ASSERT(collider->data.obb.extents.x > TA_EPSILON);
            DLB_ASSERT(collider->data.obb.extents.y > TA_EPSILON);
            DLB_ASSERT(collider->data.obb.extents.z > TA_EPSILON);
            collider->data.obb.orientation = quat_normalize(collider->data.obb.orientation);
            break;
        } default: {
            DLB_ASSERT(!"Make sure this collider doesn't need initializer");
        }
    }
}

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

static ta_mat3 sphere_inverse_tensor(ta_sphere *sphere, float mass)
{
    ta_mat3 inv_tensor = { 0 };

    // Sphere moment of inertia: 2/5 MR^2
    float moment = 2.0f/5.0f * mass * (sphere->radius * sphere->radius);
    float inv_moment = 1.0f / moment;
    inv_tensor = mat3_init(
        inv_moment, 0.0f,       0.0f,
        0.0f,       inv_moment, 0.0f,
        0.0f,       0.0f,       inv_moment
    );

    return inv_tensor;
}

static ta_mat3 obb_inverse_tensor(ta_obb *obb, float mass)
{
    ta_mat3 inv_tensor = { 0 };

    // TODO: Proper OBB tensor calculation (differs along each axis)
    // https://en.wikipedia.org/wiki/List_of_moments_of_inertia

    // HACK: Cube moment of inertia: 1/6 ms^2 (where s = side length)
    float side = obb->extents.x;
    float moment = 1.0f/12.0f * mass * (side * side + side * side);
    float inv_moment = 1.0f / moment;
    inv_tensor = mat3_init(
        inv_moment, 0.0f,       0.0f,
        0.0f,       inv_moment, 0.0f,
        0.0f,       0.0f,       inv_moment
    );

    return inv_tensor;
}

ta_mat3 ta_collider_inv_tensor(ta_collider *collider, float mass)
{
    ta_mat3 inv_tensor = { 0 };
    if (fabs(mass) < TA_EPSILON) {
        return inv_tensor;
    }

    switch (collider->type) {
        case TA_COLLIDER_SPHERE: {
            inv_tensor = sphere_inverse_tensor(&collider->data.sphere, mass);
            break;
        } case TA_COLLIDER_OBB: {
            // HACK: Use sphere tensor temporarily
            inv_tensor = sphere_inverse_tensor(&collider->data.sphere, mass);
            //inv_tensor = obb_inverse_tensor(&collider->data.obb, mass);
            break;
        } default: {
            //DLB_ASSERT(!"You can't do that for this shape");
            break;
        }
    }
    return inv_tensor;
}

static ta_aabb plane_world_bounds(ta_plane *plane, ta_xform *xform)
{
    // TODO: Calculate AABB for plane (add TA_EPSILON depth)
    // or .. infinite AABB??
    ta_aabb result = { 0 };
    result.center = vec3_rotate_quat(plane->center, xform->orientation);
    result.center = vec3_add(result.center, xform->position);
    result.extents = VEC3_ONE;
    return result;
}

static ta_aabb sphere_world_bounds(ta_sphere *sphere, ta_xform *xform)
{
    DLB_ASSERT(sphere->radius > TA_EPSILON);

    ta_aabb result = { 0 };
    result.center = vec3_rotate_quat(sphere->center, xform->orientation);
    result.center = vec3_add(result.center, xform->position);
    result.extents.x = sphere->radius;
    result.extents.y = sphere->radius;
    result.extents.z = sphere->radius;
    return result;
}

static ta_aabb obb_world_bounds(ta_obb *obb, ta_xform *xform)
{
    DLB_ASSERT(obb->extents.x > TA_EPSILON);
    DLB_ASSERT(obb->extents.y > TA_EPSILON);
    DLB_ASSERT(obb->extents.z > TA_EPSILON);

    ta_aabb result = { 0 };
    result.center = vec3_rotate_quat(obb->center, xform->orientation);
    result.center = vec3_add(result.center, xform->position);

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
        p[i] = vec3_rotate_quat(p[i], xform->orientation);
        //p[i] = vec3_add(p[i], obb->center);

        result.extents.x = (float)MAX(result.extents.x, fabs(p[i].x));
        result.extents.y = (float)MAX(result.extents.y, fabs(p[i].y));
        result.extents.z = (float)MAX(result.extents.z, fabs(p[i].z));
    }

    return result;
}

ta_aabb ta_collider_world_bounds(ta_collider *collider, ta_xform *xform)
{
    ta_aabb result = { 0 };
    switch (collider->type) {
        case TA_COLLIDER_PLANE: {
            result = plane_world_bounds(&collider->data.plane, xform);
            break;
        } case TA_COLLIDER_SPHERE: {
            result = sphere_world_bounds(&collider->data.sphere, xform);
            break;
        } case TA_COLLIDER_OBB: {
            result = obb_world_bounds(&collider->data.obb, xform);
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
        } case TA_COLLIDER_OBB: {
            ta_primitive_push_obb(collider->data.obb, color);
            break;
        } default: {
            DLB_ASSERT(!"Don't know how to render this collider type");
            break;
        }
    }
}