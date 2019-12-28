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

void ta_model_shadow_pass(ta_model *model, ta_shader *shader, ta_mat4 *light_pv,
    float alpha)
{
    DLB_ASSERT(model);
    DLB_ASSERT(shader);
    DLB_ASSERT(light_pv);

    if (model->invisible || !model->cast_shadows) {
        return;
    }
    DLB_ASSERT(dlb_vec_len(model->meshes));

    ta_transform *transform = ta_game_component(RES_COMP_TRANSFORM,
        model->entity_name);

    ta_vec3 lerp_pos = vec3_lerp(transform->xform_prev.position,
        transform->xform.position, alpha);
    ta_vec4 lerp_orient = quat_nlerp(transform->xform_prev.orientation,
        transform->xform.orientation, alpha);

    // TODO: Multiply position by parent via mat4_mul(parent, transform)
    ta_mat4 trans = mat4_translate(lerp_pos);
    ta_mat4 orient = mat4_rotate_quat(lerp_orient);
    transform->model = mat4_mul(&trans, &orient);

    // TODO: Allow updating model uniform without having to rebind everything
    // We can probably bind in the set functions instead of prerender, or can
    // set a "loaded" flag for each uniform (will be 0 by default, so safer than
    // "dirty" flag), and only load when changed. I don't know how expensive
    // glUniform calls are, so this may or may not matter.
    ta_shader_set_mat4(shader, SYM_U_MODEL, &transform->model);
    ta_mat4 light_pvm = mat4_mul(light_pv, &transform->model);
    ta_shader_set_mat4(shader, SYM_U_LIGHT_PVM, &light_pvm);
    ta_shader_bind(shader);
    dlb_vec_each(const char **, mesh_name, model->meshes) {
        ta_mesh *mesh = ta_game_by_sym(RES_MESH, *mesh_name);
        ta_mesh_render(mesh);
    }
    ta_shader_unbind();
}

