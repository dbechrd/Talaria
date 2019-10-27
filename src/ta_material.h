#pragma once
#include "ta_uid.h"

typedef struct ta_material {
    u32 index;
    const char *name;
    const char *shader;
    const char *tex_albedo;
    const char *tex_metallic;
} ta_material;