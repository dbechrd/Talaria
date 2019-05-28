#include "ta_material.h"
#include "ta_scene.h"
#include "ta_schema.h"

ta_shader *ta_material_shader(ta_material *m)
{
    ta_shader *shader = ta_scene_find(m->ref.scene, F_TA_SHADER, m->shader_uid);
    return shader;
}

ta_texture *ta_material_texture(ta_material *m)
{
    ta_texture *tex = ta_scene_find(m->ref.scene, F_TA_TEXTURE, m->texture_uid);
    return tex;
}