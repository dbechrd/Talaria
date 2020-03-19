#include "ta_model.h"
#include "ta_shader.h"
#include "ta_scene.h"
#include "ta_transform.h"
#include "ta_rigid_body.h"
#include "ta_game.h"
#include "ta_schema.h"
#include "ta_symbol.h"
#include "ta_camera.h"
#include "ta_mesh.h"
#include "ta_material.h"
#include "ta_texture.h"
#include "ta_light.h"
#include "ta_primitive.h"
#include "ta_editor.h"
#include "dlb/dlb_vector.h"

void ta_model_free(ta_model *model)
{
    dlb_vec_free(model->pieces);
}

void ta_model_shadow_pass(ta_model *model, ta_shader *shader, ta_mat4 *light_pv)
{
    DLB_ASSERT(model);
    DLB_ASSERT(shader);
    DLB_ASSERT(light_pv);

    if (model->invisible || !model->cast_shadows) {
        return;
    }
    DLB_ASSERT(dlb_vec_len(model->pieces));

    ta_transform *transform = ta_game_component(model->entity, RES_COMP_TRANSFORM);

    ta_mat4 light_pvm = mat4_mul(light_pv, &transform->world);
    ta_shader_set_mat4(shader, SYM_U_LIGHT_PVM, &light_pvm);
    dlb_vec_each(ta_piece *, piece, model->pieces) {
        ta_mesh *mesh = ta_game_by_sym(RES_MESH, piece->mesh);
        if (vec3_zero(mesh->offset)) {
            ta_shader_set_mat4(shader, SYM_U_MODEL, &transform->world);
        } else {
            ta_mat4 offset = mat4_translate(mesh->offset);
            offset = mat4_mul(&offset, &transform->world);
            ta_shader_set_mat4(shader, SYM_U_MODEL, &offset);
        }
        ta_shader_bind(shader);
        ta_mesh_render(mesh);
    }
}

void ta_model_render(ta_model *model, ta_camera *camera)
{
    DLB_ASSERT(model);
    DLB_ASSERT(camera);

    if (model->invisible) {
        return;
    }
    // If debug flags set such that there's nothing to render
    if (camera->debug_no_mesh && !camera->debug_normals) {
        return;
    }
    DLB_ASSERT(dlb_vec_len(model->pieces));

    ta_transform *transform = ta_game_component(model->entity, RES_COMP_TRANSFORM);

    if (!camera->debug_no_mesh) {
        // TODO(perf): This probably does a lot of redundant work for models with multiple meshes
        dlb_vec_each(ta_piece *, piece, model->pieces) {
            ta_mesh     *mesh     = ta_game_by_sym(RES_MESH,     piece->mesh);
            ta_material *material = ta_game_by_sym(RES_MATERIAL, piece->material);
            ta_shader   *shader   = ta_game_by_sym(RES_SHADER,   material->shader);

            ta_transform *cam_trans = ta_game_component(camera->entity, RES_COMP_TRANSFORM);
            ta_shader_set_vec3(shader, SYM_U_CAMERA_POS, &cam_trans->xform_world.position);

            ta_light *lights = ta_game_resource_pool(RES_COMP_LIGHT);
            size_t lights_len = dlb_vec_len(lights);
            u32 u_lights_count = 0;
            for (u32 i = 0; i < lights_len; ++i) {
                if (!lights[i].disabled) {
                    ta_shader_set_light(shader, SYM_U_LIGHTS, u_lights_count, &lights[i]);
                    u_lights_count++;
                }
            }
            ta_shader_set_int(shader, SYM_U_LIGHTS_COUNT, u_lights_count);
            ta_shader_set_material(shader, SYM_U_MATERIAL, material);
            ta_shader_set_int(shader, SYM_U_DEBUG_CHANNEL, camera->dbg_channel);
            ta_shader_set_mat4(shader, SYM_U_PROJ, &camera->projection);
            ta_shader_set_mat4(shader, SYM_U_VIEW, &camera->look_at);

            if (vec3_zero(mesh->offset)) {
                ta_shader_set_mat4(shader, SYM_U_MODEL, &transform->world);
            } else {
                ta_mat4 offset = mat4_translate(mesh->offset);
                offset = mat4_mul(&transform->world, &offset);
                ta_shader_set_mat4(shader, SYM_U_MODEL, &offset);
            }
            ta_shader_bind(shader);
            ta_mesh_render(mesh);
        }
        ta_shader_unbind();
    }

    if (camera->debug_normals) {
        ta_shader_set_mat4(tg_shader_lines, SYM_U_MODEL, &transform->world);
        ta_shader_set_mat4(tg_shader_quads, SYM_U_MODEL, &transform->world);
        dlb_vec_each(ta_piece *, piece, model->pieces) {
            ta_mesh *mesh = ta_game_by_sym(RES_MESH, piece->mesh);
            ta_mesh_push_normals(mesh);
            ta_primitive_render(true, false);
        }
    }
}

void ta_model_render_shader(ta_model *model, ta_camera *camera, ta_shader *shader)
{
    DLB_ASSERT(model);
    DLB_ASSERT(camera);
    DLB_ASSERT(shader);

    ta_transform *transform = ta_game_component(model->entity, RES_COMP_TRANSFORM);

    ta_shader_set_mat4(shader, SYM_U_PROJ, &camera->projection);
    ta_shader_set_mat4(shader, SYM_U_VIEW, &camera->look_at);
    dlb_vec_each(ta_piece *, piece, model->pieces) {
        ta_mesh *mesh = ta_game_by_sym(RES_MESH, piece->mesh);
        if (vec3_zero(mesh->offset)) {
            ta_shader_set_mat4(shader, SYM_U_MODEL, &transform->world);
        } else {
            ta_mat4 offset = mat4_translate(mesh->offset);
            offset = mat4_mul(&transform->world, &offset);
            ta_shader_set_mat4(shader, SYM_U_MODEL, &offset);
        }
        ta_shader_bind(shader);
        ta_mesh_render(mesh);
    }
    ta_shader_unbind();
}