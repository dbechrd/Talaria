#include "ta_rigid_body.h"
#include "ta_log.h"
#include "ta_primitive.h"
#include "ta_transform.h"
#include "ta_game.h"
#include "ta_schema.h"
#include "dlb/dlb_vector.h"
#include <math.h>

#define GRAVITY -9.81f

// HACK: These are closely related, and must be tuned to ensure velocity and
//       orientation stop changing at the same time. Not sure if there's a way
//       to calculate these analytically.
#define DV_EPSILON 0.001f     // minimum velocity required to affect position
#define DTHETA_EPSILON 0.08f  // minimum magnitude required to affect orientation

const char *ta_collider_type_str(int type)
{
    switch(type) {
        case TA_COLLIDER_PLANE:  return "TA_COLLIDER_PLANE";
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
    switch (body->collider.type) {
        case TA_COLLIDER_PLANE: {
            body->collider.data.plane.normal = vec3_normalize(body->collider.data.plane.normal);
            // Infinite AABB??
            // TODO: Calculate AABB for plane (add TA_EPSILON depth)
            break;
        } case TA_COLLIDER_SPHERE: {
            DLB_ASSERT(body->collider.data.sphere.radius > TA_EPSILON);
            body->aabb.center = body->collider.data.sphere.center;
            float radius = body->collider.data.sphere.radius;
            body->aabb.extents.x = radius;
            body->aabb.extents.y = radius;
            body->aabb.extents.z = radius;
            break;
        } case TA_COLLIDER_AABB: {
            DLB_ASSERT(body->collider.data.aabb.extents.x > TA_EPSILON);
            DLB_ASSERT(body->collider.data.aabb.extents.y > TA_EPSILON);
            DLB_ASSERT(body->collider.data.aabb.extents.z > TA_EPSILON);
            body->aabb = body->collider.data.aabb;
            break;
        } case TA_COLLIDER_OBB: {
            DLB_ASSERT(body->collider.data.obb.extents.x > TA_EPSILON);
            DLB_ASSERT(body->collider.data.obb.extents.y > TA_EPSILON);
            DLB_ASSERT(body->collider.data.obb.extents.z > TA_EPSILON);
            // TODO: Calculate AABB from OBB
            DLB_ASSERT(!"OBB not yet supported");
            break;
        } default: {
            DLB_ASSERT(!"Node needs AABB for broadphase");
        }
    }
    if (body->collider.type == TA_COLLIDER_PLANE &&
        vec3_len(body->collider.data.plane.normal))
    {
        body->collider.data.plane.normal =
            vec3_normalize(body->collider.data.plane.normal);
    }
    if (body->mass != 0.0f) {
        body->inv_mass = 1.0f / body->mass;
    }
    if (!body->e) {
        body->e = 0.2f;
    }
    if (!body->ks) {
        body->ks = 0.20f;
    }
    if (!body->kd) {
        body->kd = 0.10f;
    }
    if (body->collider.type == TA_COLLIDER_SPHERE) {
        // Sphere moment of inertia: 2/5 MR^2
        float sphere_moment = 2.0f/5.0f * body->mass *
            body->collider.data.sphere.radius * body->collider.data.sphere.radius;
        float inv_moment = 1.0f / sphere_moment;
        body->inv_tensor_local = mat3_init(
            inv_moment, 0.0f, 0.0f,
            0.0f, inv_moment, 0.0f,
            0.0f, 0.0f, inv_moment
        );
    }
    DLB_ASSERT(1);
}

void ta_rigid_body_apply_force(ta_rigid_body *body, ta_vec3 force)
{
    body->force_accum = vec3_add(body->force_accum, force);
}

void ta_rigid_body_apply_force_at(ta_rigid_body *body, ta_vec3 force, ta_vec3 at)
{
    // http://allenchou.net/2013/12/game-physics-motion-dynamics-implementations/
    body->force_accum = vec3_add(body->force_accum, force);
    body->torque_accum = vec3_add(body->torque_accum,
        vec3_cross(vec3_sub(at, body->centroid_global), force));
}

void ta_rigid_body_apply_impulse(ta_rigid_body *body, ta_vec3 impulse, ta_vec3 contact)
{
    if (body->inv_mass) {
        ta_vec3 dv = vec3_scalef(impulse, body->inv_mass);
        body->velocity = vec3_add(body->velocity, dv);

        ta_vec3 moment = vec3_cross(contact, impulse);
        ta_vec3 dw = mat3_mul_vec3(&body->inv_tensor_global, moment);
        body->ang_velocity = vec3_add(body->ang_velocity, dw);
    } else {
        body->velocity = VEC3_ZERO;
    }
}

void ta_rigid_body_update(ta_rigid_body *body, float dt)
{
    // Triggers don't need to update their position
    if (body->trigger) {
        return;
    }

    bool dirty = false;

    ta_transform *transform = ta_game_component(RES_COMP_TRANSFORM, body->entity_name);

    // TODO: Calculate this based on torque_accum and dt
    //body.m_angularVelocity +=  body.m_globalInverseInertiaTensor * (body.m_torqueAccumulator * dt);
    float dtheta_mag = vec3_len(body->ang_velocity);
    if (dtheta_mag > DTHETA_EPSILON) {
        ta_vec4 delta_orient = quat_from_axis_angle(
            vec3_normalize(body->ang_velocity),
            vec3_len(body->ang_velocity) //* dt  // TODO: angular dt??
        );
        transform->xform.orientation = quat_normalize(quat_mul(delta_orient,
            transform->xform.orientation));
        dirty = true;
    }

    // Update global tensor when orientation changes
    if (!quat_equal(body->tensor_orientation, transform->xform.orientation)) {
        // http://www.cs.cmu.edu/~baraff/sigcourse/notesd1.pdf p. D14
        // I_global = R * I_body * R^T
        ta_mat3 rot = mat3_rotate_quat(transform->xform.orientation);
        ta_mat3 rot_t = mat3_transpose(&rot);
        ta_mat3 inv_t_global = mat3_mul(&body->inv_tensor_local, &rot_t);
        inv_t_global = mat3_mul(&rot, &inv_t_global);
        body->inv_tensor_global = inv_t_global;
        body->tensor_orientation = transform->xform.orientation;
    }

    if (!body->no_gravity) {
        ta_vec3 gravity = { 0.0f, GRAVITY, 0.0f };
        ta_vec3 gravity_force = vec3_scalef(gravity, body->mass);
        ta_rigid_body_apply_force(body, gravity_force);
    }

    ta_vec3 acc = vec3_scalef(vec3_scalef(body->force_accum, dt), body->inv_mass);
    body->velocity = vec3_add(body->velocity, acc);
    ta_vec3 dv = vec3_scalef(body->velocity, dt);
    float dv_mag = vec3_len(dv);
    if (dv_mag > DV_EPSILON) {
        transform->xform.position = vec3_add(transform->xform.position, dv);
        dirty = true;
    }

    body->centroid_local = body->collider.data.center;
    body->centroid_global = vec3_add(transform->xform.position, body->collider.data.center);

#if 1
    // TODO: Implement drag in a way that doesn't vary with different timesteps
    //       The "compound interest" problem.
    body->velocity = vec3_scalef(body->velocity, 0.99f);
    body->ang_velocity = vec3_scalef(body->ang_velocity, 0.99f);
#endif

    // Reset accumulators
    body->force_accum = VEC3_ZERO;
    body->torque_accum = VEC3_ZERO;
}

bool ta_aabb_v_aabb(const ta_aabb *a, const ta_aabb *b, ta_manifold *manifold)
{
    // TODO: Handle generating manifolds for AABBs. For now, just allow this to
    //       be used for broadphase collision detection.
    DLB_ASSERT(!manifold);

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
        manifold->contact_count = 1;
        manifold->contacts[0] = vec3_add(a->center,
            vec3_scalef(manifold->normal, manifold->depth));
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
        manifold->contact_count = 1;
        manifold->contacts[0] = vec3_add(sphere->center,
            vec3_scalef(manifold->normal, -d));
    }

    return true;
}

