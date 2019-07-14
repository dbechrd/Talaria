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
#include "dlb_vector.h"

void ta_node_init(ta_node *node)
{
    if (quat_zero(node->transform.orientation)) {
        node->transform.orientation = QUAT_IDENT;
    }
    if (vec3_zero(node->transform.scale)) {
        node->transform.scale = VEC3_ONE;
    }
    node->transform_prev = node->transform;
    if (!node->material_uid) {
        node->material_uid = node->uid.scene->default_material_uid;
    }
    if (!node->mesh_group_uid) {
        node->mesh_group_uid = node->uid.scene->default_mesh_group_uid;
    }
    if (!node->rigid_body_uid) {
#if 0
        char body_uid[128] = { 0 };
        snprintf(body_uid, sizeof(body_uid) - 1, "%s_rigid_body", node->ref.uid);
        ta_rigid_body *body = ta_scene_alloc(node->ref.scene, TA_RIGID_BODY,
            INTERN(body_uid));
        body->transform.position = node->transform.position;
        body->transform.rotation = node->transform.rotation;
        ta_rigid_body_init(body);
        node->rigid_body_uid = body->ref.uid;
#endif
    }
    if (!node->aabb.extents.x) {
        ta_rigid_body *rigid_body = ta_node_rigid_body(node);
        if (rigid_body) {
            switch (rigid_body->collider.type) {
                case TA_COLLIDER_PLANE: {
                    // Infinite AABB
                    break;
                } case TA_COLLIDER_SPHERE: {
                    float radius = rigid_body->collider.data.sphere.radius;
                    node->aabb.extents.x = radius;
                    node->aabb.extents.y = radius;
                    node->aabb.extents.z = radius;
                    break;
                } case TA_COLLIDER_AABB: {
                    node->aabb = rigid_body->aabb;
                    break;
                } default: {
                    DLB_ASSERT(!"Unhandled collider type, need for broadphase");
                }
            }
        } else {
            ta_mesh_group *mesh_group = ta_node_mesh_group(node);
            node->aabb = mesh_group->aabb;
        }
    }
}

ta_material *ta_node_material(ta_node *node)
{
    if (!node->material_uid) return 0;

    // NOTE: This could cache in node->material if we want to save the hash lookup
    ta_material *mat = ta_scene_find(node->uid.scene, TA_MATERIAL, node->material_uid);
    return mat;
}

ta_mesh_group *ta_node_mesh_group(ta_node *node)
{
    if (!node->mesh_group_uid) return 0;

    // NOTE: This could cache in node->mesh_group if we want to save the hash lookup
    ta_mesh_group *mesh_group = ta_scene_find(node->uid.scene, TA_MESH_GROUP, node->mesh_group_uid);
    return mesh_group;
}

ta_rigid_body *ta_node_rigid_body(ta_node *node)
{
    if (!node->rigid_body_uid) return 0;

    // NOTE: This could cache in node->rigid_body if we want to save the hash lookup
    ta_rigid_body *rigid_body = ta_scene_find(node->uid.scene, TA_RIGID_BODY, node->rigid_body_uid);
    return rigid_body;
}

ta_button *ta_node_button(ta_node *node)
{
	if (!node->button_uid) return 0;

	// NOTE: This could cache in node->button if we want to save the hash lookup
	ta_button *button = ta_scene_find(node->uid.scene, TA_BUTTON, node->button_uid);
	return button;
}

#if 0
bool ta_node_intersect(ta_node *a, ta_node *b, ta_manifold *manifold)
{
    bool broad_phase = ta_aabb_v_aabb(&a->aabb, &b->aabb, 0);
    if (!broad_phase) {
        return false;
    }

    ta_rigid_body *ra = ta_node_rigid_body(a);
    ta_rigid_body *rb = ta_node_rigid_body(b);
    bool narrow_phase = ta_rigid_body_intersect(ra, rb, manifold);
    return narrow_phase;
}
#endif

#if 0
typedef void (entity_updater)(ta_node *base);

static entity_updater *entity_updaters[TA_ENT_COUNT] = {
    [TA_ENT_BUTTON] = &ta_ent_button_update
};
#endif

void ta_node_update(ta_node *node)
{
    node->transform_prev = node->transform;
    ta_rigid_body *body = ta_node_rigid_body(node);
    if (body) {
        node->transform.position = body->position;
        node->transform.orientation = body->orientation;
    }
#if 0
    if (entity_updaters[node->type]) {
        entity_updaters[node->type](node);
    }
#endif
}

static void ta_node_push_aabb(ta_node *node, ta_rgba color)
{
    ta_primitive_push_aabb(node->aabb, color);
}

static void ta_node_push_normals(ta_node *node)
{
    ta_mesh_group *mesh_group = ta_node_mesh_group(node);
    if (mesh_group) {
        ta_mesh_group_push_normals(mesh_group);
    }
}

