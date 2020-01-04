#include "ta_rigid_body.h"
#include "ta_game.h"
#include "ta_intersect.h"
#include "ta_log.h"
#include "ta_primitive.h"
#include "ta_schema.h"
#include "ta_symbol.h"
#include "ta_transform.h"
#include "dlb/dlb_vector.h"
#include <math.h>
#include <float.h>

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
        case TA_COLLIDER_OBB:    return "TA_COLLIDER_OBB";
        default:
            DLB_ASSERT(!"<UNKNOWN_TA_COLLIDER_TYPE>");
            return 0;
    }
}

void ta_rigid_body_init(ta_rigid_body *body)
{
    ta_collider_init(&body->collider);
    if (body->mass) {
        body->inv_mass = 1.0f / body->mass;
    }
    if (!body->e) {
        body->e = 0.5f;
    }
    // TODO: Why is ks < kd? Am I supposed to 1.0 - kd? Hmm..
    if (!body->ks) {
        //body->ks = 0.20f;
        body->ks = 0.05f;
    }
    if (!body->kd) {
        //body->kd = 0.10f;
        body->kd = 0.20f;
    }
    body->inv_tensor_local = ta_collider_inv_tensor(&body->collider, body->mass);
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

        //DLB_ASSERT(!mat3_equal(&body->inv_tensor_global, &MAT3_ZERO));
        ta_vec3 moment = vec3_cross(contact, impulse);
        ta_vec3 dw = mat3_mul_vec3(&body->inv_tensor_global, moment);
        body->ang_velocity = vec3_add(body->ang_velocity, dw);
    } else {
        body->velocity = VEC3_ZERO;
        body->ang_velocity = VEC3_ZERO;
    }
}

void ta_rigid_body_update(ta_rigid_body *body, float dt)
{
    ta_transform *transform = ta_game_component(RES_COMP_TRANSFORM, body->entity_name);

    // Update inverse mass when mass changes (editor UI)
    body->inv_mass = body->mass ? 1.0f / body->mass : 0.0f;

    // TODO: Calculate this based on torque_accum and dt
    //body.m_angularVelocity +=  body.m_globalInverseInertiaTensor * (body.m_torqueAccumulator * dt);
    float dtheta_mag = vec3_len(body->ang_velocity);
    if (dtheta_mag > DTHETA_EPSILON) {
        ta_vec4 delta_orient = quat_from_axis_angle(
            vec3_normalize(body->ang_velocity),
            dtheta_mag // * dt  // TODO: angular dt??
        );
        transform->xform.orientation = quat_normalize(quat_mul(delta_orient, transform->xform.orientation));
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
        gravity = vec3_scalef(gravity, body->mass);
        ta_rigid_body_apply_force(body, gravity);
    }

    // a = Fdt / m
    body->acceleration = vec3_scalef(body->force_accum, body->inv_mass);
    ta_vec3 da = vec3_scalef(body->acceleration, dt);
    body->velocity = vec3_add(body->velocity, da);
    ta_vec3 dv = vec3_scalef(body->velocity, dt);
    float dv_mag = vec3_len(dv);
    if (dv_mag > DV_EPSILON) {
        transform->xform.position = vec3_add(transform->xform.position, dv);
    }

    body->centroid_local = body->collider.data.center;
    body->centroid_global = vec3_rotate_quat(body->centroid_local, transform->xform.orientation);
    body->centroid_global = vec3_add(body->centroid_global, transform->xform.position);
    body->aabb = ta_collider_world_bounds(&body->collider, &transform->xform);

#if 1
    // TODO: Implement drag in a way that doesn't vary with different timesteps
    //       The "compound interest" problem.
    body->velocity = vec3_scalef(body->velocity, 0.99f);
    body->ang_velocity = vec3_scalef(body->ang_velocity, 0.99f);
#endif

    // Reset accumulators
    body->force_accum = VEC3_ZERO;
    body->torque_accum = VEC3_ZERO;

    body->dbg_broadphase = false;
    body->dbg_narrowphase = false;
}

static bool intersector_plane_v_sphere(ta_manifold *manifold,
    const ta_rigid_body *a, const ta_rigid_body *b)
{
    ta_plane plane_a;
    plane_a.center = a->centroid_global;
    plane_a.normal = a->collider.data.plane.normal;
    ta_sphere sphere_b;
    sphere_b.center = b->centroid_global;
    sphere_b.radius = b->collider.data.sphere.radius;
    bool collided = ta_plane_v_sphere(manifold, &plane_a, &sphere_b);
    return collided;
}

