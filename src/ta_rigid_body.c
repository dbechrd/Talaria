#include "ta_rigid_body.h"
#include "ta_game.h"
#include "ta_intersect.h"
#include "ta_log.h"
#include "ta_primitive.h"
#include "ta_schema.h"
#include "ta_symbol.h"
#include "dlb/dlb_vector.h"
#include <math.h>
#include <float.h>

// HACK: These are closely related, and must be tuned to ensure velocity and
//       orientation stop changing at the same time. Not sure if there's a way
//       to calculate these analytically.
#define DV_EPSILON      0.001f  // minimum velocity required to affect position
#define DTHETA_EPSILON  0.04f   // minimum magnitude required to affect orientation

void ta_rigid_body_init(ta_rigid_body *body)
{
    TracyCZone(ctxMethod, true);

    ta_collider_init(&body->collider);
    if (body->mass) {
        body->inv_mass = 1.0f / body->mass;
    }
    if (!body->e) {
        body->e = 0.02f;
    }
    if (!body->ks) {
        body->ks = 0.98f;
    }
    if (!body->kd) {
        body->kd = 0.95f;
    }
    body->inv_tensor_local = ta_collider_inv_tensor(&body->collider, body->mass);
    if (quat_zero(body->xform.orientation)) {
        body->xform.orientation = QUAT_IDENT;
    } else {
        body->xform.orientation = quat_normalize(body->xform.orientation);
    }

    TracyCZoneEnd(ctxMethod);
}
void ta_rigid_body_init_void(void *body)
{
    ta_rigid_body_init(body);
}
void ta_rigid_body_free(ta_rigid_body *body)
{
    dlb_vec_free(body->colliding_with);
}
void ta_rigid_body_free_void(void *body)
{
    ta_rigid_body_free(body);
}
void ta_rigid_body_set_mass(ta_rigid_body *body, float mass)
{
    body->mass = MAX(0.0f, mass);
    body->inv_mass = mass ? 1.0f / mass : 0.0f;
    body->inv_tensor_local = ta_collider_inv_tensor(&body->collider, body->mass);
}
// Convert a point from body local space to world space
ta_vec3 rigid_body_local_to_world(const ta_rigid_body *body, ta_vec3 p_body)
{
    ta_vec3 p_world = vec3_add(body->xform.position, quat_mul_vec3(body->xform.orientation, p_body));
    return p_world;
}
// Convert a point from body local space to world space
ta_vec3 rigid_body_local_to_world_prev(const ta_rigid_body *body, ta_vec3 p_body)
{
    ta_vec3 p_world_prev = vec3_add(body->xform_prev.position, quat_mul_vec3(body->xform_prev.orientation, p_body));
    return p_world_prev;
}
// Convert a point from world space to body local space
ta_vec3 rigid_body_world_to_local(const ta_rigid_body *body, ta_vec3 p_world)
{
    ta_vec3 p_body = quat_mul_vec3(quat_inverse(body->xform.orientation), vec3_sub(p_world, body->xform.position));
    return p_body;
}
// Convert a point from world space to body local space
ta_vec3 rigid_body_world_to_local_prev(const ta_rigid_body *body, ta_vec3 p_world)
{
    ta_vec3 p_body = quat_mul_vec3(quat_inverse(body->xform_prev.orientation), vec3_sub(p_world, body->xform_prev.position));
    return p_body;
}
// Convert a vector from body rest orientation to body world orientation
ta_vec3 rigid_body_oriented_vector(const ta_rigid_body *body, ta_vec3 v_rest)
{
    ta_vec3 v_world = quat_mul_vec3(body->xform.orientation, v_rest);
    return v_world;
}
// Convert a vector from world orientation to body rest orientation
ta_vec3 rigid_body_rest_vector(const ta_rigid_body *body, ta_vec3 v_world)
{
    ta_vec3 v_rest = quat_mul_vec3(quat_inverse(body->xform.orientation), v_world);
    return v_rest;
}
// Convert a vector from body rest orientation to body world orientation
ta_vec4 rigid_body_oriented_quaternion(const ta_rigid_body *body, ta_vec4 q_rest)
{
    ta_vec4 q_world = quat_normalize(quat_mul(body->xform.orientation, q_rest));
    return q_world;
}
// Convert a vector from world orientation to body rest orientation
ta_vec4 rigid_body_rest_quaternion(const ta_rigid_body *body, ta_vec4 q_world)
{
    ta_vec4 q_rest = quat_normalize(quat_mul(quat_inverse(body->xform.orientation), q_world));
    return q_rest;
}
// Convert a point from centroid space to body space
ta_vec3 rigid_body_centroid_to_body(const ta_rigid_body *body, ta_vec3 p_centroid)
{
    ta_vec3 p_body = vec3_add(body->centroid_local, p_centroid);
    return p_body;
}
// Convert a point from body space to centroid space
ta_vec3 rigid_body_body_to_centroid(const ta_rigid_body *body, ta_vec3 p_body)
{
    ta_vec3 p_centroid = vec3_sub(p_body, body->centroid_local);
    return p_centroid;
}
// Convert a point from centroid space to world space
ta_vec3 rigid_body_centroid_to_world(const ta_rigid_body *body, ta_vec3 p_centroid)
{
    ta_vec3 p_world = rigid_body_local_to_world(body, rigid_body_centroid_to_body(body, p_centroid));
    return p_world;
}
// Convert a point from centroid space to world space using this body's previous xform
ta_vec3 rigid_body_centroid_to_world_prev(const ta_rigid_body *body, ta_vec3 p_centroid)
{
    ta_vec3 p_world_prev = rigid_body_local_to_world_prev(body, rigid_body_centroid_to_body(body, p_centroid));
    return p_world_prev;
}
// Convert a point from world space to centroid space
ta_vec3 rigid_body_world_to_centroid(const ta_rigid_body *body, ta_vec3 p_world)
{
    ta_vec3 p_centroid = rigid_body_body_to_centroid(body, rigid_body_world_to_local(body, p_world));
    return p_centroid;
}
// Convert a point from world space to centroid space
ta_vec3 rigid_body_world_to_centroid_prev(const ta_rigid_body *body, ta_vec3 p_world)
{
    ta_vec3 p_centroid = rigid_body_body_to_centroid(body, rigid_body_world_to_local_prev(body, p_world));
    return p_centroid;
}
// Return centroid in world space
ta_vec3 rigid_body_centroid_world(const ta_rigid_body *body)
{
    ta_vec3 centroid_world = rigid_body_local_to_world(body, body->centroid_local);
    return centroid_world;
}
// Return centroid taking body's current orientation into account (body local space, but with world orientation)
ta_vec3 rigid_body_centroid_oriented(const ta_rigid_body *body)
{
    ta_vec3 centroid_oriented = rigid_body_oriented_vector(body, body->centroid_local);
    return centroid_oriented;
}
// Return inverse tensor in world space (will update stale internal state on demand)
const ta_mat3 *rigid_body_inv_tensor_world(ta_rigid_body *body)
{
    if (!quat_equal(body->priv__tensor_orientation, body->xform.orientation)) {
        // http://www.cs.cmu.edu/~baraff/sigcourse/notesd1.pdf p. D14
        // I_global = R * I_body * R^T
        ta_mat3 rot = mat3_rotate_quat(body->xform.orientation);
        ta_mat3 rot_t = mat3_transpose(&rot);
        ta_mat3 inv_t_global = mat3_mul(&body->inv_tensor_local, &rot_t);
        inv_t_global = mat3_mul(&rot, &inv_t_global);
        body->priv__inv_tensor_world = inv_t_global;
        body->priv__tensor_orientation = body->xform.orientation;
    }
    return &body->priv__inv_tensor_world;
}
void ta_rigid_body_apply_force(ta_rigid_body *body, ta_vec3 force)
{
    body->force_accum = vec3_add(body->force_accum, force);
}
void ta_rigid_body_apply_force_at(ta_rigid_body *body, ta_vec3 force, ta_vec3 at)
{
    // http://allenchou.net/2013/12/game-physics-motion-dynamics-implementations/
    body->force_accum = vec3_add(body->force_accum, force);
    body->torque_accum = vec3_add(body->torque_accum, vec3_cross(vec3_sub(at, rigid_body_centroid_world(body)), force));
}
void ta_rigid_body_apply_impulse(ta_rigid_body *body, ta_vec3 impulse, ta_vec3 contact_local)
{
    if (body->inv_mass) {
        ta_vec3 dv = vec3_scalef(impulse, body->inv_mass);
        body->velocity = vec3_add(body->velocity, dv);

        if (!body->no_rotation) {
            //DLB_ASSERT(!mat3_equal(&body->inv_tensor_world, &MAT3_ZERO));
            ta_vec3 moment = vec3_cross(contact_local, impulse);
            ta_vec3 dw = mat3_mul_vec3(rigid_body_inv_tensor_world(body), moment);
            body->ang_velocity = vec3_add(body->ang_velocity, dw);
        }
    } else {
        body->velocity = VEC3_ZERO;
        body->ang_velocity = VEC3_ZERO;
    }
}
void ta_rigid_body_apply_positional_correction(ta_rigid_body *body, ta_vec3 impulse_world, ta_vec3 r_world)
{
    if (body->inv_mass == 0.0f) return;

    // NOTE: All planes are mathematical and should have infinite mass, they have no "center" and can't translate
    DLB_ASSERT(body->collider.type != TA_COLLIDER_PLANE);

    // contact penetration impulse
    ta_vec3 centroid_world = rigid_body_centroid_world(body);
    ta_vec3 new_centroid_world = vec3_add(centroid_world, vec3_scalef(impulse_world, body->inv_mass));
    body->xform.position = vec3_sub(new_centroid_world, rigid_body_centroid_oriented(body));

    // equation 8 & 9 in PBDBodies.pdf, not sure what [stuff, 0] means, or what "q1 + stuff" is.. quat_add??
    // q = q + 1/2[I^-1(r x p), 0]q
    // q = q - 1/2[I^-1(r x p), 0]q
    ta_vec4 new_orientation_world = quat_add(
        body->xform.orientation,
        quat_scale(
            quat_mul(
                vec4_init_vec3_w(mat3_mul_vec3(rigid_body_inv_tensor_world(body), vec3_cross(r_world, impulse_world)), 0),
                body->xform.orientation
            ),
        0.5f)
    );
    body->xform.orientation = quat_normalize(new_orientation_world);
    body->aabb = ta_collider_world_bounds(&body->collider, &body->xform);
}
// a       : first rigid body
// b       : second rigid body
// ra_local: contact radius in a's local space
// rb_local: contact radius in b's local space
// dx_world: correction impulse in world space (delta_x)
// alpha   : compliance of constraint (inverse of stiffness, meters/Newton)
// lambda  : Lagrangian multiplier (will be updated)
// dt      : delta time of substep
void ta_physics_apply_position_correction(ta_rigid_body *a, ta_rigid_body *b, ta_vec3 ra_local, ta_vec3 rb_local,
    ta_vec3 dx_world, float alpha, float *lambda, float dt, bool debug_render, ta_rgba debug_color)
{
    DLB_ASSERT(a);
    DLB_ASSERT(b);
    DLB_ASSERT(lambda);

    //------------------------------------------------------------------------------
    // NOTE: Everything in this function is in world space except ra_local/rb_local
    //------------------------------------------------------------------------------

    // Impulse magnitude and normal
    const float c = vec3_len(dx_world);
    const ta_vec3 n = vec3_scalef(dx_world, 1.0f/c);

    // NOTE: Don't "optimize" this; it has to be calculated again for each correction
    const ta_vec3 ra = rigid_body_oriented_vector(a, ra_local);
    const ta_vec3 rb = rigid_body_oriented_vector(b, rb_local);

    // Calculate generalized inverse masses
    // NOTE: Using equation 42 instead of equation 43 in PBDBodies.pdf for ease (they're equivalent)
    const float wa = a->inv_mass +
        vec3_dot(vec3_cross(mat3_mul_vec3(rigid_body_inv_tensor_world(a), vec3_cross(ra, n)), ra), n);
    const float wb = b->inv_mass +
        vec3_dot(vec3_cross(mat3_mul_vec3(rigid_body_inv_tensor_world(b), vec3_cross(rb, n)), rb), n);

    // Update Lagrangian multiplier
    // NOTE: Changed `-c` to `c` because lambda_n seemed to be pointing the wrong way and so did dx_world.. I really
    // don't know what's going on with the signs here unless PBDBodies.pdf uses B -> A direction for contact normals?
    // They don't seem to specify the normal direction anywhere.
    const float alpha_wavy = alpha / (dt * dt);
    const float delta_lambda = (c - alpha_wavy * (*lambda)) / (wa + wb + alpha_wavy);
    *lambda += delta_lambda;

    // "p" = Impulse vectors
    ta_vec3 p_a = vec3_scalef(n, delta_lambda);
    ta_vec3 p_b = vec3_neg(p_a);
    ta_rigid_body_apply_positional_correction(a, p_a, ra);
    ta_rigid_body_apply_positional_correction(b, p_b, rb);

    if (debug_render) {
        const ta_vec3 ca_world = rigid_body_local_to_world(a, ra_local);
        const ta_vec3 cb_world = rigid_body_local_to_world(b, rb_local);
        ta_rgba color_a = debug_color;
        ta_rgba color_b = debug_color;
        color_b.r *= 0.7f;
        color_b.g *= 0.7f;
        color_b.b *= 0.7f;
        ta_primitive_push_arrow(&primitive_lines_perma, ca_world, p_a, color_a);
        ta_primitive_push_arrow(&primitive_lines_perma, cb_world, p_b, color_b);
    }
}
void ta_rigid_body_apply_velocity_correction(ta_rigid_body *body, ta_vec3 impulse_world, ta_vec3 r_world)
{
    if (body->inv_mass == 0.0f) return;

    // NOTE: All planes are mathematical and should have infinite mass, they have no "center" and can't rotate
    DLB_ASSERT(body->collider.type != TA_COLLIDER_PLANE);
    DLB_ASSERT(!vec3_zero(impulse_world));

    body->velocity = vec3_add(body->velocity, vec3_scalef(impulse_world, body->inv_mass));
    if (vec3_tiny(body->velocity)) {
        body->velocity = VEC3_ZERO;
    }
    DLB_ASSERT(vec3_good(body->velocity));

    if (!body->no_rotation) {
        body->ang_velocity = vec3_add(body->ang_velocity, mat3_mul_vec3(rigid_body_inv_tensor_world(body),
            vec3_cross(r_world, impulse_world)));
        if (vec3_tiny(body->ang_velocity)) {
            body->ang_velocity = VEC3_ZERO;
        }
        DLB_ASSERT(vec3_good(body->ang_velocity));
    }
}
void ta_physics_apply_velocity_correction(ta_rigid_body *a, ta_rigid_body *b, ta_vec3 ra_local, ta_vec3 rb_local,
    ta_vec3 dv_world, bool debug_render, ta_rgba debug_color)
{
    DLB_ASSERT(a);
    DLB_ASSERT(b);

    //------------------------------------------------------------------------------
    // NOTE: Everything in this function is in world space except ra_local/rb_local
    //------------------------------------------------------------------------------

    if (!vec3_len2(dv_world)) return;

    // Impulse magnitude and normal
    const float c = vec3_len(dv_world);
    const ta_vec3 n = vec3_scalef(dv_world, 1.0f/c);

    // NOTE: This one could be cached because velocity corrections don't change the orientation of the body
    const ta_vec3 ra = rigid_body_oriented_vector(a, ra_local);
    const ta_vec3 rb = rigid_body_oriented_vector(b, rb_local);

    // Calculate generalized inverse masses
    // NOTE: Using equation 42 instead of equation 43 in PBDBodies.pdf for ease (they're equivalent)
    const float wa = a->inv_mass +
        vec3_dot(vec3_cross(mat3_mul_vec3(rigid_body_inv_tensor_world(a), vec3_cross(ra, n)), ra), n);
    const float wb = b->inv_mass +
        vec3_dot(vec3_cross(mat3_mul_vec3(rigid_body_inv_tensor_world(b), vec3_cross(rb, n)), rb), n);

    // "p" = Impulse vectors
    ta_vec3 p_a = vec3_scalef(dv_world, 1.0f/(wa + wb));
    ta_vec3 p_b = vec3_neg(p_a);
    ta_rigid_body_apply_velocity_correction(a, p_a, ra);
    ta_rigid_body_apply_velocity_correction(b, p_b, rb);

    if (debug_render) {
        const ta_vec3 ca_world = rigid_body_local_to_world(a, ra_local);
        const ta_vec3 cb_world = rigid_body_local_to_world(b, rb_local);
        ta_rgba color_a = debug_color;
        ta_rgba color_b = debug_color;
        color_b.r *= 0.7f;
        color_b.g *= 0.7f;
        color_b.b *= 0.7f;
        ta_primitive_push_arrow(&primitive_lines_perma, ca_world, p_a, color_a);
        ta_primitive_push_arrow(&primitive_lines_perma, cb_world, p_b, color_b);
    }
}

