#include "ta_collider.h"
#include "ta_primitive.h"
#include "ta_transform.h"
#include <math.h>

const char *ta_collider_type_str(int type)
{
    switch(type) {
        case TA_COLLIDER_PLANE:   return "TA_COLLIDER_PLANE";
        case TA_COLLIDER_SPHERE:  return "TA_COLLIDER_SPHERE";
        case TA_COLLIDER_OBB:     return "TA_COLLIDER_OBB";
        case TA_COLLIDER_CAPSULE: return "TA_COLLIDER_CAPSULE";
        default: DLB_ASSERT(0);   return "TA_COLLIDER_???";
    }
}

void ta_collider_init(ta_collider *collider)
{
    switch (collider->type) {
        case TA_COLLIDER_PLANE: {
            if (vec3_zero(collider->data.plane.normal)) {
                collider->data.plane.normal = VEC3_Y;
            } else {
                collider->data.plane.normal = vec3_normalize(collider->data.plane.normal);
            }
            break;
        } case TA_COLLIDER_SPHERE: {
            if (collider->data.sphere.radius == 0.0f) {
                collider->data.sphere.radius = 1.0f;
            }
            collider->data.sphere.radius = MAX(TA_EPSILON, collider->data.sphere.radius);
            break;
        } case TA_COLLIDER_OBB: {
            if (vec3_zero(collider->data.obb.extents)) {
                collider->data.obb.extents = VEC3_ONE;
            }
            collider->data.obb.extents.x = MAX(TA_EPSILON, collider->data.obb.extents.x);
            collider->data.obb.extents.y = MAX(TA_EPSILON, collider->data.obb.extents.y);
            collider->data.obb.extents.z = MAX(TA_EPSILON, collider->data.obb.extents.z);

            if (vec4_zero(collider->data.obb.orientation)) {
                collider->data.obb.orientation = QUAT_IDENT;
            } else {
                collider->data.obb.orientation = quat_normalize(collider->data.obb.orientation);
            }
            break;
        } case TA_COLLIDER_CAPSULE: {
            if (vec3_zero(collider->data.capsule.up)) {
                collider->data.capsule.up = VEC3_Y;
            } else {
                collider->data.capsule.up = vec3_normalize(collider->data.capsule.up);
            }
            if (collider->data.capsule.half_h == 0.0f) {
                collider->data.capsule.half_h = 1.0f;
            }
            if (collider->data.capsule.radius == 0.0f) {
                collider->data.capsule.radius = 1.0f;
            }
            collider->data.capsule.half_h = MAX(TA_EPSILON, collider->data.capsule.half_h);
            collider->data.capsule.radius = MAX(TA_EPSILON, collider->data.capsule.radius);
            break;
        } default: {
            DLB_ASSERT(!"Make sure this collider doesn't need initializer");
        }
    }
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

#if 0
    // HACK: Cube moment of inertia: 1/6 ms^2 (where s = side length)
    float side = MAX(1.0f, obb->extents.x);
    float moment = 1.0f/6.0f * mass * (side * side);
    float inv_moment = 1.0f / moment;
    inv_tensor = mat3_init(
        inv_moment, 0.0f,       0.0f,
        0.0f,       inv_moment, 0.0f,
        0.0f,       0.0f,       inv_moment
    );
#else
    float mass_twelfth = mass / 12.0f;
    float width  = 2.0f * obb->extents.x;
    float height = 2.0f * obb->extents.y;
    float depth  = 2.0f * obb->extents.z;
    float moment_x = mass_twelfth * (width * width);
    float moment_y = mass_twelfth * (height * height);
    float moment_z = mass_twelfth * (depth * depth);
    inv_tensor = mat3_init(
        1.0f / moment_x,      0.0f      ,      0.0f      ,
             0.0f      , 1.0f / moment_y,      0.0f      ,
             0.0f      ,      0.0f      , 1.0f / moment_z
    );
#endif

    return inv_tensor;
}

static ta_mat3 capsule_inverse_tensor(ta_capsule *capsule, float mass)
{
    // HACK: Use bounding sphere inertia tensor cus I'm lazy and capsule intertia tensors are really convoluted
    // TODO: https://www.gamedev.net/tutorials/programming/math-and-physics/capsule-inertia-tensor-r3856/

    ta_sphere sphere = { 0 };
    sphere.center = capsule->center;
    sphere.radius = capsule->half_h + capsule->radius;

    ta_mat3 inv_tensor = sphere_inverse_tensor(&sphere, mass);
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
            inv_tensor = obb_inverse_tensor(&collider->data.obb, mass);
            break;
        } case TA_COLLIDER_CAPSULE: {
            inv_tensor = capsule_inverse_tensor(&collider->data.capsule, mass);
            break;
        } default: {
            //DLB_ASSERT(!"You can't do that for this shape");
            break;
        }
    }
    return inv_tensor;
}

