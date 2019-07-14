#pragma once
#include "ta_uid.h"

typedef struct ta_material {
    ta_uid uid;
    const char *shader_uid;
    const char *texture_albedo_uid;
    const char *texture_metallic_uid;
} ta_material;

struct ta_shader *ta_material_shader(ta_material *mat);
struct ta_texture *ta_material_texture_albedo(ta_material *mat);
struct ta_texture *ta_material_texture_metallic(ta_material *mat);