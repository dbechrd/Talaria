#include "ta_node.h"
#include "ta_schema.h"
#include "ta_symbol.h"
#include "ta_window.h"
#include "ta_game.h"
#include "ta_log.h"
#include "ta_mesh_group.h"
#include "ta_material.h"
#include "ta_texture.h"
#include "ta_button.h"
#include "ta_camera.h"
#include "ta_shader.h"
#include "ta_scene.h"
#include "ta_light.h"
#include "ta_editor.h"
#include "ta_rigid_body.h"
#include "ta_primitive.h"
#include "ta_entity.h"
#include "ta_model.h"
#include "ta_position.h"
#include "dlb/dlb_vector.h"

void ta_node_shadow_pass(ta_entity *entity, ta_shader *shader, ta_mat4 *light_pv,
    float alpha)
{
    ta_model *model = ta_scene_component(tg_game.scene, RES_COMP_MODEL, entity);

    if (model->invisible || !model->cast_shadows) {
        return;
    }
    DLB_ASSERT(dlb_vec_len(model->mesh_groups));

    ta_position *position = ta_scene_component(tg_game.scene, RES_COMP_POSITION,
        entity);
    ta_rigid_body *body = ta_scene_component(tg_game.scene, RES_COMP_RIGID_BODY,
        entity);

    ta_vec3 lerp_pos;
    ta_quat lerp_orient;

    if (body) {
        lerp_pos = vec3_lerp(position->transform.position, body->position, alpha);
        lerp_orient = quat_nlerp(position->transform.orientation, body->orientation, alpha);
    } else {
        lerp_pos = vec3_lerp(position->transform_prev.position, position->transform.position, alpha);
        lerp_orient = quat_nlerp(position->transform_prev.orientation, position->transform.orientation, alpha);
    }

    // TODO: Multiply position by parent via mat4_mul(parent, transform)
    ta_mat4 trans = mat4_translate(lerp_pos);
    ta_mat4 orient = mat4_rotate_quat(lerp_orient);
    position->model = mat4_mul(&trans, &orient);

    // TODO: Allow updating model uniform without having to rebind everything
    // We can probably bind in the set functions instead of prerender, or can
    // set a "loaded" flag for each uniform (will be 0 by default, so safer than
    // "dirty" flag), and only load when changed. I don't know how expensive
    // glUniform calls are, so this may or may not matter.
    ta_shader_set_mat4(shader, SYM_U_MODEL, &position->model);
    ta_mat4 light_pvm = mat4_mul(light_pv, &position->model);
    ta_shader_set_mat4(shader, SYM_U_LIGHT_PVM, &light_pvm);
    ta_shader_prerender(shader);
    dlb_vec_each(const char *, mesh_group_name, model->mesh_groups) {
        ta_mesh_group *mesh_group = ta_scene_find_by_name(tg_game.scene,
            RES_MESH_GROUP, mesh_group_name);
        ta_mesh_group_render(mesh_group);
    }
}

