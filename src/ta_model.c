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

void ta_model_init(ta_model *model)
{
    TracyCZone(ctxMethod, true);

    if (quat_zero(model->xform.orientation)) {
        model->xform.orientation = QUAT_IDENT;
    } else {
        model->xform.orientation = quat_normalize(model->xform.orientation);
    }

    TracyCZoneEnd(ctxMethod);
}
void ta_model_init_void(void *model)
{
    ta_model_init(model);
}
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

    ta_mesh *mesh = (ta_mesh *)ta_game_by_name_try(RES_MESH, SYM(model->entity));
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

static void model_set_shader_morph_weights(ta_model *model, ta_shader *shader)
{
    if (model->morph_target_weights) {
        size_t morph_idx = 0;
        dlb_vec_each(float *, morph_target_weight, model->morph_target_weights) {
            ta_shader_set_float(shader, SYM_U_MORPH_WEIGHTS[morph_idx], *morph_target_weight);
            morph_idx++;
            if (morph_idx == TA_MODEL_MAX_MORPHS) {
                break;
            }
        }
    } else {
        for (size_t i = 0; i < TA_MODEL_MAX_MORPHS; ++i) {
            ta_shader_set_float(shader, SYM_U_MORPH_WEIGHTS[i], 0.0f);
        }
    }
}

void ta_model_shadow_pass(ta_model *model, ta_shader *shader, ta_mat4 *light_pv)
{
    DLB_ASSERT(model);
    DLB_ASSERT(shader);
    DLB_ASSERT(light_pv);

    if (model->no_render || model->no_shadow_cast) {
        return;
    }

    ta_transform *transform = (ta_transform *)ta_game_component(model->entity, RES_COMP_TRANSFORM);

    // TODO: Cache this? Is it worth the space?
    // Calculate visual offset matrix
    ta_mat4 trans = mat4_translate(model->xform.position);
    ta_mat4 rot = mat4_rotate_quat(model->xform.orientation);
    ta_mat4 visual = mat4_mul(&trans, &rot);
    visual = mat4_mul(&transform->world, &visual);

    ta_mat4 light_pvm = mat4_mul(light_pv, &visual);
    ta_shader_set_mat4(shader, SYM_U_LIGHT_PVM, &light_pvm);
    ta_mesh *mesh = (ta_mesh *)ta_game_by_sym_try(RES_MESH, model->mesh);
    if (!mesh) {
        // HACK: Decide if we really want to do this here..?
        mesh = ta_game_by_sym(RES_MESH, tg_mesh_default);
    }

    // Only used for point lights
    if (find_uniform_by_name_try(shader->uniforms, SYM_U_MODEL, TA_GLSL_MAT4)) {
        ta_shader_set_mat4(shader, SYM_U_MODEL, &visual);
    }
    model_set_shader_morph_weights(model, shader);

    ta_shader_bind(shader);
    ta_mesh_render(mesh);
    ta_shader_unbind(shader);
}

