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
    if (quat_zero(body->orientation)) {
        body->orientation = QUAT_IDENT;
    }

    // HACK: Assume collider center always first property
    body->collider.data.center = body->position;

    if (body->collider.type == TA_COLLIDER_SPHERE &&
        !body->collider.data.sphere.radius)
    {
        body->collider.data.sphere.radius = 1.0f;
    }
    if (body->mass != 0.0f) {
        body->inv_mass = 1.0f / body->mass;
    }
    if (!body->restitution) {
        body->restitution = 0.5f;
    }
}

void ta_rigid_body_apply_force(ta_rigid_body *body, ta_vec3 force, ta_vec3 at)
{
    // http://allenchou.net/2013/12/game-physics-motion-dynamics-implementations/
    body->force_accum = vec3_add(body->force_accum, force);
    //body->torque_accum = vec3_add(body->torque_accum,
    UNUSED(at);
}

void ta_rigid_body_update(ta_rigid_body *body, double dt)
{
    // TODO: Calculate this based on torque_accum and dt
    if (!vec3_equal(body->ang_velocity, VEC3_ZERO)) {
        ta_quat delta_orient = quat_from_axis_angle(
            vec3_normalize(body->ang_velocity),
            vec3_len(body->ang_velocity)
        );
        body->orientation = quat_mul(delta_orient, body->orientation);
    }

    ta_vec3 gravity = { 0.0f, -9.81f, 0.0f };
    gravity = vec3_scalef(gravity, (float)dt);
    ta_rigid_body_apply_force(body, gravity, VEC3_ZERO);

    ta_vec3 acc = vec3_scalef(body->force_accum, body->inv_mass);
    body->velocity = vec3_add(body->velocity, acc);
    body->position = vec3_add(body->position,
        vec3_scalef(body->velocity, (float)dt));

    // HACK: Collide with "ground" at y = 0
    if (body->position.y <= body->collider.data.sphere.radius) {
        body->velocity.y *= -0.5f;
        body->position.y = body->collider.data.sphere.radius;
    }
    body->ang_velocity = vec3_scalef(body->ang_velocity, 0.95f);
    body->velocity = vec3_scalef(body->velocity, 0.99f);

    // HACK: Assume collider center always first property
    body->collider.data.center = body->position;

    // Reset accumulators
    body->force_accum = VEC3_ZERO;
    body->torque_accum = VEC3_ZERO;
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
            manifold->normal = VEC3_Y;
        }
    }

    return true;
}