static bool intersector_plane_v_obb(ta_manifold *manifold,
    const ta_rigid_body *a, const ta_rigid_body *b)
{
    ta_transform *btrans = ta_game_component(RES_COMP_TRANSFORM, b->entity_name);

    ta_plane plane_a;
    plane_a.center = a->centroid_global;
    plane_a.normal = a->collider.data.plane.normal;
    ta_obb obb_b = b->collider.data.obb;
    obb_b.center = b->centroid_global;
    obb_b.orientation = quat_mul(btrans->xform.orientation, obb_b.orientation);
    bool collided = ta_plane_v_obb(manifold, &plane_a, &obb_b);
    return collided;
}

static bool intersector_sphere_v_sphere(ta_manifold *manifold,
    const ta_rigid_body *a, const ta_rigid_body *b)
{
    ta_sphere sphere_a;
    sphere_a.center = a->centroid_global;
    sphere_a.radius = a->collider.data.sphere.radius;
    ta_sphere sphere_b;
    sphere_b.center = b->centroid_global;
    sphere_b.radius = b->collider.data.sphere.radius;
    bool collided = ta_sphere_v_sphere(manifold, &sphere_a, &sphere_b);
    return collided;
}

static bool intersector_sphere_v_obb(ta_manifold *manifold,
    const ta_rigid_body *a, const ta_rigid_body *b)
{
    UNUSED(manifold && a && b);
    return false;
}

static bool intersector_obb_v_obb(ta_manifold *manifold, const ta_rigid_body *a,
    const ta_rigid_body *b)
{
    UNUSED(manifold && a && b);
    return false;
}

bool ta_rigid_body_intersect(ta_manifold *manifold, ta_rigid_body *a,
    ta_rigid_body *b)
{
    typedef bool (intersector)(ta_manifold *manifold, const ta_rigid_body *a,
        const ta_rigid_body *b);

    static intersector *intersectors[TA_COLLIDER_COUNT][TA_COLLIDER_COUNT] = {
        [TA_COLLIDER_PLANE][TA_COLLIDER_SPHERE] = intersector_plane_v_sphere,
        [TA_COLLIDER_PLANE][TA_COLLIDER_OBB] = intersector_plane_v_obb,
        [TA_COLLIDER_SPHERE][TA_COLLIDER_SPHERE] = intersector_sphere_v_sphere,
        [TA_COLLIDER_SPHERE][TA_COLLIDER_OBB] = intersector_sphere_v_obb,
        [TA_COLLIDER_OBB][TA_COLLIDER_OBB] = intersector_obb_v_obb,
    };

    DLB_ASSERT(a);
    DLB_ASSERT(b);

    // TODO(cleanup): If this gets called at all, these two bodies are
    // broadphase intersecting.
    // HACK: Don't set debug flags true for when colliding with floor. These are
    // used to set colors when rendering debug colliders.
    bool floor_collision = a->collider.type == TA_COLLIDER_PLANE ||
                           b->collider.type == TA_COLLIDER_PLANE;
    if (!floor_collision)
    {
        a->dbg_broadphase = true;
        b->dbg_broadphase = true;
    }

    if (a->trigger || b->trigger) {
        return false;
    }

    bool collided = false;
    bool swap_ab = (a->collider.type > b->collider.type);

    // HACK: These asserts are just to make Visual Studio shut up. There's no
    // code path that would allow collider type to fall outside the valid range.
    DLB_ASSERT(a->collider.type >= 0);
    DLB_ASSERT(b->collider.type >= 0);

    intersector *intersect_method = swap_ab
        ? intersectors[b->collider.type][a->collider.type]
        : intersectors[a->collider.type][b->collider.type];

    if (intersect_method) {
        collided = (*intersect_method)(manifold, a, b);
    } else {
        // TODO: Log this. For now, just return false.
        //ta_log_write(&tg_debug_log, "[Rigid Body] Unhandled collision pair.\n");
    }

    if (collided) {
        if (manifold) {
#if 0
            manifold->a = a;
            manifold->b = b;
#else
            // TODO: Is it necessary to swap normal and bodies?
            if (swap_ab) {
                manifold->a = b;
                manifold->b = a;
                manifold->normal = vec3_neg(manifold->normal);
            } else {
                manifold->a = a;
                manifold->b = b;
            }
#endif
            // Arithmetic mean (page 7)
            // https://graphics.stanford.edu/projects/bouncemap/assets/restitution_lowres.pdf
            manifold->e = (a->e + b->e) / 2.0f;
            // Randy likes Pythagorean, but wasteful if not noticeably different
            manifold->sf = (a->ks + b->ks) / 2.0f; // sqrtf(a->ks * a->ks + b->ks * b->ks);
            manifold->df = (a->kd + b->kd) / 2.0f; // sqrtf(a->kd * a->kd + b->kd * b->kd);
        }
        if (!floor_collision) {
            a->dbg_narrowphase = true;
            b->dbg_narrowphase = true;
        }
    }

    return collided;
}

