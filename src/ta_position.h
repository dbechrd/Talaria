#pragma once
#include "ta_math.h"

typedef struct ta_position {
    u32 entity_id;

    ta_transform transform;
    ta_transform transform_prev;

    ta_mat4 model;  // cached model matrix
} ta_position;