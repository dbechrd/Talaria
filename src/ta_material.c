#include "ta_material.h"
#include "ta_schema.h"
#include "ta_scene.h"
#include "ta_shader.h"
#include "ta_texture.h"

ta_shader *ta_material_shader(ta_material *mat)
{
    ta_shader *shader = ta_scene_find(mat->uid.scene, TYP_SHADER,
        mat->shader_uid);
    return shader;
}

ta_texture *ta_material_texture_albedo(ta_material *mat)
{
    ta_texture *tex = ta_scene_find(mat->uid.scene, TYP_TEXTURE,
        mat->texture_albedo_uid);
    return tex;
}

ta_texture *ta_material_texture_metallic(ta_material *mat)
{
    ta_texture *tex = ta_scene_find(mat->uid.scene, TYP_TEXTURE,
        mat->texture_metallic_uid);
    return tex;
}