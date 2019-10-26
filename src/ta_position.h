#pragma once
#include "ta_math.h"
#include "dlb/dlb_types.h"

typedef struct ta_position {
    u32 index;
    const char *name;
    const char *entity_name;
    ta_transform transform;
    ta_transform transform_prev;
    ta_mat4 model;  // cached model matrix
} ta_position;

void ta_position_init(ta_position *position);