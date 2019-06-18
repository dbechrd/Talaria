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
    if (quat_zero(e->transform.orientation)) {
        e->transform.orientation = QUAT_IDENT;
    }
    if (vec3_zero(e->transform.scale)) {
        e->transform.scale = VEC3_ONE;
    }
    e->transform_prev = e->transform;
    if (!e->material_uid) {
        e->material_uid = e->ref.scene->default_material_uid;
    }
    if (!e->mesh_group_uid) {
        e->mesh_group_uid = e->ref.scene->default_mesh_group_uid;
    }
    if (!e->rigid_body_uid) {
#if 0
        char body_uid[128] = { 0 };
        snprintf(body_uid, sizeof(body_uid) - 1, "%s_rigid_body", e->ref.uid);
        ta_rigid_body *body = ta_scene_obj_alloc(e->ref.scene, F_TA_RIGID_BODY,
            INTERN(body_uid));
        body->transform.position = e->transform.position;
        body->transform.rotation = e->transform.rotation;
        ta_rigid_body_init(body);
        e->rigid_body_uid = body->ref.uid;
#endif
    }
    if (!e->aabb.extents.x) {
        bool use_mesh_aabb = false;
        ta_rigid_body *rigid_body = ta_entity_rigid_body(e);
        if (rigid_body) {
            switch (rigid_body->collider.type) {
                case TA_COLLIDER_PLANE: {
                    // Infinite AABB
                    break;
                } case TA_COLLIDER_SPHERE: {
                    float radius = rigid_body->collider.data.sphere.radius;
                    e->aabb.extents.x = radius;
                    e->aabb.extents.y = radius;
                    e->aabb.extents.z = radius;
                    break;
                } case TA_COLLIDER_AABB: {
                    e->aabb = rigid_body->aabb;
                    break;
                } default: {
                    DLB_ASSERT(!"Unhandled collider type, need for broadphase");
                }
            }
        } else {
            ta_mesh_group *mesh_group = ta_entity_mesh_group(e);
            e->aabb = mesh_group->aabb;
        }
    }
}

ta_material *ta_entity_material(ta_entity *e)
{
    if (!e->material_uid) return 0;

    // NOTE: This could cache in e->material if we want to save the hash lookup
    ta_material *mat = ta_scene_find(e->ref.scene, F_TA_MATERIAL, e->material_uid);
    return mat;
}

ta_mesh_group *ta_entity_mesh_group(ta_entity *e)
{
    if (!e->mesh_group_uid) return 0;

    // NOTE: This could cache in e->mesh_group if we want to save the hash lookup
    ta_mesh_group *mesh_group = ta_scene_find(e->ref.scene, F_TA_MESH_GROUP,
        e->mesh_group_uid);
    return mesh_group;
}

ta_rigid_body *ta_entity_rigid_body(ta_entity *e)
{
    if (!e->rigid_body_uid) return 0;

    // NOTE: This could cache in e->mesh_group if we want to save the hash lookup
    ta_rigid_body *rigid_body = ta_scene_find(e->ref.scene, F_TA_RIGID_BODY,
        e->rigid_body_uid);
    return rigid_body;
}
#if 0
bool ta_entity_intersect(ta_entity *a, ta_entity *b, ta_manifold *manifold)
{
    bool broad_phase = ta_aabb_v_aabb(&a->aabb, &b->aabb, 0);
    if (!broad_phase) {
        return false;
    }

    ta_rigid_body *ra = ta_entity_rigid_body(a);
    ta_rigid_body *rb = ta_entity_rigid_body(b);
    bool narrow_phase = ta_rigid_body_intersect(ra, rb, manifold);
    return narrow_phase;
}
#endif
void ta_entity_update(ta_entity *e)
{
    e->transform_prev = e->transform;
    ta_rigid_body *body = ta_entity_rigid_body(e);
    if (body) {
        e->transform.position = body->position;
        e->transform.orientation = body->orientation;
    }
}

static void ta_entity_push_aabb(ta_entity *e, ta_rgba color)
{
    ta_primitive_push_aabb(e->aabb, color);
}

static void ta_entity_push_normals(ta_entity *e)
{
    ta_mesh_group *mesh_group = ta_entity_mesh_group(e);
    if (mesh_group) {
        ta_mesh_group_push_normals(mesh_group);
    }
}

void ta_entity_render(ta_entity *e, ta_camera *camera, float alpha)
{
    if (e->invisible) {
        return;
    }

    ta_material *mat = ta_entity_material(e);
    ta_shader *shader = ta_material_shader(mat);
    ta_texture *texture_albedo = ta_material_texture_albedo(mat);
    ta_texture *texture_metallic = ta_material_texture_metallic(mat);
    ta_mesh_group *mesh_group = ta_entity_mesh_group(e);

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

    ta_vec3 lerp_pos;
    ta_quat lerp_orient;

    ta_rigid_body *body = ta_entity_rigid_body(e);
    if (body) {
        lerp_pos = vec3_lerp(e->transform.position, body->position, alpha);
        lerp_orient = quat_nlerp(e->transform.orientation, body->orientation, alpha);
    } else {
        lerp_pos = vec3_lerp(e->transform_prev.position, e->transform.position, alpha);
        lerp_orient = quat_nlerp(e->transform_prev.orientation, e->transform.orientation, alpha);
    }

    // TODO: Multiply position by parent via mat4_mul(parent, transform)
    ta_mat4 trans = mat4_translate(lerp_pos);
    ta_mat4 orient = mat4_rotate_quat(lerp_orient);
    e->model = mat4_mul(&trans, &orient);

    if (!camera->debug_no_mesh) {
        ta_shader_set_mat4(shader, SYM_U_PROJ, &camera->projection);
        ta_shader_set_mat4(shader, SYM_U_VIEW, &camera->look_at);
        ta_shader_set_mat4(shader, SYM_U_MODEL, &e->model);
        ta_shader_set_light(shader, SYM_U_SUN, tg_game.sun);
        ta_shader_set_vec3(shader, SYM_U_CAMERA_POS, &camera->position);
        ta_shader_set_sampler2d(shader, SYM_U_TEX_ALBEDO, texture_albedo->gl_id);
        ta_shader_set_sampler2d(shader, SYM_U_TEX_METALLIC, texture_metallic->gl_id);
        ta_shader_bind(shader);
        ta_shader_prerender(shader);
        ta_mesh_group_render(mesh_group);
        ta_shader_unbind(shader);
    }

    if (camera->debug_normals ||
        camera->debug_bounding_boxes)
    {
        ta_primitive_render();
        ta_primitive_clear();

        ta_shader_set_mat4(tg_shader_lines, SYM_U_MODEL, &e->model);
        ta_shader_set_mat4(tg_shader_quads, SYM_U_MODEL, &e->model);
        if (camera->debug_normals) {
            ta_entity_push_normals(e);
        }
        ta_primitive_render();
        ta_primitive_clear();

        ta_shader_set_mat4(tg_shader_lines, SYM_U_MODEL, &trans);
        ta_shader_set_mat4(tg_shader_quads, SYM_U_MODEL, &trans);
        if (camera->debug_bounding_boxes) {
            ta_entity_push_aabb(e, TA_COLOR_RED);
        }
        ta_primitive_render();
        ta_primitive_clear();

        ta_shader_set_mat4(tg_shader_lines, SYM_U_MODEL, &MAT4_IDENT);
        ta_shader_set_mat4(tg_shader_quads, SYM_U_MODEL, &MAT4_IDENT);
    }
}