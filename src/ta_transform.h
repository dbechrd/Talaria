#pragma once
#include "ta_schema.h"
#include "ta_math.h"

typedef struct ta_transform {
    TA_COMPONENT_HEADER
    ta_xform    xform;      // current transform
    ta_xform    xform_prev; // xform last frame (for interpolation)
    ta_mat4     model;      // cached model matrix
} ta_transform;

void ta_transform_init      (ta_transform *transform);
void ta_transform_update    (ta_transform *transform, float alpha);