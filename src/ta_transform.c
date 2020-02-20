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
    transform->xform_prev = transform->xform;
}

void ta_transform_free(ta_transform *transform)
{
    dlb_vec_free(transform->children);
}

void ta_transform_update(ta_transform *transform, float alpha)
{
#if 1
    // TODO: Does lerp serve a useful purpose?
    ta_vec3 lerp_pos = vec3_lerp(transform->xform_prev.position,
        transform->xform.position, alpha);
    ta_vec4 lerp_orient = quat_nlerp(transform->xform_prev.orientation,
        transform->xform.orientation, alpha);
#else
    ta_vec3 lerp_pos = transform->xform.position;
    ta_vec4 lerp_orient = transform->xform.orientation;
#endif

    // TODO: Multiply position by parent via mat4_mul(parent, transform)
    ta_mat4 trans = mat4_translate(lerp_pos);
    ta_mat4 rot = mat4_rotate_quat(lerp_orient);
    ta_mat4 scal = MAT4_IDENT;
    transform->model = mat4_mul(&rot, &scal);
    transform->model = mat4_mul(&trans, &transform->model);

    transform->xform_prev = transform->xform;
}