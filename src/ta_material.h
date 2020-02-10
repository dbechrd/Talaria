#pragma once
#include "ta_uid.h"

typedef struct ta_material {
    size_t index;
    const char *name;
    const char *shader;
    const char *tex_albedo;
    const char *tex_height;
    const char *tex_metallic;
    const char *tex_normal;
    const char *tex_occlusion;
    const char *tex_roughness;
} ta_material;

void ta_material_init(ta_material *material);
