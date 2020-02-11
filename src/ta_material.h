#pragma once
#include "ta_schema.h"

typedef struct ta_material {
    TA_RESOURCE_HEADER
    const char *shader;         // shader name
    const char *tex_albedo;     // relative path to texture file
    const char *tex_height;     // relative path to texture file
    const char *tex_metallic;   // relative path to texture file
    const char *tex_normal;     // relative path to texture file
    const char *tex_occlusion;  // relative path to texture file
    const char *tex_roughness;  // relative path to texture file
} ta_material;

void ta_material_init(ta_material *material);
