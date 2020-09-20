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
#include "ta_light.h"
#include "ta_primitive.h"
#include "ta_editor.h"
#include "dlb/dlb_vector.h"

void ta_model_free(ta_model *model)
{
    dlb_vec_free(model->materials);
    dlb_vec_free(model->morph_target_weights);
}
void ta_model_free_void(void *model)
{
    ta_model_free(model);
}

static bool model_find_morph_target_index(size_t *index, ta_model *model, const char *morph_target_name)
{
    size_t i = 0;
    bool found = false;

    ta_mesh *mesh = ta_game_by_name_try(RES_MESH, SYM(model->entity));
    if (mesh) {
        dlb_vec_each(ta_morph_target *, morph_target, mesh->morph_targets) {
            if (morph_target->name == morph_target_name) {
                if (index) *index = i;
                found = true;
                break;
            }
            i++;
        }
    }

    ta_log_write(&tg_debug_log, SRC_MODEL, "Morph target not found: %s\n", morph_target_name);
    return found;
}
float ta_model_get_morph_target_weight(ta_model *model, const char *morph_target_name)
{
    size_t index = 0;
    bool found = model_find_morph_target_index(&index, model, morph_target_name);
    if (found) {
        DLB_ASSERT(index < dlb_vec_len(model->morph_target_weights));
        return model->morph_target_weights[index];
    }
    return 0.0f;
}
void ta_model_set_morph_target_weight(ta_model *model, const char *morph_target_name, float weight)
{
    size_t index = 0;
    bool found = model_find_morph_target_index(&index, model, morph_target_name);
    if (found) {
        DLB_ASSERT(index < dlb_vec_len(model->morph_target_weights));
        model->morph_target_weights[index] = weight;
    }
}

static void model_set_shader_morph_targets(ta_model *model, ta_shader *shader)
{
    const size_t max_morph_targets = ARRAY_SIZE(SYM_U_MORPH_WEIGHTS);
    if (model->morph_target_weights) {
        size_t morph_idx = 0;
        dlb_vec_each(float *, morph_target_weight, model->morph_target_weights) {
            ta_shader_set_float(shader, SYM_U_MORPH_WEIGHTS[morph_idx], *morph_target_weight);
            morph_idx++;
            if (morph_idx == max_morph_targets) {
                break;
            }
        }
    } else {
        for (size_t i = 0; i < max_morph_targets; ++i) {
            ta_shader_set_float(shader, SYM_U_MORPH_WEIGHTS[i], 0.0f);
        }
    }
}

void ta_model_shadow_pass(ta_model *model, ta_shader *shader, ta_mat4 *light_pv)
{
    DLB_ASSERT(model);
    DLB_ASSERT(shader);
    DLB_ASSERT(light_pv);

    if (model->invisible || !model->cast_shadows) {
        return;
    }

    ta_transform *transform = ta_game_component(model->entity, RES_COMP_TRANSFORM);

    ta_mat4 light_pvm = mat4_mul(light_pv, &transform->world);
    ta_shader_set_mat4(shader, SYM_U_LIGHT_PVM, &light_pvm);
    ta_mesh *mesh = ta_game_by_sym_try(RES_MESH, model->mesh);
    if (!mesh) {
        // HACK: Decide if we really want to do this here..?
        mesh = ta_game_by_sym(RES_MESH, tg_mesh_default);
    }

    ta_shader_set_mat4(shader, SYM_U_MODEL, &transform->world);

    model_set_shader_morph_targets(model, shader);
    ta_shader_bind(shader);
    ta_mesh_render(mesh, shader);
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

    ta_transform *transform = ta_game_component(model->entity, RES_COMP_TRANSFORM);

    static const char *button = 0;
    if (!button) {
        button = INTERN("button_0001");
    }

    if (model->entity == button) {
        DLB_ASSERT(1);
    }

    if (!camera->debug_no_mesh) {
        const char *selected_entity = 0;
        ta_editor_selected_entity(&selected_entity);
        bool selected = model->entity == selected_entity;

        ta_mesh *mesh = ta_game_by_sym_try(RES_MESH, model->mesh);
        if (!mesh) {
            mesh = ta_game_by_sym(RES_MESH, tg_mesh_default);
        }
        // HACK: We need to assign all of the materials in model->materials to the material slots
        // TODO: Upload all materials in material UBO, use ta_shader_set_int to set material slots as index into
        // the material UBO.
        ta_material *material = 0;
        if (model->materials) {
            material = ta_game_by_sym_try(RES_MATERIAL, model->materials[0]);
        }
        if (!material) {
            material = ta_game_by_sym(RES_MATERIAL, tg_material_default);
        }

        // TODO: Need to group materials by shader if we allow them to start having custom shaders
        ta_shader *shader = ta_game_by_sym(RES_SHADER, material->shader);

        ta_shader_set_bool(shader, SYM_U_SELECTED, (GLboolean)selected);

        ta_transform *cam_trans = ta_game_component(camera->entity, RES_COMP_TRANSFORM);
        ta_shader_set_vec3(shader, SYM_U_CAMERA_POS, &cam_trans->xform_world.position);

        ta_light *lights = ta_game_resource_pool(RES_COMP_LIGHT);
        size_t lights_len = dlb_vec_len(lights);
        u32 u_lights_count = 0;
        for (u32 i = 0; i < lights_len; ++i) {
            if (lights[i].enabled) {
                //ta_shader_set_light(shader, SYM_U_LIGHTS, u_lights_count, &lights[i]);
                u_lights_count++;
            }
        }
        ta_shader_set_int(shader, SYM_U_LIGHTS_COUNT, u_lights_count);
        ta_shader_set_material(shader, SYM_U_MATERIAL, material);
        ta_shader_set_int(shader, SYM_U_DEBUG_CHANNEL, camera->dbg_channel);
        ta_shader_set_mat4(shader, SYM_U_PROJ, &camera->projection);
        ta_shader_set_mat4(shader, SYM_U_VIEW, &camera->look_at);
        ta_shader_set_mat4(shader, SYM_U_MODEL, &transform->world);

        model_set_shader_morph_targets(model, shader);
        ta_shader_bind(shader);
        ta_mesh_render(mesh, shader);
        ta_shader_unbind();
    }

    // TODO: Do this in a separate pass, after all models are rendered. One big buffer, single render call.
    if (camera->debug_normals) {
        ta_shader_set_mat4(tg_shader_lines, SYM_U_MODEL, &transform->world);
        ta_mesh *mesh = ta_game_by_sym_try(RES_MESH, model->mesh);
        if (!mesh) {
            mesh = ta_game_by_sym(RES_MESH, tg_mesh_default);
        }
        ta_mesh_push_normals(mesh);
        ta_primitive_render(true, false);
        ta_shader_set_mat4(tg_shader_lines, SYM_U_MODEL, &MAT4_IDENT);
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
    ta_shader_set_mat4(shader, SYM_U_MODEL, &transform->world);

    ta_mesh *mesh = ta_game_by_sym_try(RES_MESH, model->mesh);
    if (!mesh) {
        // HACK: Decide if we really want to do this here..?
        mesh = ta_game_by_sym(RES_MESH, tg_mesh_default);
    }

    // TODO: Fix this for editor_select (either make it a set_try or don't do it at all in this function)
    //model_set_shader_morph_targets(model, piece, shader);
    ta_shader_bind(shader);
    ta_mesh_render(mesh, shader);
    ta_shader_unbind();
}