#include "ta_rigid_body.h"
#include <math.h>

const char *ta_collider_type_str(int type)
{
    switch(type) {
        case TA_COLLIDER_AABB: return "TA_COLLIDER_AABB";
        case TA_COLLIDER_OBB:  return "TA_COLLIDER_OBB";
        default:
            DLB_ASSERT(!"<UNKNOWN_TA_COLLIDER_TYPE>");
            return 0;
    }
}

bool ta_intersect_sphere_vs_sphere(const ta_sphere *a, const ta_sphere *b)
{
    float r = a->radius + b->radius;
    float r2 = r*r;

    float dx = a->center.x - b->center.x;
    float dy = a->center.y - b->center.y;
    float dz = a->center.z - b->center.z;
    float d2 = dx*dx + dy*dy + dz*dz;

    return r2 < d2;
}

void ta_rigid_body_update(ta_rigid_body *rigid_body, double dt)
{
    // TODO: Handle other types of colliders
    if (rigid_body->collider.type != TA_COLLIDER_AABB) {
        return;
    }

    ta_vec3 center = vec3_add(rigid_body->transform.position,
        rigid_body->collider.data.aabb.center);
    ta_vec3 contact = center;
    contact.y -= rigid_body->collider.data.aabb.extents.y;

    const float ground_height = 0.0f;
    if (contact.y != ground_height) {
        if (fabs(contact.y) < TA_EPSILON) {
            contact.y = ground_height;
        } else {
            // TODO: Apply velocity impulse
            double dy = ground_height - contact.y;
            contact.y += (float)(dy * 0.98 * dt);
        }
    }

    rigid_body->transform.position = contact;
    rigid_body->transform.position.y += rigid_body->collider.data.aabb.extents.y;
}