void ta_rigid_body_resolve_collision(ta_manifold *manifold, float dt)
{
    DLB_ASSERT(manifold->a != manifold->b);

    ta_rigid_body *a = manifold->a;
    ta_rigid_body *b = manifold->b;
    ta_transform *atrans = ta_game_component(RES_COMP_TRANSFORM, a->entity_name);
    ta_transform *btrans = ta_game_component(RES_COMP_TRANSFORM, b->entity_name);

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
        ta_vec3 ra = vec3_sub(manifold->contacts[i], a->centroid_global);
        ta_vec3 rb = vec3_sub(manifold->contacts[i], b->centroid_global);

        // Relative velocity
        ta_vec3 rv_a = vec3_sub(a->velocity, vec3_cross(a->ang_velocity, ra));
        ta_vec3 rv_b = vec3_add(b->velocity, vec3_cross(b->ang_velocity, rb));
        ta_vec3 rv = vec3_sub(rv_b, rv_a);

        // Separating velocity along normal
        float v_separate = vec3_dot(rv, manifold->normal);
        if (v_separate >= 0) {
            // If bodies moving apart, let it happen
            continue;
        }

        float restitution = manifold->e;
#if 1
        // "Box2D also uses inelastic collisions when the collision velocity is
        // small. This is done to prevent jitter." -Box2D manual
        //if (fabs(rv_normal) < 0.01f) {
        //    restitution = 0.0f;
        //}
#endif
        float v_separate_new = -v_separate * restitution;

#if 1
        // Calculate separating velocity coming from acceleration this frame
        ta_vec3 v_acc = vec3_sub(b->acceleration, a->acceleration);
        float v_acc_separate = vec3_dot(v_acc, manifold->normal) * dt;

        if (v_acc_separate < 0) {
            v_separate_new += v_acc_separate * restitution;
            v_separate_new = MAX(0, v_separate_new);
        }

        float delta_v = v_separate_new - v_separate;
#else
        float delta_v = v_separate_new;
#endif

#if 1
        // https://gamedevelopment.tutsplus.com/tutorials/how-to-create-a-custom-2d-physics-engine-oriented-rigid-bodies--gamedev-8032
        // TODO: Equation6 from the above the proper equation for 3D?
        // http://chrishecker.com/images/b/bb/Gdmphys4.pdf p. 24, Figure 4
        // TODO: Read all of https://chrishecker.com/Rigid_Body_Dynamics#Physics_Articles
        ta_vec3 impulse_ang_a = vec3_cross(mat3_mul_vec3(&a->inv_tensor_local,
            vec3_cross(ra, manifold->normal)), ra);
        ta_vec3 impulse_ang_b = vec3_cross(mat3_mul_vec3(&b->inv_tensor_local,
            vec3_cross(rb, manifold->normal)), rb);
        float impulse_ang = vec3_dot(vec3_add(impulse_ang_a, impulse_ang_b),
            manifold->normal);
#else
        float impulse_ang = 0.0f;
#endif

        float j_denom = a->inv_mass + b->inv_mass + impulse_ang;

        // Calculate impulse
        float jn = delta_v / j_denom;
        ta_vec3 a_resolve = vec3_scalef(manifold->normal, -jn);
        ta_vec3 b_resolve = vec3_neg(a_resolve);

        // Apply separation impulses
        ta_rigid_body_apply_impulse(a, a_resolve, ra);
        ta_rigid_body_apply_impulse(b, b_resolve, rb);

        //-----------------------------
        // Debug rendering (resolution impulse)

        //ta_vec3 a_impulse = vec3_scalef(impulse, a->inv_mass);
        //ta_line_3d dbg_impulse_a;
        //dbg_impulse_a.p0 = manifold->contacts[i];
        //dbg_impulse_a.p1 = vec3_add(manifold->contacts[i], a_impulse);
        //dbg_impulse_a.p0 = vec3_sub(dbg_impulse_a.p0, (ta_vec3){0.01f,0.01f,0.01f});
        //dbg_impulse_a.p1 = vec3_sub(dbg_impulse_a.p1, (ta_vec3){0.01f,0.01f,0.01f});
        //ta_primitive_push_line_3d(dbg_impulse_a, TA_COLOR_MAGENTA, TA_COLOR_MAGENTA);

        //ta_vec3 b_impulse = vec3_scalef(impulse, b->inv_mass);
        //ta_line_3d dbg_impulse_b;
        //dbg_impulse_b.p0 = manifold->contacts[i];
        //dbg_impulse_b.p1 = vec3_add(manifold->contacts[i], b_impulse);
        //dbg_impulse_b.p0 = vec3_add(dbg_impulse_b.p0, (ta_vec3){0.01f,0.01f,0.01f});
        //dbg_impulse_b.p1 = vec3_add(dbg_impulse_b.p1, (ta_vec3){0.01f,0.01f,0.01f});
        //ta_primitive_push_line_3d(dbg_impulse_b, TA_COLOR_CYAN, TA_COLOR_CYAN);
        //-----------------------------

        // Randy's Coloumb friction
        // https://gamedevelopment.tutsplus.com/tutorials/how-to-create-a-custom-2d-physics-engine-friction-scene-and-jump-table--gamedev-7756

        // Recalculate relative velocity after impulses are applied
        rv_a = vec3_sub(a->velocity, vec3_cross(a->ang_velocity, ra));
        rv_b = vec3_add(b->velocity, vec3_cross(b->ang_velocity, rb));
        rv = vec3_sub(rv_b, rv_a);
        v_separate = vec3_dot(rv, manifold->normal);

        // Tangent vector
        ta_vec3 t = vec3_sub(rv, vec3_scalef(manifold->normal, v_separate));
        if (!vec3_tiny(t)) {
            t = vec3_normalize(t);

            float jt = -vec3_dot(rv, t) / j_denom;
            float jt_abs = fabsf(jt);
            if (jt_abs < TA_EPSILON) {
                continue;
            }

            ta_vec3 a_friction;
            if (jt_abs < jn * manifold->sf) {
                a_friction = vec3_scalef(t, -jt);
            } else {
                a_friction = vec3_scalef(t, jn * manifold->df);
            }
            ta_vec3 b_friction = vec3_neg(a_friction);

            // Apply friction impulses
            ta_rigid_body_apply_impulse(a, a_friction, ra);
            ta_rigid_body_apply_impulse(b, b_friction, rb);

            //-----------------------------
            // Debug rendering (friction impulse)
            if (a->inv_mass) {
                ta_line_3d dbg_friction_a = { 0 };
                dbg_friction_a.p0 = manifold->contacts[i];
                dbg_friction_a.p1 = vec3_add(manifold->contacts[i], a_friction);
                ta_primitive_push_line_3d(dbg_friction_a, TA_COLOR_RED, TA_COLOR_RED);
                ta_sphere dbg_friction_a_cone = { 0 };
                dbg_friction_a_cone.center = dbg_friction_a.p1;
                dbg_friction_a_cone.radius = 0.01f;
                ta_primitive_push_sphere(dbg_friction_a_cone, TA_COLOR_RED);
            }
            if (b->inv_mass) {
                ta_line_3d dbg_friction_b = { 0 };
                dbg_friction_b.p0 = manifold->contacts[i];
                dbg_friction_b.p1 = vec3_add(manifold->contacts[i], b_friction);
                ta_primitive_push_line_3d(dbg_friction_b, TA_COLOR_GREEN, TA_COLOR_GREEN);
                ta_sphere dbg_friction_b_cone = { 0 };
                dbg_friction_b_cone.center = dbg_friction_b.p1;
                dbg_friction_b_cone.radius = 0.01f;
                ta_primitive_push_sphere(dbg_friction_b_cone, TA_COLOR_GREEN);
            }
            //-----------------------------
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

    a->centroid_global = vec3_rotate_quat(a->centroid_local, atrans->xform.orientation);
    a->centroid_global = vec3_add(a->centroid_global, atrans->xform.position);
    b->centroid_global = vec3_rotate_quat(b->centroid_local, btrans->xform.orientation);
    b->centroid_global = vec3_add(b->centroid_global, btrans->xform.position);
    a->aabb = ta_collider_world_bounds(&a->collider, &atrans->xform);
    b->aabb = ta_collider_world_bounds(&b->collider, &btrans->xform);
}