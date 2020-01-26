#pragma once
#include "ta_math.h"
#include "dlb/dlb_types.h"

typedef struct ta_transform {
    size_t index;
    const char *name;
    const char *entity_name;
    ta_xform xform;
    ta_xform xform_prev;
    ta_mat4 model;  // cached model matrix
} ta_transform;

void ta_transform_init(ta_transform *transform);
void ta_transform_update(ta_transform *transform, float alpha);