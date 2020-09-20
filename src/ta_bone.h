#pragma once

#include "ta_schema.h"

typedef struct ta_bone {
    TA_COMPONENT_HEADER
    const char  *armature;  // [SYM] Name of armature this bone belongs to
} ta_bone;