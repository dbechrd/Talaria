#include "ta_transform.h"

void ta_transform_init(ta_transform *transform)
{
    if (quat_zero(transform->xform.orientation)) {
        transform->xform.orientation = QUAT_IDENT;
    } else {
        transform->xform.orientation = quat_normalize(transform->xform.orientation);
    }
    //if (vec3_zero(node->xform.scale)) {
    //    node->xform.scale = VEC3_ONE;
    //}
    //transform->xform_prev = transform->xform;
}
void ta_transform_init_void(void *transform)
{
    ta_transform_init(transform);
}

void ta_transform_free(ta_transform *transform)
{
    dlb_vec_free(transform->children);
}
void ta_transform_free_void(void *transform)
{
    ta_transform_free(transform);
}

static void ta_transform_update(ta_transform *transform, float alpha, bool dirty_flag)
{
    DLB_ASSERT(transform->dirty_flag == dirty_flag);

    // Update local matrix
#if 0
    // TODO: Does lerp serve a useful purpose?
    ta_vec3 lerp_pos = vec3_lerp(transform->xform_prev.position, transform->xform.position, alpha);
    ta_vec4 lerp_orient = quat_nlerp(transform->xform_prev.orientation, transform->xform.orientation, alpha);
    transform->xform_prev = transform->xform;

    ta_mat4 trans = mat4_translate(lerp_pos);
    ta_mat4 rot = mat4_rotate_quat(lerp_orient);
#else
    UNUSED(alpha);
    ta_mat4 trans = mat4_translate(transform->xform.position);
    ta_mat4 rot = mat4_rotate_quat(transform->xform.orientation);
#endif
    ta_mat4 scal = MAT4_IDENT;
    transform->local = mat4_mul(&rot, &scal);
    transform->local = mat4_mul(&trans, &transform->local);

    // Get parent world matrix (recursively calculate if still dirty)
    if (transform->parent) {
        ta_transform *parent = ta_game_component(transform->parent, RES_COMP_TRANSFORM);
        if (parent->dirty_flag == dirty_flag) {
            ta_transform_update(parent, alpha, dirty_flag);
        }
        transform->world = mat4_mul(&parent->world, &transform->local);
    } else {
        transform->world = transform->local;
    }

    transform->xform_world.position = mat4_to_location(&transform->world);
    transform->xform_world.orientation = quat_normalize(mat4_to_quaternion(&transform->world));

    transform->dirty_flag = !dirty_flag;
}

void ta_transform_update_all(ta_transform *transforms, float alpha)
{
    // Represents the value that means "dirty" this frame. Flip-flops between true and false to prevent having to clear
    // every transform's dirty_flag.
    static bool dirty_flag = false;

    dlb_vec_each(ta_transform *, transform, transforms) {
        if (transform->dirty_flag == dirty_flag) {
            ta_transform_update(transform, alpha, dirty_flag);
        }
    }

    dirty_flag = !dirty_flag;
}