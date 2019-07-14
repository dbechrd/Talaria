#include "ta_rigid_body.h"
#include "ta_node.h"
#include "ta_log.h"
#include "ta_primitive.h"
#include "dlb_vector.h"
#include <math.h>

typedef bool (intersector)(const ta_collider *a, const ta_collider *b,
    ta_manifold *manifold);

static intersector intersector_sphere_v_sphere;
static intersector intersector_plane_v_sphere;

static intersector *intersectors[TA_COLLIDER_COUNT][TA_COLLIDER_COUNT] = {
    [TA_COLLIDER_SPHERE][TA_COLLIDER_SPHERE] = intersector_sphere_v_sphere,
    [TA_COLLIDER_PLANE][TA_COLLIDER_SPHERE] = intersector_plane_v_sphere,
};

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

static void update_collider_center(ta_rigid_body *body)
{
    // TODO: For each collider in body->colliders
    body->collider.center_world = vec3_add(body->position, body->collider.data.center);
    body->centroid_local = body->collider.data.center;
    body->centroid_global = body->collider.center_world;
#if 0
    switch (body->collider.type) {
        case TA_COLLIDER_PLANE: {
            break;
        } case TA_COLLIDER_SPHERE: {
            break;
        } case TA_COLLIDER_AABB: {
            break;
        } case TA_COLLIDER_OBB: {
            break;
        }
    }
#endif
}

