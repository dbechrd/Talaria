#include "ta_rigid_body.h"
#include "ta_entity.h"
#include "ta_log.h"
#include "dlb_vector.h"
#include <math.h>

typedef bool (intersector)(const ta_collider *a,
    const ta_collider *b, ta_manifold *manifold);

static intersector intersector_sphere_v_sphere;
static intersector intersector_plane_v_sphere;

static intersector *intersectors[TA_COLLIDER_COUNT][TA_COLLIDER_COUNT] = {
    [TA_COLLIDER_SPHERE][TA_COLLIDER_SPHERE] = intersector_sphere_v_sphere,
    [TA_COLLIDER_PLANE][TA_COLLIDER_SPHERE] = intersector_plane_v_sphere,
};

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
    body->collider.center_world = body->position;
    if (body->collider.type == 0 && !vec3_len(body->collider.data.plane.normal))
    {
        body->collider.type = TA_COLLIDER_SPHERE;
        body->collider.data.sphere.radius = 1.0f;
    }
    if (body->mass != 0.0f) {
        body->inv_mass = 1.0f / body->mass;
    }
    if (!body->restitution) {
        body->restitution = 0.1f;
    }
}

void ta_rigid_body_apply_force(ta_rigid_body *body, ta_vec3 force, ta_vec3 at)
{
    // http://allenchou.net/2013/12/game-physics-motion-dynamics-implementations/
    body->force_accum = vec3_add(body->force_accum, force);
    //body->torque_accum = vec3_add(body->torque_accum,
    UNUSED(at);
}

void ta_rigid_body_update(ta_rigid_body *body, float dt)
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
    ta_rigid_body_apply_force(body, gravity, VEC3_ZERO);

    ta_vec3 acc = vec3_scalef(body->force_accum, body->inv_mass);
    body->velocity = vec3_add(body->velocity, vec3_scalef(acc, dt));
    body->position = vec3_add(body->position,
        vec3_scalef(body->velocity, dt));
    body->collider.center_world = body->position;

    // HACK: Collide with "ground" at y = 0
    //if (body->position.y <= body->collider.data.sphere.radius) {
    //    body->velocity.y *= -0.5f;
    //    body->position.y = body->collider.data.sphere.radius;
    //}
    body->ang_velocity = vec3_scalef(body->ang_velocity, 0.95f);
    body->velocity = vec3_scalef(body->velocity, 0.99f);

    // Reset accumulators
    body->force_accum = VEC3_ZERO;
    body->torque_accum = VEC3_ZERO;
}

bool ta_sphere_v_sphere(const ta_sphere *a, const ta_sphere *b,
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

bool ta_plane_v_sphere(const ta_plane *plane, const ta_sphere *sphere,
    ta_manifold *manifold)
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
    }

    return true;
}

static bool intersector_sphere_v_sphere(const ta_collider *a,
    const ta_collider *b, ta_manifold *manifold)
{
    ta_sphere sphere_a;
    sphere_a.center = a->center_world;
    sphere_a.radius = a->data.sphere.radius;
    ta_sphere sphere_b;
    sphere_b.center = b->center_world;
    sphere_b.radius = b->data.sphere.radius;
    return ta_sphere_v_sphere(&sphere_a, &sphere_b, manifold);
}

static bool intersector_plane_v_sphere(const ta_collider *a,
    const ta_collider *b, ta_manifold *manifold)
{
    ta_plane plane;
    plane.center = a->center_world;
    plane.normal = a->data.plane.normal;
    ta_sphere sphere;
    sphere.center = b->center_world;
    sphere.radius = b->data.sphere.radius;
    return ta_plane_v_sphere(&plane, &sphere, manifold);
}

bool ta_rigid_body_intersect(const ta_rigid_body *a, const ta_rigid_body *b,
    ta_manifold *manifold)
{
    bool collided = false;

    ta_collider_type a_type = a->collider.type;
    ta_collider_type b_type = b->collider.type;
    intersector *intersect = a_type <= b_type
        ? intersectors[a_type][b_type]
        : intersectors[b_type][a_type];

    if (intersect) {
        collided = (*intersect)(&a->collider, &b->collider, manifold);
    } else {
        // TODO: Log this. For now, just return false.
        //ta_log_write(tg_debug_log, "[Rigid Body] Unhandled collision pair.\n");
    }

    if (collided && manifold) {
        manifold->a = (void *)a;
        manifold->b = (void *)b;
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

        //ta_rigid_body_apply_force(a, vec3_negate(impulse), vec3_scalef(vec3_negate(manifold->normal), a->collider.data.sphere.radius));
        //ta_rigid_body_apply_force(b, impulse,              vec3_scalef(manifold->normal, b->collider.data.sphere.radius));

#if 0
        a->velocity = vec3_sub(a->velocity, vec3_scalef(impulse, a->inv_mass));
        b->velocity = vec3_add(b->velocity, vec3_scalef(impulse, b->inv_mass));
#else
        ta_vec3 a_impulse = vec3_negate(vec3_scalef(impulse, a->inv_mass));
        ta_vec3 b_impulse = vec3_scalef(impulse, b->inv_mass);
        a->velocity = vec3_add(a->velocity, a_impulse);
        b->velocity = vec3_add(b->velocity, b_impulse);

        ta_vec3 a_impulse_global = vec3_add(a->position, a_impulse);
        ta_vec3 b_impulse_global = vec3_add(b->position, b_impulse);

        ta_vec3 a_at = vec3_add(a->position, vec3_scalef(manifold->normal, a->collider.data.sphere.radius - manifold->depth));
        ta_vec3 b_at = vec3_add(b->position, vec3_scalef(manifold->normal, -(b->collider.data.sphere.radius - manifold->depth)));

        if (a->inv_mass && b->inv_mass) {
            a->ang_velocity = vec3_cross(a_at, vec3_add(a->position, a_impulse));
            b->ang_velocity = vec3_cross(b_at, vec3_add(b->position, b_impulse));
        }

        //vec3_scalef(a->ang_velocity, 100.0f);
        //vec3_scalef(b->ang_velocity, 100.0f);

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
#endif
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

    // Positional correction
    const float percent = 1.0f;
    const float slop = TA_EPSILON;
    float c = MAX(manifold->depth - slop, 0.0f) / (a->inv_mass + b->inv_mass) * percent;
    ta_vec3 correction = vec3_scalef(manifold->normal, c);
    a->position = vec3_sub(a->position, vec3_scalef(correction, a->inv_mass));
    b->position = vec3_add(b->position, vec3_scalef(correction, b->inv_mass));
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