static ta_aabb plane_world_bounds(ta_plane *plane, const ta_xform *xform)
{
    // TODO: Calculate AABB for plane (add TA_EPSILON depth)
    // or .. infinite AABB??
    ta_aabb result = { 0 };
    result.center = vec3_add(xform->position, quat_mul_vec3(xform->orientation, plane->center));
    result.extents = VEC3_ONE;
    return result;
}

static ta_aabb sphere_world_bounds(ta_sphere *sphere, const ta_xform *xform)
{
    DLB_ASSERT(sphere->radius >= TA_EPSILON);

    ta_aabb result = { 0 };
    result.center = vec3_add(xform->position, quat_mul_vec3(xform->orientation, sphere->center));
    result.extents.x = sphere->radius;
    result.extents.y = sphere->radius;
    result.extents.z = sphere->radius;
    return result;
}

static ta_aabb obb_world_bounds(ta_obb *obb, const ta_xform *xform)
{
    DLB_ASSERT(obb->extents.x >= TA_EPSILON);
    DLB_ASSERT(obb->extents.y >= TA_EPSILON);
    DLB_ASSERT(obb->extents.z >= TA_EPSILON);

    ta_aabb result = { 0 };
    result.center = quat_mul_vec3(xform->orientation, obb->center);
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
        p[i] = quat_mul_vec3(obb->orientation, p[i]);
        p[i] = quat_mul_vec3(xform->orientation, p[i]);
        //p[i] = vec3_add(p[i], obb->center);

        result.extents.x = (float)MAX(result.extents.x, fabs(p[i].x));
        result.extents.y = (float)MAX(result.extents.y, fabs(p[i].y));
        result.extents.z = (float)MAX(result.extents.z, fabs(p[i].z));
    }

    return result;
}

static ta_aabb capsule_world_bounds(ta_capsule *capsule, const ta_xform *xform)
{
    DLB_ASSERT(capsule->half_h >= TA_EPSILON);
    DLB_ASSERT(capsule->radius >= TA_EPSILON);
    DLB_ASSERT(!vec3_zero(capsule->up));

    ta_aabb result = { 0 };
    result.center = vec3_add(xform->position, quat_mul_vec3(xform->orientation, capsule->center));

    // Project inner segment onto primary axes then add radius of hemisphere
    ta_vec3 h = vec3_scalef(capsule->up, capsule->half_h);
    h = quat_mul_vec3(xform->orientation, h);
    result.extents.x = fabsf(vec3_dot(h, VEC3_X)) + capsule->radius;
    result.extents.y = fabsf(vec3_dot(h, VEC3_Y)) + capsule->radius;
    result.extents.z = fabsf(vec3_dot(h, VEC3_Z)) + capsule->radius;

    return result;
}

ta_aabb ta_collider_world_bounds(ta_collider *collider, const ta_xform *xform)
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
        } case TA_COLLIDER_CAPSULE: {
            result = capsule_world_bounds(&collider->data.capsule, xform);
            break;
        } default: {
            DLB_ASSERT(!"Don't know how to bound this collider type");
            break;
        }
    }
    return result;
}

void ta_collider_push(ta_collider *collider, ta_rgba color)
{
    switch (collider->type) {
        case TA_COLLIDER_PLANE: {
            // TODO: When would we ever actually need an infinite plane collider?
            // These should probably be OBBs or quads (mesh colliders) instead.
            ta_primitive_push_plane(0, collider->data.plane, 5.0f, color);
            break;
        } case TA_COLLIDER_SPHERE: {
            ta_primitive_push_sphere(0, collider->data.sphere, color);
            break;
        } case TA_COLLIDER_OBB: {
            ta_primitive_push_obb(0, collider->data.obb, color);
            break;
        } case TA_COLLIDER_CAPSULE: {
            ta_primitive_push_capsule(0, collider->data.capsule, color);
            break;
        } default: {
            DLB_ASSERT(!"Don't know how to render this collider type");
            break;
        }
    }
}