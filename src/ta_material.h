#pragma once
#include "ta_scene.h"
#include "ta_shader.h"
#include "ta_texture.h"

typedef struct ta_material_s {
    ta_uid uid;
    const char *shader_uid;
    const char *texture_albedo_uid;
    const char *texture_metallic_uid;
} ta_material;

ta_shader *ta_material_shader(ta_material *mat);
ta_texture *ta_material_texture_albedo(ta_material *mat);
ta_texture *ta_material_texture_metallic(ta_material *mat);