static bool intersector_sphere_v_sphere(const ta_rigid_body *a,
    const ta_rigid_body *b, ta_manifold *manifold)
{
    ta_sphere sphere_a;
    sphere_a.center = vec3_add(a->centroid_global, a->collider.data.center);
    sphere_a.radius = a->collider.data.sphere.radius;
    ta_sphere sphere_b;
    sphere_b.center = vec3_add(b->centroid_global, b->collider.data.center);
    sphere_b.radius = b->collider.data.sphere.radius;
    bool collided = ta_sphere_v_sphere(&sphere_a, &sphere_b, manifold);
    return collided;
}

static bool intersector_plane_v_sphere(const ta_rigid_body *a,
    const ta_rigid_body *b, ta_manifold *manifold)
{
    ta_plane plane_a;
    plane_a.center = vec3_add(a->centroid_global, a->collider.data.center);
    plane_a.normal = a->collider.data.plane.normal;
    ta_sphere sphere_b;
    sphere_b.center = vec3_add(b->centroid_global, b->collider.data.center);
    sphere_b.radius = b->collider.data.sphere.radius;
    bool collided = ta_plane_v_sphere(&plane_a, &sphere_b, manifold);
    return collided;
}

bool ta_rigid_body_intersect(ta_rigid_body *a, ta_rigid_body *b,
    ta_manifold *manifold)
{
    DLB_ASSERT(a);
    DLB_ASSERT(b);

    typedef bool (intersector)(const ta_rigid_body *a, const ta_rigid_body *b,
        ta_manifold *manifold);

    static intersector *intersectors[TA_COLLIDER_COUNT][TA_COLLIDER_COUNT] = {
        [TA_COLLIDER_SPHERE][TA_COLLIDER_SPHERE] = intersector_sphere_v_sphere,
        [TA_COLLIDER_PLANE][TA_COLLIDER_SPHERE] = intersector_plane_v_sphere,
    };

    bool collided = false;

    ta_collider_type a_type = a->collider.type;
    ta_collider_type b_type = b->collider.type;
    intersector *intersect = a_type <= b_type
        ? intersectors[a_type][b_type]
        : intersectors[b_type][a_type];

    if (intersect) {
        collided = (*intersect)(a, b, manifold);
    } else {
        // TODO: Log this. For now, just return false.
        //ta_log_write(&tg_debug_log, "[Rigid Body] Unhandled collision pair.\n");
    }

    if (collided && manifold) {
        manifold->a = a;
        manifold->b = b;
        manifold->atrans = ta_game_component(RES_COMP_TRANSFORM, a->entity_name);
        manifold->btrans = ta_game_component(RES_COMP_TRANSFORM, b->entity_name);
        manifold->e = MAX(a->e, b->e);
        manifold->sf = sqrtf(a->ks * a->ks + b->ks * b->ks);
        manifold->df = sqrtf(a->kd * a->kd + b->kd * b->kd);
    }
    return collided;
}

