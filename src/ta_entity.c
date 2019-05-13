#include "ta_entity.h"
#include "ta_scene.h"
#include "ta_schema.h"
#include "ta_symbol.h"
#include "ta_window.h"
#include "ta_game.h"
#include "dlb_vector.h"

void ta_entity_init(ta_entity *e)
{
    if (!e->aabb.extents.x) {
        ta_mesh_group *mesh_group = ta_entity_mesh_group(e);
        e->aabb = ta_mesh_group_aabb(mesh_group);
    }
}

ta_material *ta_entity_material(ta_entity *e)
{
    // NOTE: This could cache in e->material if we want to save the hash lookup
    ta_material *mat = ta_scene_find(e->scene, F_TA_MATERIAL, e->material_uid);
    return mat;
}

ta_mesh_group *ta_entity_mesh_group(ta_entity *e)
{
    // NOTE: This could cache in e->mesh_group if we want to save the hash lookup
    ta_mesh_group *mesh_group = ta_scene_find(e->scene, F_TA_MESH_GROUP,
        e->mesh_group_uid);
    return mesh_group;
}

void ta_entity_push_aabb(ta_entity *e, ta_rgba color)
{
    ta_primitive_push_aabb(e->aabb, color);
}

void ta_entity_push_normals(ta_entity *e)
{
    ta_mesh_group *mesh_group = ta_scene_find(e->scene, F_TA_MESH_GROUP,
        e->mesh_group_uid);
    ta_mesh_group_push_normals(mesh_group);
}

void ta_entity_render(ta_entity *e, ta_mat4 *proj, ta_mat4 *view)
{
    ta_material *mat = ta_entity_material(e);
    ta_shader *shader = ta_material_shader(mat);
    ta_texture *texture = ta_material_texture(mat);
    ta_mesh_group *mesh_group = ta_entity_mesh_group(e);

    // TODO: Calculate this in entity_update via mat4_mul(parent, transform)
    ta_mat4 model = MAT4_IDENT;

    ta_shader_set_mat4(shader, SYM_U_PROJ, proj);
    ta_shader_set_mat4(shader, SYM_U_VIEW, view);
    ta_shader_set_mat4(shader, SYM_U_MODEL, &model);
    ta_shader_set_sampler2d(shader, SYM_U_TEX0, texture->gl_id);
    ta_shader_bind(shader);
    ta_shader_prerender(shader);
    ta_mesh_group_render(mesh_group);
    ta_shader_unbind(shader);
}