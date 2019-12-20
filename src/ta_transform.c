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