void ta_rigid_body_init(ta_rigid_body *body)
{
    if (quat_zero(body->orientation)) {
        body->orientation = QUAT_IDENT;
    } else {
        body->orientation = quat_normalize(body->orientation);
    }
    if (body->collider.type == 0 && !vec3_len(body->collider.data.plane.normal))
    {
        body->collider.type = TA_COLLIDER_SPHERE;
        body->collider.data.sphere.radius = 1.0f;
    } else if (body->collider.type == TA_COLLIDER_PLANE) {
        body->collider.data.plane.normal = vec3_normalize(body->collider.data.plane.normal);
    }
    update_collider_center(body);
    if (body->mass != 0.0f) {
        body->inv_mass = 1.0f / body->mass;
    }
    if (!body->e) {
        body->e = 0.5f;
    }
    if (!body->ks) {
        body->ks = 0.20f;
    }
    if (!body->kd) {
        body->kd = 0.15f;
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

    // TODO: Calculate this based on torque_accum and dt
    //body.m_angularVelocity +=  body.m_globalInverseInertiaTensor * (body.m_torqueAccumulator * dt);
    if (!vec3_equal(body->ang_velocity, VEC3_ZERO)) {
        ta_quat delta_orient = quat_from_axis_angle(
            vec3_normalize(body->ang_velocity),
            vec3_len(body->ang_velocity) //* dt  // TODO: angular dt??
        );
        body->orientation = quat_normalize(quat_mul(delta_orient, body->orientation));
    }

    ta_vec3 gravity = { 0.0f, -9.81f, 0.0f };
    ta_rigid_body_apply_force(body, gravity);

    ta_vec3 acc = vec3_scalef(vec3_scalef(body->force_accum, dt), body->inv_mass);
    body->velocity = vec3_add(body->velocity, acc);
    body->position = vec3_add(body->position,
        vec3_scalef(body->velocity, dt));
    update_collider_center(body);

#if 1
    // TODO: Implement drag in a way that doesn't vary with different timesteps
    //       The "compound interest" problem.
    body->velocity = vec3_scalef(body->velocity, 0.99f);
    body->ang_velocity = vec3_scalef(body->ang_velocity, 0.99f);
#endif

    // Update global tensor when orientation changes
    if (!quat_equals(body->tensor_orientation, body->orientation)) {
        // http://www.cs.cmu.edu/~baraff/sigcourse/notesd1.pdf p. D14
        // I_global = R * I_body * R^T
        ta_mat3 rot = mat3_rotate_quat(body->orientation);
        ta_mat3 rot_t = mat3_transpose(&rot);
        ta_mat3 inv_t_global = mat3_mul(&body->inv_tensor_local, &rot_t);
        inv_t_global = mat3_mul(&rot, &inv_t_global);
        body->inv_tensor_global = inv_t_global;
        body->tensor_orientation = body->orientation;
    }

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

static bool intersector_sphere_v_sphere(const ta_collider *a,
    const ta_collider *b, ta_manifold *manifold)
{
    ta_sphere sphere_a;
    sphere_a.center = a->center_world;
    sphere_a.radius = a->data.sphere.radius;
    ta_sphere sphere_b;
    sphere_b.center = b->center_world;
    sphere_b.radius = b->data.sphere.radius;
    bool collided = ta_sphere_v_sphere(&sphere_a, &sphere_b, manifold);
    return collided;
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
    bool collided = ta_plane_v_sphere(&plane, &sphere, manifold);
    return collided;
}

bool ta_rigid_body_intersect(const ta_rigid_body *a, const ta_rigid_body *b,
    ta_manifold *manifold)
{
    DLB_ASSERT(a);
    DLB_ASSERT(b);

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
        manifold->e = MAX(a->e, b->e);
        manifold->sf = sqrtf(a->ks * a->ks + b->ks * b->ks);
        manifold->df = sqrtf(a->kd * a->kd + b->kd * b->kd);
    }
    return collided;
}

void ta_rigid_body_resolve_collision(ta_manifold *manifold)
{
    ta_rigid_body *a = manifold->a;
    ta_rigid_body *b = manifold->b;

    // Trigger colliders don't need any resolution
    if (a->trigger || b->trigger) {
        return;
    }

    // https://github.com/RandyGaul/ImpulseEngine/blob/master/Manifold.cpp#L57
    if (a->inv_mass == 0.0f && b->inv_mass == 0.0f)
    {
        ta_log_write(tg_debug_log, "[WARNING] Detected movement of infinite mass body\n");
        a->velocity = VEC3_ZERO;
        b->velocity = VEC3_ZERO;
        return;
    }

    for (u32 i = 0; i < manifold->contact_count; i++) {
        // Radii
        ta_vec3 ra = vec3_sub(manifold->contacts[i], a->position);
        ta_vec3 rb = vec3_sub(manifold->contacts[i], b->position);

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
        ta_rigid_body_apply_impulse(a, vec3_negate(impulse), ra);
        ta_rigid_body_apply_impulse(b, impulse, rb);

#if _DEBUG
        ta_vec3 contact_world = manifold->contacts[i];
        ta_vec3 a_impulse = vec3_scalef(impulse, a->inv_mass);
        ta_vec3 b_impulse = vec3_scalef(impulse, b->inv_mass);

        ta_sphere debug_a_contact;
        debug_a_contact.center = contact_world;
        debug_a_contact.radius = 0.1f;
        ta_primitive_push_sphere(debug_a_contact, TA_COLOR_RED);

        if (a->collider.type != TA_COLLIDER_PLANE) {
            ta_line_3d debug_a_contact_world;
            debug_a_contact_world.p0 = a->position;
            debug_a_contact_world.p1 = manifold->contacts[i];
            ta_primitive_push_line_3d(debug_a_contact_world, TA_COLOR_WHITE,
                TA_COLOR_RED);
        }
        if (b->collider.type != TA_COLLIDER_PLANE) {
            ta_line_3d debug_b_contact_world;
            debug_b_contact_world.p0 = b->position;
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
            ta_rigid_body_apply_impulse(a, vec3_negate(friction_impulse), ra);
            ta_rigid_body_apply_impulse(b, friction_impulse, rb);
        }
    }
}

void ta_rigid_body_positional_correction(ta_manifold *manifold)
{
    ta_rigid_body *a = manifold->a;
    ta_rigid_body *b = manifold->b;

    // Trigger colliders don't need any resolution
    if (a->trigger || b->trigger) {
        return;
    }

    // Positional correction
    const float slop = TA_EPSILON;
    const float percent = 1.0f;
    float c = MAX(manifold->depth - slop, 0.0f) / (a->inv_mass + b->inv_mass) *
        percent;
    ta_vec3 correction = vec3_scalef(manifold->normal, c);
    a->position = vec3_sub(a->position, vec3_scalef(correction, a->inv_mass));
    b->position = vec3_add(b->position, vec3_scalef(correction, b->inv_mass));
}