static bool intersector_plane_v_sphere(ta_manifold *manifold, const ta_rigid_body *a, const ta_rigid_body *b)
{
    DLB_ASSERT(a->collider.type == TA_COLLIDER_PLANE);
    DLB_ASSERT(b->collider.type == TA_COLLIDER_SPHERE);

    if (b->name == tg_e_player_one) {
        DLB_ASSERT(1);
    }

    ta_plane plane_a = a->collider.data.plane;
    ta_sphere sphere_b = b->collider.data.sphere;

    plane_a.center = rigid_body_local_to_world(a, plane_a.center);
    sphere_b.center = rigid_body_local_to_world(b, sphere_b.center);

    bool collided = ta_plane_v_sphere(manifold, &plane_a, &sphere_b);
    for (u32 i = 0; i < manifold->contact_count; ++i) {
        manifold->contacts[i].ra_local = rigid_body_rest_vector(a, manifold->contacts[i].ra_local);
        manifold->contacts[i].rb_local = rigid_body_rest_vector(b, manifold->contacts[i].rb_local);
    }
    return collided;
}
static bool intersector_plane_v_obb(ta_manifold *manifold, const ta_rigid_body *a, const ta_rigid_body *b)
{
    DLB_ASSERT(a->collider.type == TA_COLLIDER_PLANE);
    DLB_ASSERT(b->collider.type == TA_COLLIDER_OBB);

    ta_plane plane_a = a->collider.data.plane;
    plane_a.center = rigid_body_local_to_world(a, plane_a.center);

    ta_obb obb_b = b->collider.data.obb;
    obb_b.center = rigid_body_local_to_world(b, obb_b.center);
    obb_b.orientation = rigid_body_oriented_quaternion(b, obb_b.orientation);

    bool collided = ta_plane_v_obb(manifold, &plane_a, &obb_b);
    for (u32 i = 0; i < manifold->contact_count; ++i) {
        manifold->contacts[i].ra_local = rigid_body_rest_vector(a, manifold->contacts[i].ra_local);
        manifold->contacts[i].rb_local = rigid_body_rest_vector(b, manifold->contacts[i].rb_local);
    }
    return collided;
}
static bool intersector_plane_v_capsule(ta_manifold *manifold, const ta_rigid_body *a, const ta_rigid_body *b)
{
    DLB_ASSERT(a->collider.type == TA_COLLIDER_PLANE);
    DLB_ASSERT(b->collider.type == TA_COLLIDER_CAPSULE);

    // TODO: Plane v. Capsule collision detection
    UNUSED(manifold);
    return false;
}
static bool intersector_sphere_v_sphere(ta_manifold *manifold, const ta_rigid_body *a, const ta_rigid_body *b)
{
    DLB_ASSERT(a->collider.type == TA_COLLIDER_SPHERE);
    DLB_ASSERT(b->collider.type == TA_COLLIDER_SPHERE);

    // Check sphere v. sphere in world space
    ta_sphere sphere_a = a->collider.data.sphere;
    sphere_a.center = rigid_body_local_to_world(a, sphere_a.center);

    ta_sphere sphere_b = b->collider.data.sphere;
    sphere_b.center = rigid_body_local_to_world(b, sphere_b.center);

    bool collided = ta_sphere_v_sphere(manifold, &sphere_a, &sphere_b);
    for (u32 i = 0; i < manifold->contact_count; ++i) {
        manifold->contacts[i].ra_local = rigid_body_rest_vector(a, manifold->contacts[i].ra_local);
        manifold->contacts[i].rb_local = rigid_body_rest_vector(b, manifold->contacts[i].rb_local);
    }
    return collided;
}
static bool intersector_sphere_v_obb(ta_manifold *manifold, const ta_rigid_body *a, const ta_rigid_body *b)
{
    DLB_ASSERT(a->collider.type == TA_COLLIDER_SPHERE);
    DLB_ASSERT(b->collider.type == TA_COLLIDER_OBB);

    ta_sphere sphere_a = a->collider.data.sphere;
    sphere_a.center = rigid_body_local_to_world(a, sphere_a.center);

    ta_obb obb_b = b->collider.data.obb;
    obb_b.center = rigid_body_local_to_world(b, obb_b.center);
    obb_b.orientation = rigid_body_oriented_quaternion(b, obb_b.orientation);

    bool collided = ta_sphere_v_obb(manifold, &sphere_a, &obb_b);
    for (u32 i = 0; i < manifold->contact_count; ++i) {
        manifold->contacts[i].ra_local = rigid_body_rest_vector(a, manifold->contacts[i].ra_local);
        manifold->contacts[i].rb_local = rigid_body_rest_vector(b, manifold->contacts[i].rb_local);
    }
    return collided;
}
static bool intersector_sphere_v_capsule(ta_manifold *manifold, const ta_rigid_body *a, const ta_rigid_body *b)
{
    DLB_ASSERT(a->collider.type == TA_COLLIDER_SPHERE);
    DLB_ASSERT(b->collider.type == TA_COLLIDER_CAPSULE);

    // TODO: Sphere v. Capsule collision detection
    UNUSED(manifold);
    return false;
}
static bool intersector_obb_v_obb(ta_manifold *manifold, const ta_rigid_body *a, const ta_rigid_body *b)
{
    DLB_ASSERT(a->collider.type == TA_COLLIDER_OBB);
    DLB_ASSERT(b->collider.type == TA_COLLIDER_OBB);

    // TODO: OBB v. OBB collision detection
    UNUSED(manifold);
    return false;
}
static bool intersector_obb_v_capsule(ta_manifold *manifold, const ta_rigid_body *a, const ta_rigid_body *b)
{
    DLB_ASSERT(a->collider.type == TA_COLLIDER_OBB);
    DLB_ASSERT(b->collider.type == TA_COLLIDER_CAPSULE);

    // TODO: OBB v. Capsule collision detection
    UNUSED(manifold);
    return false;
}
static bool intersector_capsule_v_capsule(ta_manifold *manifold, const ta_rigid_body *a, const ta_rigid_body *b)
{
    DLB_ASSERT(a->collider.type == TA_COLLIDER_CAPSULE);
    DLB_ASSERT(b->collider.type == TA_COLLIDER_CAPSULE);

    // TODO: Capsule v. Capsule collision detection
    UNUSED(manifold);
    return false;
}
bool ta_rigid_body_intersect(ta_manifold *manifold, ta_rigid_body *a, ta_rigid_body *b)
{
    typedef bool (intersector)(ta_manifold *manifold, ta_rigid_body *a, ta_rigid_body *b);

    static intersector *intersectors[TA_COLLIDER_COUNT][TA_COLLIDER_COUNT] = {
        [TA_COLLIDER_PLANE  ][TA_COLLIDER_SPHERE ] = intersector_plane_v_sphere,
        [TA_COLLIDER_PLANE  ][TA_COLLIDER_OBB    ] = intersector_plane_v_obb,
        [TA_COLLIDER_PLANE  ][TA_COLLIDER_CAPSULE] = intersector_plane_v_capsule,
        [TA_COLLIDER_SPHERE ][TA_COLLIDER_SPHERE ] = intersector_sphere_v_sphere,
        [TA_COLLIDER_SPHERE ][TA_COLLIDER_OBB    ] = intersector_sphere_v_obb,
        [TA_COLLIDER_SPHERE ][TA_COLLIDER_CAPSULE] = intersector_sphere_v_capsule,
        [TA_COLLIDER_OBB    ][TA_COLLIDER_OBB    ] = intersector_obb_v_obb,
        [TA_COLLIDER_OBB    ][TA_COLLIDER_CAPSULE] = intersector_obb_v_capsule,
        [TA_COLLIDER_CAPSULE][TA_COLLIDER_CAPSULE] = intersector_capsule_v_capsule,
    };

    DLB_ASSERT(a);
    DLB_ASSERT(b);
    // HACK: These asserts are just to make Visual Studio shut up. There's no
    // code path that would allow collider type to fall outside the valid range.
    DLB_ASSERT(a->collider.type >= 0);
    DLB_ASSERT(b->collider.type >= 0);

    ta_rigid_body *body_a = a;
    ta_rigid_body *body_b = b;
    if (body_a->collider.type > body_b->collider.type) {
        body_a = b;
        body_b = a;
    }

    // If this gets called at all, these two bodies are broadphase intersecting.
    body_a->dbg_broadphase = true;
    body_b->dbg_broadphase = true;

    bool collided = false;

    intersector *intersect_method = intersectors[body_a->collider.type][body_b->collider.type];
    if (intersect_method) {
        collided = (*intersect_method)(manifold, body_a, body_b);
    } else {
        // TODO: Log this. For now, just return false.
        //ta_log_write(&tg_debug_log, "[Rigid Body] Unhandled collision pair.\n");
    }

    if (collided) {
        if (manifold) {
            manifold->a = body_a;
            manifold->b = body_b;

            // Arithmetic mean (page 7)
            // https://graphics.stanford.edu/projects/bouncemap/assets/restitution_lowres.pdf
            manifold->e = (body_a->e + body_b->e) / 2.0f;
            manifold->coef_static  = (body_a->ks + body_b->ks) / 2.0f;
            manifold->coef_dynamic = (body_a->kd + body_b->kd) / 2.0f;

            DLB_ASSERT(!vec3_zero(manifold->normal_world));
            DLB_ASSERT(manifold->contact_count);
        }

        // Set some handy flags for debug rendering
        body_a->dbg_narrowphase = true;
        body_b->dbg_narrowphase = true;
    }

    return collided;
}
#if 0
void ta_rigid_body_resolve_collision(ta_manifold *manifold, float dt)
{
    DLB_ASSERT(manifold->a != manifold->b);

    ta_rigid_body *a = manifold->a;
    ta_rigid_body *b = manifold->b;

    // Sensor colliders don't need any resolution
    if (a->sensor || b->sensor) {
        return;
    }

    // https://github.com/RandyGaul/ImpulseEngine/blob/master/Manifold.cpp#L57
    if (a->inv_mass == 0.0f && b->inv_mass == 0.0f) {
        ta_log_write(&tg_debug_log, SRC_RIGID_BODY, "WARNING: Detected movement of infinite mass body\n");
        a->velocity = VEC3_ZERO;
        b->velocity = VEC3_ZERO;
        return;
    }

    // TODO: Use mesh instancing for primitives (need scale :/)
    // TODO: Use circular buffer for perma lines instead of dumping everything at arbitrary threshold
    if (dlb_vec_len(primitive_lines_perma.buffers[0]) > 100000) {
        ta_mesh_clear_buffers(&primitive_lines_perma);
    }

    if (a->name == tg_e_can || b->name == tg_e_can) {
        DLB_ASSERT(1);
    }

    for (u32 i = 0; i < manifold->contact_count; i++) {
        ta_vec3 a_center_world = a->centroid_world;
        ta_vec3 b_center_world = b->centroid_world;

        // Radii (to local space)
        ta_vec3 ra = vec3_sub(manifold->contacts[i].world, a_center_world);
        ta_vec3 rb = vec3_sub(manifold->contacts[i].world, b_center_world);

#if 0
        // HACK: Planes don't have a valid centroid for the purposes of collision resolution
        if (a->collider.type == TA_COLLIDER_PLANE) {
            ra = vec3_neg(rb);
            a_center_world = vec3_sub(manifold->contacts[i], ra);
            // Can't handle plane vs. plane rigid body interactions, makes no sense
            DLB_ASSERT(b->collider.type != TA_COLLIDER_PLANE);
        } else if (b->collider.type == TA_COLLIDER_PLANE) {
            rb = vec3_neg(ra);
            b_center_world = vec3_sub(manifold->contacts[i], rb);
        }
#endif
#if 1
        // Debug rendering (resolution impulse)
        ta_primitive_push_arrow(&primitive_lines_perma, a_center_world, ra, TA_COLOR_PINK);
        ta_primitive_push_arrow(&primitive_lines_perma, b_center_world, rb, TA_COLOR_CYAN);
#endif

        // Angular velocity
        ta_vec3 va_a = vec3_cross(a->ang_velocity, ra);
        ta_vec3 va_b = vec3_cross(b->ang_velocity, rb);
        // Linear velocity
        ta_vec3 v_a = vec3_sub(a->velocity, va_a);
        ta_vec3 v_b = vec3_add(b->velocity, va_b);
        // Relative velocity from A to B
        ta_vec3 rv = vec3_sub(v_b, v_a);
        // Closing velocity from A to B
        float v_closing = vec3_dot(rv, manifold->normal);

        // Relative acceleration from A to B
        ta_vec3 v_acc = vec3_sub(b->acceleration, a->acceleration);
        // Closing acceleration from A to B this frame (i.e. delta velocity)
        float v_acc_closing = vec3_dot(v_acc, manifold->normal) * dt;

        if (fabs(v_acc_closing) > 0) {
            v_closing -= v_acc_closing;
            v_closing = MIN(0, v_closing);  // Prevent over-correction from flipping the sign
        }

        float restitution = manifold->e;
#if 0
        // "Box2D also uses inelastic collisions when the collision velocity is
        // small. This is done to prevent jitter." -Box2D manual
        if (fabs(v_closing) < 0.01f) {
            restitution = 0.0f;
        }
#endif
        // Separating velocity (with respect to A)
        v_closing *= restitution;
        if (v_closing < 0) {
            // If bodies moving apart, let it happen
            continue;
        }

#if 0
        // https://gamedevelopment.tutsplus.com/tutorials/how-to-create-a-custom-2d-physics-engine-oriented-rigid-bodies--gamedev-8032
        // TODO: Equation6 from the above the proper equation for 3D?
        // http://chrishecker.com/images/b/bb/Gdmphys4.pdf p. 24, Figure 4
        // TODO: Read all of https://chrishecker.com/Rigid_Body_Dynamics#Physics_Articles
        ta_vec3 impulse_ang_a = vec3_cross(mat3_mul_vec3(&a->inv_tensor_local, vec3_cross(ra, manifold->normal)), ra);
        ta_vec3 impulse_ang_b = vec3_cross(mat3_mul_vec3(&b->inv_tensor_local, vec3_cross(rb, manifold->normal)), rb);
        float impulse_ang = vec3_dot(vec3_add(impulse_ang_a, impulse_ang_b), manifold->normal);
#else
        float impulse_ang = 0.0f;
#endif

        float j_denom = a->inv_mass + b->inv_mass + impulse_ang;

        // Calculate impulse
        float jn = v_closing / j_denom;
        ta_vec3 a_resolve = vec3_scalef(manifold->normal, -jn);
        ta_vec3 b_resolve = vec3_neg(a_resolve);

        // Apply separation impulses
        ta_rigid_body_apply_impulse(a, a_resolve, ra);
        ta_rigid_body_apply_impulse(b, b_resolve, rb);

#if 0
        // Debug rendering (resolution impulse)
        if (a->inv_mass) {
            ta_vec3 a_impulse = vec3_scalef(a_resolve, a->inv_mass);
            ta_primitive_push_arrow(0, manifold->contacts[i], a_impulse, TA_COLOR_MAGENTA);
        }
        if (b->inv_mass) {
            ta_vec3 b_impulse = vec3_scalef(b_resolve, b->inv_mass);
            ta_primitive_push_arrow(0, manifold->contacts[i], b_impulse, TA_COLOR_CYAN);
        }
#endif

#if 0
        // Randy's Coloumb friction
        // https://gamedevelopment.tutsplus.com/tutorials/how-to-create-a-custom-2d-physics-engine-friction-scene-and-jump-table--gamedev-7756

        // Recalculate relative velocity after impulses are applied
        // Angular velocity
        va_a = vec3_cross(a->ang_velocity, ra);
        va_b = vec3_cross(b->ang_velocity, rb);
        // Linear velocity
        v_a = vec3_sub(a->velocity, va_a);
        v_b = vec3_add(b->velocity, va_b);
        // Relative velocity from A to B
        rv = vec3_sub(v_b, v_a);
        // Closing velocity from A to B
        v_closing = vec3_dot(rv, manifold->normal);

        // Tangent vector
        ta_vec3 t = vec3_sub(rv, vec3_scalef(manifold->normal, v_closing));
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

#if 0
            //-----------------------------
            // Debug rendering (friction impulse)
            if (a->inv_mass) {
                ta_primitive_push_arrow(0, manifold->contacts[i], a_friction, TA_COLOR_RED);
            }
            if (b->inv_mass) {
                ta_primitive_push_arrow(0, manifold->contacts[i], b_friction, TA_COLOR_GREEN);
            }
            //-----------------------------
#endif
        }
#endif
    }

    // Positional correction (original PBD)
    const float slop = TA_EPSILON;
    const float percent = 1.0f;
    float c = MAX(manifold->depth - slop, 0.0f) / (a->inv_mass + b->inv_mass) * percent;
    ta_vec3 correction = vec3_scalef(manifold->normal, c);

    a->xform.position = vec3_sub(a->xform.position, vec3_scalef(correction, a->inv_mass));
    b->xform.position = vec3_add(b->xform.position, vec3_scalef(correction, b->inv_mass));

    a->centroid_world = quat_mul_vec3(a->xform.orientation, a->centroid_local);
    a->centroid_world = vec3_add(a->centroid_world, a->xform.position);
    b->centroid_world = quat_mul_vec3(b->xform.orientation, b->centroid_local);
    b->centroid_world = vec3_add(b->centroid_world, b->xform.position);
    a->aabb = ta_collider_world_bounds(&a->collider, &a->xform);
    b->aabb = ta_collider_world_bounds(&b->collider, &b->xform);
}
#endif