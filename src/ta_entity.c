#include "ta_entity.h"
#include "ta_scene.h"
#include "ta_schema.h"
#include "ta_symbol.h"
#include "ta_window.h"
#include "ta_game.h"
#include "ta_log.h"
#include "dlb_vector.h"

void ta_entity_init(ta_entity *e)
{
    if (!e->material_uid) {
        e->material_uid = e->scene->materials->uid;
    }
    if (!e->mesh_group_uid) {
        e->mesh_group_uid = e->scene->mesh_groups->uid;
    }
    if (vec4_zero(e->transform.rotation)) {
        e->transform.rotation = QUAT_IDENT;
    }
    if (vec3_zero(e->transform.scale)) {
        e->transform.scale = VEC3_ONE;
    }
    if (!e->aabb.extents.x) {
        bool use_mesh_aabb = false;
        ta_rigid_body *rigid_body = ta_entity_rigid_body(e);
        if (rigid_body) {
            switch (rigid_body->collider.type) {
                case TA_COLLIDER_AABB: {
                    e->aabb = rigid_body->collider.data.aabb;
                    break;
                }
                default:
                    use_mesh_aabb = true;
            }
        } else {
            use_mesh_aabb = true;
        }
        if (use_mesh_aabb) {
            ta_mesh_group *mesh_group = ta_entity_mesh_group(e);
            e->aabb = ta_mesh_group_aabb(mesh_group);
        }
    }
}

ta_material *ta_entity_material(ta_entity *e)
{
    if (!e->material_uid) return 0;

    // NOTE: This could cache in e->material if we want to save the hash lookup
    ta_material *mat = ta_scene_find(e->scene, F_TA_MATERIAL, e->material_uid);
    return mat;
}

ta_mesh_group *ta_entity_mesh_group(ta_entity *e)
{
    if (!e->mesh_group_uid) return 0;

    // NOTE: This could cache in e->mesh_group if we want to save the hash lookup
    ta_mesh_group *mesh_group = ta_scene_find(e->scene, F_TA_MESH_GROUP,
        e->mesh_group_uid);
    return mesh_group;
}

ta_rigid_body *ta_entity_rigid_body(ta_entity *e)
{
    if (!e->rigid_body_uid) return 0;

    // NOTE: This could cache in e->mesh_group if we want to save the hash lookup
    ta_rigid_body *rigid_body = ta_scene_find(e->scene, F_TA_RIGID_BODY,
        e->rigid_body_uid);
    return rigid_body;
}

void ta_entity_update(ta_entity *e, double dt)
{
    UNUSED(dt);
    ta_rigid_body *rigid_body = ta_entity_rigid_body(e);
    if (rigid_body) {
        e->transform.position = rigid_body->transform.position;
    }
}

void ta_entity_push_aabb(ta_entity *e, ta_rgba color)
{
    ta_primitive_push_aabb(e->aabb, color);
}

void ta_entity_push_normals(ta_entity *e)
{
    ta_mesh_group *mesh_group = ta_entity_mesh_group(e);
    if (mesh_group) {
        ta_mesh_group_push_normals(mesh_group);
    }
}

void ta_entity_render(ta_entity *e, ta_mat4 *proj, ta_mat4 *view)
{
    ta_material *mat = ta_entity_material(e);
    ta_shader *shader = ta_material_shader(mat);
    ta_texture *texture = ta_material_texture(mat);
    ta_mesh_group *mesh_group = ta_entity_mesh_group(e);

    // TODO: Allow some entities to not be renderable; skip them
    DLB_ASSERT(mat);
    DLB_ASSERT(shader);
    DLB_ASSERT(texture);
    DLB_ASSERT(mesh_group);

    // TODO: Calculate this in entity_update via mat4_mul(parent, transform)
    ta_mat4 model = mat4_translate(&e->transform.position);

    ta_shader_set_mat4(shader, SYM_U_PROJ, proj);
    ta_shader_set_mat4(shader, SYM_U_VIEW, view);
    ta_shader_set_mat4(shader, SYM_U_MODEL, &model);
    ta_shader_set_sampler2d(shader, SYM_U_TEX0, texture->gl_id);
    ta_shader_bind(shader);
    ta_shader_prerender(shader);
    ta_mesh_group_render(mesh_group);
    ta_shader_unbind(shader);
}