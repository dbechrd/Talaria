#pragma once
#include "ta_uid.h"

typedef struct ta_material {
    u32 index;
    const char *name;
    const char *shader_name;
    const char *tex_albedo_name;
    const char *tex_metallic_name;
} ta_material;