void ta_model_render(ta_model *model)
{
    DLB_ASSERT(model);

    if (model->no_render) {
        return;
    }
    // If debug flags set such that there's nothing to render
    if (tg_game.debug_no_mesh && !tg_game.debug_normals) {
        return;
    }

    ta_transform *transform = (ta_transform *)ta_game_component(model->entity, RES_COMP_TRANSFORM);

    static const char *button = 0;
    if (!button) {
        button = INTERN("button_0001");
    }

    if (model->entity == button) {
        DLB_ASSERT(1);
    }

    if (!tg_game.debug_no_mesh) {
        const char *selected_entity = 0;
        ta_editor_selected_entity(&selected_entity);
        bool selected = model->entity == selected_entity;

        ta_mesh *mesh = (ta_mesh *)ta_game_by_sym_try(RES_MESH, model->mesh);
        if (!mesh) {
            mesh = ta_game_by_sym(RES_MESH, tg_mesh_default);
        }

        // Bind bone UBOs
        if (mesh->skin.gl_ubo_bone_xforms) {
            glBindBufferBase(GL_UNIFORM_BUFFER, TA_GLSL_UBO_BONE_XFORMS, mesh->skin.gl_ubo_bone_xforms);
            if (mesh->skin.bone_xforms_dirty) {
                glBufferData(GL_UNIFORM_BUFFER, sizeof(mesh->skin.bone_xforms), mesh->skin.bone_xforms, GL_DYNAMIC_DRAW);
                mesh->skin.bone_xforms_dirty = false;
            }
        }
        if (mesh->skin.gl_ubo_bone_normal_xforms) {
            glBindBufferBase(GL_UNIFORM_BUFFER, TA_GLSL_UBO_BONE_NORMAL_XFORMS, mesh->skin.gl_ubo_bone_normal_xforms);
            if (mesh->skin.bone_normal_xforms_dirty) {
                glBufferData(GL_UNIFORM_BUFFER, sizeof(mesh->skin.bone_normal_xforms), mesh->skin.bone_normal_xforms, GL_DYNAMIC_DRAW);
                mesh->skin.bone_normal_xforms_dirty = false;
            }
        }

        // HACK: We need to assign all of the materials in model->materials to the material slots
        // TODO: Upload all materials in material UBO, use ta_shader_set_int to set material slots as index into
        // the material UBO.
        ta_material *material_old = 0;
        if (model->materials) {
            material_old = ta_game_by_sym_try(RES_MATERIAL, model->materials[0]);
        }
        if (!material_old) {
            material_old = ta_game_by_sym(RES_MATERIAL, tg_material_default);
        }

        // TODO: Need to group materials by shader if we allow them to start having custom shaders
        ta_shader *shader = (ta_shader *)ta_game_by_sym(RES_SHADER, material_old->shader);

        ta_shader_set_bool(shader, SYM_U_SELECTED, (GLboolean)selected);
        ta_shader_set_material(shader, SYM_U_MATERIAL, material_old);

        // TODO: Cache this? Is it worth the space?
        // Calculate visual offset matrix
        ta_mat4 trans = mat4_translate(model->xform.position);
        ta_mat4 rot = mat4_rotate_quat(model->xform.orientation);
        ta_mat4 visual = mat4_mul(&trans, &rot);
        visual = mat4_mul(&transform->world, &visual);
        ta_shader_set_mat4(shader, SYM_U_MODEL, &visual);

        model_set_shader_morph_weights(model, shader);
        ta_shader_bind(shader);

        /////////////////////////////////////////////////////////////////////////////////////////////////////////
        // ta_mesh_render() doesn't have access to the shader or model->materials, so i'm duplicating this here
        // for the case of rendering meshes for now
        if (!mesh->gl_vao) {
            mesh = ta_game_by_sym(RES_MESH, tg_mesh_default);
        }

        // HACK: Need to manually set material index uniform because shader has already been bound
        ta_shader_uniform *u_material_index = find_uniform_by_name_try(shader->uniforms, SYM_U_MATERIAL_INDEX,
            TA_GLSL_UINT);

        glBindVertexArray(mesh->gl_vao);
        if (mesh->index_arrays) {
            dlb_vec_each(ta_index_array *, index_array, mesh->index_arrays) {
                // Find material (if slot out of range or material doesn't exist, fallback on default material)
                ta_material *material = 0;
                if (index_array->material_slot < dlb_vec_len(model->materials)) {
                    material = ta_game_by_sym_try(RES_MATERIAL, model->materials[index_array->material_slot]);
                }
                if (!material) {
                    material = ta_game_by_sym(RES_MATERIAL, tg_material_default);
                }

                // HACK: Assumes that *all* materials are always bound in the same order as they exist in the densea
                // resource array. This is fine for now, but may need to be further abstracted if we only want to bind
                // a subset of loaded materials later on.
                ta_shader_set_uint(shader, SYM_U_MATERIAL_INDEX, (u32)material->index);

                // HACK: Need to manually set material index uniform because shader has already been bound
                glUniform1ui(u_material_index->location, u_material_index->value.gluint);

                size_t index_count = dlb_vec_len(index_array->values);
                DLB_ASSERT(index_count);
                glDrawElements(ta_mesh_gl_primitive_mode(index_array->mode), (GLsizei)index_count, GL_UNSIGNED_SHORT,
                    (void *)index_array->offset_bytes);
            }
        } else {
            size_t materials_count = dlb_vec_len(model->materials);
            if (materials_count > 1) {
                DLB_ASSERT(!"Models must use index arrays in order to support multiple materials");
            }

            ta_material *material = ta_game_by_sym_try(RES_MATERIAL, model->materials[0]);
            if (!material) {
                material = ta_game_by_sym(RES_MATERIAL, tg_material_default);
            }
            ta_shader_set_uint(shader, SYM_U_MATERIAL_INDEX, (u32)material->index);

            // HACK: Need to manually set material index uniform because shader has already been bound
            glUniform1ui(u_material_index->location, u_material_index->value.gluint);

            size_t positions_count = dlb_vec_len(mesh->positions);
            glDrawArrays(ta_mesh_gl_primitive_mode(mesh->mode), 0, (GLsizei)positions_count);
        }

        glBindVertexArray(0);
        //glBindBuffer(GL_UNIFORM_BUFFER, 0);

        // Reset bone UBOs
        if (mesh->skin.gl_ubo_bone_xforms) {
            glBindBufferBase(GL_UNIFORM_BUFFER, TA_GLSL_UBO_BONE_XFORMS, tg_mesh_gl_default_bone_xforms);
        }
        if (mesh->skin.gl_ubo_bone_normal_xforms) {
            glBindBufferBase(GL_UNIFORM_BUFFER, TA_GLSL_UBO_BONE_NORMAL_XFORMS, tg_mesh_gl_default_bone_normal_xforms);
        }
        /////////////////////////////////////////////////////////////////////////////////////////////////////////

        ta_shader_unbind();
    }

    // TODO: Do this in a separate pass, after all models are rendered. One big buffer, single render call.
    if (tg_game.debug_normals) {
        ta_shader_set_mat4(tg_shader_lines, SYM_U_MODEL, &transform->world);
        ta_mesh *mesh = (ta_mesh *)ta_game_by_sym_try(RES_MESH, model->mesh);
        if (!mesh) {
            mesh = ta_game_by_sym(RES_MESH, tg_mesh_default);
        }
        ta_mesh_render_debug_lines(mesh);
        ta_shader_set_mat4(tg_shader_lines, SYM_U_MODEL, &MAT4_IDENT);
    }
}