bool ta_rigid_body_intersect(ta_rigid_body *a, ta_rigid_body *b,
    ta_manifold *manifold)
{
    bool unknown = false;
    bool collided = false;

    // TODO: Replace with jump table. Find MIN(collider.type) then use that one
    //       first to avoid needing duplicate methods (e.g. AABB_OBB / OBB_AABB)
    switch (a->collider.type) {
        case TA_COLLIDER_SPHERE: {
            switch (a->collider.type) {
                case TA_COLLIDER_SPHERE: {
                    collided = ta_intersect_sphere_vs_sphere(
                        &a->collider.data.sphere, &b->collider.data.sphere,
                        manifold);
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

    // If bodies moving apart, let it happen
    if (v_normal < 0.0f) {
        // TODO: Restitution
        float e = MIN(a->restitution, b->restitution);

        // Impulse
        float j = -(1.0f + e) * v_normal;
        j /= a->inv_mass + b->inv_mass;

        // Apply inpulse
        // HACK: The "at" vector is only valid for spheres right now, need to
        // calculate contact points properly for other types of colliders.
        ta_vec3 impulse = vec3_scalef(manifold->normal, j);

        ta_rigid_body_apply_force(a, vec3_negate(impulse), vec3_scalef(vec3_negate(manifold->normal), a->collider.data.sphere.radius));
        ta_rigid_body_apply_force(b, impulse,              vec3_scalef(manifold->normal, b->collider.data.sphere.radius));

#if 1
        ta_vec3 a_impulse = vec3_scalef(impulse, a->inv_mass);
        ta_vec3 b_impulse = vec3_scalef(impulse, b->inv_mass);
        //a->velocity = vec3_sub(a->velocity, a_impulse);
        //b->velocity = vec3_add(b->velocity, b_impulse);
#else
        // TODO: Use this when done debugging
        a->velocity = vec3_sub(a->velocity, vec3_scalef(impulse, a->inv_mass));
        b->velocity = vec3_add(b->velocity, vec3_scalef(impulse, b->inv_mass));
#endif

        ta_vec3 a_at = vec3_add(a->position, vec3_scalef(manifold->normal, a->collider.data.sphere.radius - manifold->depth));
        ta_vec3 b_at = vec3_add(b->position, vec3_scalef(manifold->normal, -(b->collider.data.sphere.radius - manifold->depth)));

        ta_vec3 a_impulse_global = vec3_add(a->position, a_impulse);
        ta_vec3 b_impulse_global = vec3_add(b->position, b_impulse);

        a->ang_velocity = vec3_cross(a_at, vec3_add(a->position, vec3_negate(impulse)));
        b->ang_velocity = vec3_cross(b_at, vec3_add(b->position, impulse));

        ta_sphere debug_a_at;
        debug_a_at.center = a_at;
        debug_a_at.radius = 0.1f;
        ta_primitive_push_sphere(debug_a_at, TA_COLOR_RED);
        ta_sphere debug_b_at;
        debug_b_at.center = b_at;
        debug_b_at.radius = 0.1f;
        ta_primitive_push_sphere(debug_b_at, TA_COLOR_GREEN);
        ta_sphere debug_a_impulse;
        debug_a_impulse.center = a_impulse_global;
        debug_a_impulse.radius = 0.1f;
        ta_primitive_push_sphere(debug_a_impulse, TA_COLOR_BLUE);
        ta_sphere debug_b_impulse;
        debug_b_impulse.center = b_impulse_global;
        debug_b_impulse.radius = 0.1f;
        ta_primitive_push_sphere(debug_b_impulse, TA_COLOR_YELLOW);
    }
    DLB_ASSERT(1);

    // Randy's random Coloumb friction idea
    // https://gamedevelopment.tutsplus.com/tutorials/how-to-create-a-custom-2d-physics-engine-friction-scene-and-jump-table--gamedev-7756
#if 0
    // Re-calculate relative velocity after normal impulse
    // is applied (impulse from first article, this code comes
    // directly thereafter in the same resolve function)
    Vec2 rv = VB - VA

        // Solve for the tangent vector
        Vec2 tangent = rv - Dot( rv, normal ) * normal
        tangent.Normalize( )

        // Solve for magnitude to apply along the friction vector
        float jt = -Dot( rv, t )
        jt = jt / (1 / MassA + 1 / MassB)

        // PythagoreanSolve = A^2 + B^2 = C^2, solving for C given A and B
        // Use to approximate mu given friction coefficients of each body
        float mu = PythagoreanSolve( A->staticFriction, B->staticFriction )

        // Clamp magnitude of friction and create impulse vector
        Vec2 frictionImpulse
        if(abs( jt ) < j * mu)
            frictionImpulse = jt * t
        else
        {
            dynamicFriction = PythagoreanSolve( A->dynamicFriction, B->dynamicFriction )
                frictionImpulse = -j * t * dynamicFriction
        }

    // Apply
    A->velocity -= (1 / A->mass) * frictionImpulse
        B->velocity += (1 / B->mass) * frictionImpulse
#endif

#if 0
    // Positional correction
    const float percent = 1.0f;
    const float slop = TA_EPSILON;
    float c = MAX(manifold->depth - slop, 0.0f) / (a->inv_mass + b->inv_mass) * percent;
    ta_vec3 correction = vec3_scalef(manifold->normal, c);
    a->position = vec3_sub(a->position, vec3_scalef(correction, a->inv_mass));
    b->position = vec3_add(b->position, vec3_scalef(correction, b->inv_mass));
#endif
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

    ta_vec3 center = vec3_add(rigid_body->position,
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

    rigid_body->position = contact;
    rigid_body->position.y += rigid_body->collider.data.aabb.extents.y;
}
#endif