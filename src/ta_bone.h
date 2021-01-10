#pragma once

#include "ta_schema.h"

typedef struct ta_bone {
    TA_COMPONENT_HEADER
    const char  *armature;  // [SYM] Name of armature this bone belongs to
} ta_bone;

void ta_bone_init(ta_bone *bone);
void ta_bone_init_void(void *bone);
void ta_bone_free(ta_bone *bone);
void ta_bone_free_void(void *bone);
