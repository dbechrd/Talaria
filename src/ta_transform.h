#pragma once
#include "ta_math.h"
#include "dlb/dlb_types.h"

typedef struct ta_transform {
    u32 index;
    const char *name;
    const char *entity_name;
    ta_xform xform;
    ta_xform xform_prev;
    ta_mat4 model;  // cached model matrix
} ta_transform;

void ta_transform_init(ta_transform *transform);