void ta_model_render_shader(ta_model *model, ta_camera *camera, ta_shader *shader)
{
    DLB_ASSERT(model);
    DLB_ASSERT(camera);
    DLB_ASSERT(shader);

    ta_transform *transform = (ta_transform *)ta_game_component(model->entity, RES_COMP_TRANSFORM);

    ta_shader_set_mat4(shader, SYM_U_PROJ, &camera->projection);
    ta_shader_set_mat4(shader, SYM_U_VIEW, &camera->look_at);

    // TODO: Cache this? Is it worth the space?
    // Calculate visual offset matrix
    ta_mat4 trans = mat4_translate(model->xform.position);
    ta_mat4 rot = mat4_rotate_quat(model->xform.orientation);
    ta_mat4 visual = mat4_mul(&trans, &rot);
    visual = mat4_mul(&transform->world, &visual);
    ta_shader_set_mat4(shader, SYM_U_MODEL, &visual);

    ta_mesh *mesh = (ta_mesh *)ta_game_by_sym_try(RES_MESH, model->mesh);
    if (!mesh) {
        // HACK: Decide if we really want to do this here..?
        mesh = ta_game_by_sym(RES_MESH, tg_mesh_default);
    }

    // TODO: Fix this for editor_select (either make it a set_try or don't do it at all in this function)
    //model_set_shader_morph_weights(model, piece, shader);
    ta_shader_bind(shader);
    ta_mesh_render(mesh);
    ta_shader_unbind();
}