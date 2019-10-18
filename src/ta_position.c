#include "ta_position.h"

void ta_position_init(ta_position *position)
{
    if (quat_zero(position->transform.orientation)) {
        position->transform.orientation = QUAT_IDENT;
    }
    //if (vec3_zero(node->transform.scale)) {
    //    node->transform.scale = VEC3_ONE;
    //}
    position->transform_prev = position->transform;
}