void ta_rigid_body_resolve_collision(ta_manifold *manifold)
{
    DLB_ASSERT(manifold->a != manifold->b);

    ta_rigid_body *a = manifold->a;
    ta_rigid_body *b = manifold->b;
    ta_transform *atrans = manifold->atrans;
    ta_transform *btrans = manifold->btrans;

    // Trigger colliders don't need any resolution
    if (a->trigger || b->trigger) {
        return;
    }

    // https://github.com/RandyGaul/ImpulseEngine/blob/master/Manifold.cpp#L57
    if (a->inv_mass == 0.0f && b->inv_mass == 0.0f)
    {
        ta_log_write(&tg_debug_log, SRC_RIGID_BODY,
            "WARNING: Detected movement of infinite mass body\n");
        a->velocity = VEC3_ZERO;
        b->velocity = VEC3_ZERO;
        return;
    }

    for (u32 i = 0; i < manifold->contact_count; i++) {
        // Radii
        ta_vec3 ra = vec3_sub(manifold->contacts[i], atrans->xform.position);
        ta_vec3 rb = vec3_sub(manifold->contacts[i], btrans->xform.position);

        // Relative velocity
        //ta_vec3 rv = vec3_sub(b->velocity, a->velocity);
        ta_vec3 rv_a = vec3_sub(a->velocity, vec3_cross(a->ang_velocity, ra));
        ta_vec3 rv_b = vec3_add(b->velocity, vec3_cross(b->ang_velocity, rb));
        ta_vec3 rv = vec3_sub(rv_b, rv_a);

        // Relative velocity along normal
        float v_normal = vec3_dot(rv, manifold->normal);

        // If bodies moving apart, let it happen
        if (v_normal >= 0.0f) {
            return;
        }

        float e = manifold->e;
#if 0
        // "Box2D also uses inelastic collisions when the collision velocity is
        // small. This is done to prevent jitter." -Box2D manual
        if (v_normal < 0.01f) {
            e = 0.0f;
        }
#endif
        float j_numer = -(1.0f + e) * v_normal;

        // https://gamedevelopment.tutsplus.com/tutorials/how-to-create-a-custom-2d-physics-engine-oriented-rigid-bodies--gamedev-8032
        // TODO: Equation6 from the above the proper equation for 3D?
        // http://chrishecker.com/images/b/bb/Gdmphys4.pdf p. 24, Figure 4
        ta_vec3 impulse_ang_a = vec3_cross(mat3_mul_vec3(&a->inv_tensor_local,
            vec3_cross(ra, manifold->normal)), ra);
        ta_vec3 impulse_ang_b = vec3_cross(mat3_mul_vec3(&b->inv_tensor_local,
            vec3_cross(rb, manifold->normal)), rb);
        float impulse_ang = vec3_dot(vec3_add(impulse_ang_a, impulse_ang_b),
            manifold->normal);

        float j_denom = a->inv_mass + b->inv_mass + impulse_ang;

        // Calculate impulse
        float j = j_numer / j_denom;
        ta_vec3 impulse = vec3_scalef(manifold->normal, j);

        // Apply impulses
        ta_rigid_body_apply_impulse(a, vec3_neg(impulse), ra);
        ta_rigid_body_apply_impulse(b, impulse, rb);

#if _DEBUG && 1
        // Render debug primitives at collision contact points
        ta_vec3 contact_world = manifold->contacts[i];
        ta_vec3 a_impulse = vec3_scalef(impulse, a->inv_mass);
        ta_vec3 b_impulse = vec3_scalef(impulse, b->inv_mass);

        ta_sphere debug_a_contact;
        debug_a_contact.center = contact_world;
        debug_a_contact.radius = 0.1f;
        ta_primitive_push_sphere(debug_a_contact, TA_COLOR_RED);

        if (a->collider.type != TA_COLLIDER_PLANE) {
            ta_line_3d debug_a_contact_world;
            debug_a_contact_world.p0 = atrans->xform.position;
            debug_a_contact_world.p1 = manifold->contacts[i];
            ta_primitive_push_line_3d(debug_a_contact_world, TA_COLOR_WHITE,
                TA_COLOR_RED);
        }
        if (b->collider.type != TA_COLLIDER_PLANE) {
            ta_line_3d debug_b_contact_world;
            debug_b_contact_world.p0 = btrans->xform.position;
            debug_b_contact_world.p1 = manifold->contacts[i];
            ta_primitive_push_line_3d(debug_b_contact_world, TA_COLOR_WHITE,
                TA_COLOR_RED);
        }

        ta_line_3d debug_a_impulse;
        debug_a_impulse.p0 = contact_world;
        debug_a_impulse.p1 = vec3_add(contact_world, a_impulse);
        ta_primitive_push_line_3d(debug_a_impulse, TA_COLOR_WHITE, TA_COLOR_GREEN);
        ta_line_3d debug_b_impulse;
        debug_b_impulse.p0 = contact_world;
        debug_b_impulse.p1 = vec3_add(contact_world, b_impulse);
        ta_primitive_push_line_3d(debug_b_impulse, TA_COLOR_WHITE, TA_COLOR_BLUE);

        if (a->collider.type == TA_COLLIDER_PLANE) {
            ta_primitive_push_plane(a->collider.data.plane, 1.0f, TA_COLOR_CYAN);
        }
#endif

        // Randy's Coloumb friction
        // https://gamedevelopment.tutsplus.com/tutorials/how-to-create-a-custom-2d-physics-engine-friction-scene-and-jump-table--gamedev-7756

        // Recalculate relative velocity after impulses are applied
        //rv = vec3_sub(b->velocity, a->velocity);
        rv_a = vec3_sub(a->velocity, vec3_cross(a->ang_velocity, ra));
        rv_b = vec3_add(b->velocity, vec3_cross(b->ang_velocity, rb));
        rv = vec3_sub(rv_b, rv_a);

        float rv_normal = vec3_dot(rv, manifold->normal);
        ta_vec3 t = vec3_sub(rv, vec3_scalef(manifold->normal, rv_normal));
        if (!vec3_tiny(t)) {
            t = vec3_normalize(t);

            float jt = -vec3_dot(rv, t) / j_denom;
            float jt_abs = fabsf(jt);
            if (jt_abs < TA_EPSILON) {
                return;
            }

            ta_vec3 friction_impulse;
            if (jt_abs < j * manifold->sf) {
                friction_impulse = vec3_scalef(t, jt);
            } else {
                friction_impulse = vec3_scalef(t, -j * manifold->df);
            }

            // Apply friction impulses
            ta_rigid_body_apply_impulse(a, vec3_neg(friction_impulse), ra);
            ta_rigid_body_apply_impulse(b, friction_impulse, rb);
        }
    }

    // Positional correction
    const float slop = TA_EPSILON;
    const float percent = 1.0f;
    float c = MAX(manifold->depth - slop, 0.0f) / (a->inv_mass + b->inv_mass) *
        percent;
    ta_vec3 correction = vec3_scalef(manifold->normal, c);
    atrans->xform.position =
        vec3_sub(atrans->xform.position, vec3_scalef(correction, a->inv_mass));
    btrans->xform.position =
        vec3_add(btrans->xform.position, vec3_scalef(correction, b->inv_mass));
}