void ta_node_render(ta_entity *entity, ta_camera *camera, float alpha)
{
    DLB_ASSERT(entity);
    DLB_ASSERT(camera);
    ta_model *model = ta_scene_component(tg_game.scene, RES_COMP_MODEL, entity);

    if (model->invisible) {
        return;
    }
    // If debug flags set such that there's nothing to render
    if (camera->debug_no_mesh && !camera->debug_normals &&
        !camera->debug_bounding_boxes)
    {
        return;
    }
    DLB_ASSERT(dlb_vec_len(model->mesh_groups));

    ta_position *position = ta_scene_component(tg_game.scene, RES_COMP_POSITION,
        entity);
    ta_rigid_body *body = ta_scene_component(tg_game.scene, RES_COMP_RIGID_BODY,
        entity);

    ta_vec3 lerp_pos;
    ta_quat lerp_orient;
    if (body) {
        lerp_pos = vec3_lerp(position->transform.position, body->position, alpha);
        lerp_orient = quat_nlerp(position->transform.orientation,
            body->orientation, alpha);
    } else {
        lerp_pos = vec3_lerp(position->transform_prev.position,
            position->transform.position, alpha);
        lerp_orient = quat_nlerp(position->transform_prev.orientation,
            position->transform.orientation, alpha);
    }

    // TODO: Multiply position by parent via mat4_mul(parent, transform)
    ta_mat4 trans = mat4_translate(lerp_pos);
    ta_mat4 rot = mat4_rotate_quat(lerp_orient);
    ta_mat4 scal = MAT4_IDENT;
    position->model = mat4_mul(&rot, &scal);
    position->model = mat4_mul(&trans, &position->model);

    if (!camera->debug_no_mesh) {
        ta_material *material = ta_scene_find_by_name(tg_game.scene,
            RES_MATERIAL, model->material);
        ta_shader *shader = ta_scene_find_by_name(tg_game.scene,
            RES_SHADER, material->shader);
        ta_texture *texture_albedo = ta_scene_find_by_name(tg_game.scene,
            RES_SHADER, material->tex_albedo);
        ta_texture *texture_metallic = ta_scene_find_by_name(tg_game.scene,
            RES_SHADER, material->tex_metallic);
        DLB_ASSERT(material);
        DLB_ASSERT(shader);
        DLB_ASSERT(texture_albedo);
        DLB_ASSERT(texture_metallic);

        ta_shader_bind(shader);
        ta_shader_set_mat4(shader, SYM_U_PROJ, &camera->projection);
        ta_shader_set_mat4(shader, SYM_U_VIEW, &camera->look_at);
        ta_shader_set_mat4(shader, SYM_U_MODEL, &position->model);
        ta_light *lights = tg_game.scene->resource_data[RES_COMP_LIGHT];
        u32 lights_len = dlb_vec_len(lights);
        ta_shader_set_uint(shader, SYM_U_LIGHTS_COUNT, lights_len);
        for (u32 i = 0; i < lights_len; ++i) {
            if (lights[i].disabled) {
                continue;
            }
            ta_shader_set_light(shader, SYM_U_LIGHTS, i, &lights[i]);
        }
        ta_shader_set_vec3(shader, SYM_U_CAMERA_POS, &camera->position);
        ta_shader_set_sampler2d(shader, SYM_U_TEX_ALBEDO, texture_albedo->gl_id);
        ta_shader_set_sampler2d(shader, SYM_U_TEX_METALLIC, texture_metallic->gl_id);
        ta_shader_prerender(shader);
        dlb_vec_each(const char *, mesh_group_name, model->mesh_groups) {
            ta_mesh_group *mesh_group = ta_scene_find_by_name(tg_game.scene,
                RES_MESH_GROUP, mesh_group_name);
            ta_mesh_group_render(mesh_group);
        }
        ta_shader_unbind(shader);
    }

    if (camera->debug_normals || camera->debug_bounding_boxes) {
        ta_shader_set_mat4(tg_shader_lines, SYM_U_MODEL, &position->model);
        if (camera->debug_normals) {
            dlb_vec_each(const char *, mesh_group_name, model->mesh_groups) {
                ta_mesh_group *mesh_group = ta_scene_find_by_name(tg_game.scene,
                    RES_MESH_GROUP, mesh_group_name);
                ta_mesh_group_push_normals(mesh_group);
            }
        }
        if (camera->debug_bounding_boxes) {
            ta_primitive_push_aabb(body->aabb, TA_COLOR_RED);
        }
    } else if (entity->name == ta_editor_selected_node()) {
        ta_shader_set_mat4(tg_shader_lines, SYM_U_MODEL, &position->model);
        ta_primitive_push_aabb(body->aabb, TA_COLOR_ORANGE);
    }

    ta_primitive_render(true, false);
}

void ta_node_render_shader(ta_entity *entity, ta_camera *camera, ta_shader *shader,
    float alpha, float scale)
{
    DLB_ASSERT(entity);
    DLB_ASSERT(camera);
    DLB_ASSERT(shader);

    ta_model *model = ta_scene_component(tg_game.scene, RES_COMP_MODEL, entity);
    ta_position *position = ta_scene_component(tg_game.scene, RES_COMP_POSITION,
        entity);
    ta_rigid_body *body = ta_scene_component(tg_game.scene, RES_COMP_RIGID_BODY,
        entity);

    ta_vec3 lerp_pos;
    ta_quat lerp_orient;
    if (body) {
        lerp_pos = vec3_lerp(position->transform.position, body->position, alpha);
        lerp_orient = quat_nlerp(position->transform.orientation,
            body->orientation, alpha);
    } else {
        lerp_pos = vec3_lerp(position->transform_prev.position,
            position->transform.position, alpha);
        lerp_orient = quat_nlerp(position->transform_prev.orientation,
            position->transform.orientation, alpha);
    }

    // TODO: Multiply position by parent via mat4_mul(parent, transform)
    ta_mat4 trans = mat4_translate(lerp_pos);
    ta_mat4 rot = mat4_rotate_quat(lerp_orient);
    ta_mat4 scal = mat4_scalef(scale);
    position->model = mat4_mul(&rot, &scal);
    position->model = mat4_mul(&trans, &position->model);

    ta_shader_bind(shader);
    ta_shader_set_mat4(shader, SYM_U_PROJ, &camera->projection);
    ta_shader_set_mat4(shader, SYM_U_VIEW, &camera->look_at);
    ta_shader_set_mat4(shader, SYM_U_MODEL, &position->model);
    ta_shader_prerender(shader);
    dlb_vec_each(const char *, mesh_group_name, model->mesh_groups) {
        ta_mesh_group *mesh_group = ta_scene_find_by_name(tg_game.scene,
            RES_MESH_GROUP, mesh_group_name);
        ta_mesh_group_render(mesh_group);
    }
    ta_shader_unbind(shader);
}