void ta_node_shadow_pass(ta_node *node, ta_shader *shader, ta_mat4 *light_pv,
    float alpha)
{
    if (node->invisible || !node->cast_shadows) {
        return;
    }

    ta_mesh_group *mesh_group = ta_node_mesh_group(node);
    DLB_ASSERT(mesh_group);

    ta_vec3 lerp_pos;
    ta_quat lerp_orient;

    ta_rigid_body *body = ta_node_rigid_body(node);
    if (body) {
        lerp_pos = vec3_lerp(node->transform.position, body->position, alpha);
        lerp_orient = quat_nlerp(node->transform.orientation, body->orientation, alpha);
    } else {
        lerp_pos = vec3_lerp(node->transform_prev.position, node->transform.position, alpha);
        lerp_orient = quat_nlerp(node->transform_prev.orientation, node->transform.orientation, alpha);
    }

    // TODO: Multiply position by parent via mat4_mul(parent, transform)
    ta_mat4 trans = mat4_translate(lerp_pos);
    ta_mat4 orient = mat4_rotate_quat(lerp_orient);
    node->model = mat4_mul(&trans, &orient);

    // TODO: Allow updating model uniform without having to rebind everything
    // We can probably bind in the set functions instead of prerender, or can
    // set a "loaded" flag for each uniform (will be 0 by default, so safer than
    // "dirty" flag), and only load when changed. I don't know how expensive
    // glUniform calls are, so this may or may not matter.
    ta_shader_set_mat4(shader, SYM_U_MODEL, &node->model);
    ta_mat4 light_pvm = mat4_mul(light_pv, &node->model);
    ta_shader_set_mat4(shader, SYM_U_LIGHT_PVM, &light_pvm);
    ta_shader_prerender(shader);
    ta_mesh_group_render(mesh_group);
}

void ta_node_render(ta_node *node, ta_camera *camera, float alpha)
{
    // If invisible or all rendering disabled
    if (node->invisible || !(!camera->debug_no_mesh || camera->debug_normals || camera->debug_bounding_boxes)) {
        return;
    }

    ta_vec3 lerp_pos;
    ta_quat lerp_orient;
    ta_rigid_body *body = ta_node_rigid_body(node);
    if (body) {
        lerp_pos = vec3_lerp(node->transform.position, body->position, alpha);
        lerp_orient = quat_nlerp(node->transform.orientation, body->orientation, alpha);
    } else {
        lerp_pos = vec3_lerp(node->transform_prev.position, node->transform.position, alpha);
        lerp_orient = quat_nlerp(node->transform_prev.orientation, node->transform.orientation, alpha);
    }

    // TODO: Multiply position by parent via mat4_mul(parent, transform)
    ta_mat4 trans = mat4_translate(lerp_pos);
    ta_mat4 orient = mat4_rotate_quat(lerp_orient);
    node->model = mat4_mul(&trans, &orient);

    if (!camera->debug_no_mesh) {
        ta_material *mat = ta_node_material(node);
        ta_shader *shader = ta_material_shader(mat);
        ta_texture *texture_albedo = ta_material_texture_albedo(mat);
        ta_texture *texture_metallic = ta_material_texture_metallic(mat);
        ta_mesh_group *mesh_group = ta_node_mesh_group(node);

        // TODO: Allow some entities to not be renderable; skip them
        DLB_ASSERT(mat);
        DLB_ASSERT(shader);
        DLB_ASSERT(texture_albedo);
        DLB_ASSERT(texture_metallic);
        DLB_ASSERT(mesh_group);

        // TODO: This is going to make a zillion extranous calls
        GLenum camera_poly_mode = camera->debug_wireframe ? GL_LINE : GL_FILL;
        if (camera_poly_mode != tg_polygon_mode) {
            glPolygonMode(GL_FRONT_AND_BACK, camera_poly_mode);
            tg_polygon_mode = camera_poly_mode;
        }

        ta_shader_set_mat4(shader, SYM_U_PROJ, &camera->projection);
        ta_shader_set_mat4(shader, SYM_U_VIEW, &camera->look_at);
        ta_shader_set_mat4(shader, SYM_U_MODEL, &node->model);
        ta_shader_set_uint(shader, SYM_U_LIGHTS_COUNT, dlb_vec_len(tg_game.lights));
        int light_index = 0;
        dlb_vec_each(ta_light *, light, tg_game.lights) {
            if (light->disabled) {
                continue;
            }
            ta_shader_set_light(shader, SYM_U_LIGHTS, light_index, light);
            light_index++;
        }
        ta_shader_set_vec3(shader, SYM_U_CAMERA_POS, &camera->position);
        ta_shader_set_sampler2d(shader, SYM_U_TEX_ALBEDO, texture_albedo->gl_id);
        ta_shader_set_sampler2d(shader, SYM_U_TEX_METALLIC, texture_metallic->gl_id);
        ta_shader_bind(shader);
        ta_shader_prerender(shader);
        ta_mesh_group_render(mesh_group);
        ta_shader_unbind(shader);
    }

    ta_primitive_render();
    ta_primitive_clear();

    if (camera->debug_normals) {
        ta_shader_set_mat4(tg_shader_lines, SYM_U_MODEL, &node->model);
        ta_shader_set_mat4(tg_shader_quads, SYM_U_MODEL, &node->model);
        ta_node_push_normals(node);
        ta_primitive_render();
        ta_primitive_clear();
    }

    if (camera->debug_bounding_boxes) {
        ta_shader_set_mat4(tg_shader_lines, SYM_U_MODEL, &trans);
        ta_shader_set_mat4(tg_shader_quads, SYM_U_MODEL, &trans);
        ta_node_push_aabb(node, TA_COLOR_RED);
        ta_primitive_render();
        ta_primitive_clear();
    }
}