void ta_model_render(ta_model *model, ta_camera *camera, float alpha)
{
    DLB_ASSERT(model);
    DLB_ASSERT(camera);

    if (model->invisible) {
        return;
    }
    // If debug flags set such that there's nothing to render
    if (camera->debug_no_mesh && !camera->debug_normals &&
        !camera->debug_colliders)
    {
        return;
    }
    DLB_ASSERT(dlb_vec_len(model->meshes));

    ta_transform *transform = ta_game_component(RES_COMP_TRANSFORM, model->entity_name);

    ta_vec3 lerp_pos = vec3_lerp(transform->xform_prev.position,
        transform->xform.position, alpha);
    ta_vec4 lerp_orient = quat_nlerp(transform->xform_prev.orientation,
        transform->xform.orientation, alpha);

    // TODO: Multiply position by parent via mat4_mul(parent, transform)
    ta_mat4 trans = mat4_translate(lerp_pos);
    ta_mat4 rot = mat4_rotate_quat(lerp_orient);
    ta_mat4 scal = MAT4_IDENT;
    transform->model = mat4_mul(&rot, &scal);
    transform->model = mat4_mul(&trans, &transform->model);

    if (!camera->debug_no_mesh) {
        ta_material *material = ta_game_by_sym(RES_MATERIAL, model->material);
        ta_shader *shader = ta_game_by_sym(RES_SHADER, material->shader);
        ta_texture *texture_albedo    = ta_game_by_sym(RES_TEXTURE, material->tex_albedo    ? material->tex_albedo    : SYM_MISSING_ALBEDO);
        ta_texture *texture_height    = ta_game_by_sym(RES_TEXTURE, material->tex_height    ? material->tex_height    : SYM_MISSING_HEIGHT);
        ta_texture *texture_metallic  = ta_game_by_sym(RES_TEXTURE, material->tex_metallic  ? material->tex_metallic  : SYM_MISSING_METALLIC);
        ta_texture *texture_normal    = ta_game_by_sym(RES_TEXTURE, material->tex_normal    ? material->tex_normal    : SYM_MISSING_NORMAL);
        ta_texture *texture_occlusion = ta_game_by_sym(RES_TEXTURE, material->tex_occlusion ? material->tex_occlusion : SYM_MISSING_OCCLUSION);
        ta_texture *texture_roughness = ta_game_by_sym(RES_TEXTURE, material->tex_roughness ? material->tex_roughness : SYM_MISSING_ROUGHNESS);

        ta_shader_set_mat4(shader, SYM_U_PROJ, &camera->projection);
        ta_shader_set_mat4(shader, SYM_U_VIEW, &camera->look_at);
        ta_shader_set_mat4(shader, SYM_U_MODEL, &transform->model);
        ta_light *lights = ta_game_resource_pool(RES_COMP_LIGHT);
        u32 lights_len = dlb_vec_len(lights);
        u32 u_lights_count = 0;
        for (u32 i = 0; i < lights_len; ++i) {
            if (!lights[i].disabled) {
                ta_shader_set_light(shader, SYM_U_LIGHTS, u_lights_count, &lights[i]);
                u_lights_count++;
            }
        }
        ta_shader_set_uint(shader, SYM_U_LIGHTS_COUNT, u_lights_count);
        ta_shader_set_vec3(shader, SYM_U_CAMERA_POS, &camera->position);
        ta_shader_set_sampler2d(shader, SYM_U_TEX_ALBEDO,    texture_albedo->gl_id);
        ta_shader_set_sampler2d(shader, SYM_U_TEX_HEIGHT,    texture_height->gl_id);
        ta_shader_set_sampler2d(shader, SYM_U_TEX_METALLIC,  texture_metallic->gl_id);
        ta_shader_set_sampler2d(shader, SYM_U_TEX_NORMAL,    texture_normal->gl_id);
        ta_shader_set_sampler2d(shader, SYM_U_TEX_OCCLUSION, texture_occlusion->gl_id);
        ta_shader_set_sampler2d(shader, SYM_U_TEX_ROUGHNESS, texture_roughness->gl_id);
        ta_shader_set_int(shader, SYM_U_DEBUG_CHANNEL, camera->debug_channel);
        ta_shader_bind(shader);
        dlb_vec_each(const char **, mesh_name, model->meshes) {
            ta_mesh *mesh = ta_game_by_sym(RES_MESH, *mesh_name);
            ta_mesh_render(mesh);
        }
        ta_shader_unbind();
    }

    if (camera->debug_normals || camera->debug_colliders) {
        ta_shader_set_mat4(tg_shader_lines, SYM_U_MODEL, &transform->model);
        ta_shader_set_mat4(tg_shader_quads, SYM_U_MODEL, &transform->model);

        if (camera->debug_normals) {
            dlb_vec_each(const char **, mesh_name, model->meshes) {
                ta_mesh *mesh = ta_game_by_sym(RES_MESH, *mesh_name);
                ta_mesh_push_normals(mesh);
                ta_primitive_render(true, false);
            }
        }
        if (camera->debug_colliders) {
            ta_rigid_body *body = ta_game_component_try(RES_COMP_RIGID_BODY, model->entity_name);
            if (body) {
                // Model space
                ta_sphere centroid_local = { 0 };
                centroid_local.center = body->centroid_local;
                centroid_local.radius = 0.05f;
                ta_primitive_push_sphere(centroid_local, TA_COLOR_MAGENTA);

                ta_rgba narrowphase_color = body->dbg_narrowphase ? TA_COLOR_MAGENTA : TA_COLOR_ORANGE;
                ta_collider_render(&body->collider, narrowphase_color);

                ta_primitive_render(true, false);

                // World space
                ta_shader_set_mat4(tg_shader_lines, SYM_U_MODEL, &MAT4_IDENT);

                ta_sphere centroid_global = { 0 };
                centroid_global.center = body->centroid_global;
                centroid_global.radius = 0.1f;
                ta_primitive_push_sphere(centroid_global, TA_COLOR_CYAN);

                ta_rgba broadphase_color = body->dbg_broadphase ? TA_COLOR_RED : TA_COLOR_GRAY4;
                ta_primitive_push_aabb(body->aabb, broadphase_color);

                ta_primitive_render(true, false);
            }
        }
    }
}

void ta_model_render_shader(ta_model *model, ta_camera *camera,
    ta_shader *shader, float alpha, float scale)
{
    DLB_ASSERT(model);
    DLB_ASSERT(camera);
    DLB_ASSERT(shader);

    ta_transform *transform = ta_game_component(RES_COMP_TRANSFORM, model->entity_name);

    ta_vec3 lerp_pos = vec3_lerp(transform->xform_prev.position,
        transform->xform.position, alpha);
    ta_vec4 lerp_orient = quat_nlerp(transform->xform_prev.orientation,
        transform->xform.orientation, alpha);

    // TODO: Multiply position by parent via mat4_mul(parent, transform)
    ta_mat4 trans = mat4_translate(lerp_pos);
    ta_mat4 rot = mat4_rotate_quat(lerp_orient);
    ta_mat4 scal = mat4_scalef(scale);
    transform->model = mat4_mul(&rot, &scal);
    transform->model = mat4_mul(&trans, &transform->model);

    ta_shader_set_mat4(shader, SYM_U_PROJ, &camera->projection);
    ta_shader_set_mat4(shader, SYM_U_VIEW, &camera->look_at);
    ta_shader_set_mat4(shader, SYM_U_MODEL, &transform->model);
    ta_shader_bind(shader);
    dlb_vec_each(const char **, mesh_name, model->meshes) {
        ta_mesh *mesh = ta_game_by_sym(RES_MESH, *mesh_name);
        ta_mesh_render(mesh);
    }
    ta_shader_unbind();
}