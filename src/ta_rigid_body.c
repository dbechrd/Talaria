#include "ta_rigid_body.h"
#include "ta_entity.h"
#include "ta_log.h"
#include "dlb_vector.h"
#include <math.h>

const char *ta_collider_type_str(int type)
{
    switch(type) {
        case TA_COLLIDER_SPHERE: return "TA_COLLIDER_SHERE";
        case TA_COLLIDER_AABB:   return "TA_COLLIDER_AABB";
        case TA_COLLIDER_OBB:    return "TA_COLLIDER_OBB";
        default:
            DLB_ASSERT(!"<UNKNOWN_TA_COLLIDER_TYPE>");
            return 0;
    }
}

void ta_rigid_body_init(ta_rigid_body *body)
{
    if (vec4_zero(body->transform.rotation)) {
        body->transform.rotation = QUAT_IDENT;
    }
    if (vec3_zero(body->transform.scale)) {
        body->transform.scale = VEC3_ONE;
    }
    if (!body->mass) {
        body->mass = 1.0f;
    }
    body->inv_mass = 1.0f / body->mass;
    if (!body->restitution) {
        body->restitution = 0.5f;
    }
}

bool ta_intersect_sphere_vs_sphere(const ta_sphere *a, const ta_sphere *b,
    ta_manifold *manifold)
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
            // Edge case: Circles at same position
            manifold->depth = a->radius;
            manifold->normal = VEC3_X;
        }
    }

    return true;
}

bool ta_rigid_body_intersect(ta_rigid_body *a, ta_rigid_body *b,
    ta_manifold *manifold)
{
    // TODO: Rigid body scale??
    DLB_ASSERT(vec3_equal(a->transform.scale, VEC3_ONE));
    DLB_ASSERT(vec3_equal(b->transform.scale, VEC3_ONE));

    bool unknown = false;
    bool collided = false;

    switch (a->collider.type) {
        case TA_COLLIDER_SPHERE: {
            switch (a->collider.type) {
                case TA_COLLIDER_SPHERE: {
                    collided = ta_intersect_sphere_vs_sphere(
                        &a->collider.data.sphere, &b->collider.data.sphere,
                        manifold
                    );
                    break;
                } case TA_COLLIDER_AABB: {
                    break;
                } case TA_COLLIDER_OBB: {
                    break;
                } default: unknown = true;
            }
            break;
        } case TA_COLLIDER_AABB: {
            switch (a->collider.type) {
                case TA_COLLIDER_AABB: {
                    break;
                } case TA_COLLIDER_OBB: {
                    break;
                } default: unknown = true;
            }
            break;
        } case TA_COLLIDER_OBB: {
            switch (a->collider.type) {
                case TA_COLLIDER_OBB: {
                    break;
                } default: unknown = true;
            }
            break;
        } default: unknown = true;
    }

    if (manifold && collided) {
        manifold->a = a;
        manifold->b = b;
    }
    if (unknown) {
        // TODO: Log this. For now, just return false.
        //ta_log_write(tg_debug_log, "[Rigid Body] Unhandled collision pair.\n");
    }
    return collided;
}

void ta_rigid_body_resolve_collision(ta_manifold *manifold)
{
    ta_rigid_body *a = manifold->a;
    ta_rigid_body *b = manifold->b;

    // Relative velocity
    ta_vec3 dv = vec3_sub(b->velocity, a->velocity);

    // Relative velocity along normal
    float v_normal = vec3_dot(dv, manifold->normal);

    // Bodies moving apart, let it happen
    if (v_normal > 0.0f) {
        return;
    }

    // TODO: Restitution
    float e = MIN(a->restitution, b->restitution);

    // Impulse
    float j = -(1.0f + e) * v_normal;
    j /= a->inv_mass + b->inv_mass;
}

#if 0
void ta_rigid_body_resolve(ta_entity *entities)
{
    static int print = 5;

    ta_manifold manifold;

    if (print) ta_log_write(tg_debug_log, "[START] ta_rigid_body_resolve\n");
    dlb_vec_each(ta_entity *, a, entities) {
        if (a->invisible) continue;
        for (ta_entity *b = a + 1; b != dlb_vec_end(entities); b++) {
            if (b->invisible) continue;
            bool collide = ta_rigid_body_intersect(a, b, &manifold);
            if (print) {
                ta_log_write(tg_debug_log, "  '%s' v. '%s': %s\n",
                    a->uid, b->uid,
                    collide ? "true" : "false"
                );
            }
        }
    }
    if (print) ta_log_write(tg_debug_log, "[END]\n");
    if (print) print--;
}

void ta_rigid_body_update(ta_rigid_body *rigid_body, double dt)
{
    UNUSED(rigid_body);
    UNUSED(dt);
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
#endif