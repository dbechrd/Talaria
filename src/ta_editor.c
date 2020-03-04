#include "ta_audio.h"
#include "ta_camera.h"
#include "ta_editor.h"
#include "ta_event.h"
#include "ta_font.h"
#include "ta_game.h"
#include "ta_intersect.h"
#include "ta_keybind.h"
#include "ta_light.h"
#include "ta_log.h"
#include "ta_material.h"
#include "ta_mesh.h"
#include "ta_model.h"
#include "ta_mouse.h"
#include "ta_parse.h"
#include "ta_transform.h"
#include "ta_primitive.h"
#include "ta_rigid_body.h"
#include "ta_scene.h"
#include "ta_shader.h"
#include "ta_symbol.h"
#include "ta_texture.h"
#include "ta_timer.h"
#include "ta_ui.h"
#include "ta_window.h"
#include "dlb/dlb_vector.h"
#include <float.h>

// Higher level widgets
typedef enum editor_widget {
    WIDGET_TRANSLATE,
    WIDGET_ROTATE,
    WIDGET_SCALE,
} editor_widget;

// Individual components of a widget (i.e. "handles")
typedef enum editor_gizmo {
    GIZMO_NONE,
    GIZMO_TRANSLATE_X,
    GIZMO_TRANSLATE_Y,
    GIZMO_TRANSLATE_Z,
    GIZMO_TRANSLATE_YZ,
    GIZMO_TRANSLATE_XZ,
    GIZMO_TRANSLATE_XY,
    GIZMO_TRANSLATE_VIEW,
    GIZMO_COUNT
} editor_gizmo;

static struct {
    ta_scene        scene;
    const char      *shader_editor_select;
    editor_widget   widget;
    editor_gizmo    gizmo;
    //ta_vec3         gizmo_start_hit;  // if gizmo active, starting contact point of gizmo in world space
    ta_xform        gizmo_start_xform;  // if gizmo active, starting xform of entity being transformed

    union {
        struct {
            ta_aabb hitbox1d[3];        // One-axis arrow handles
            ta_quad hitbox2d[3];        // Two-axis plane handles
            ta_aabb hitbox3d;           // Three-axis view-plane handle
        } transform;
        struct {
            int unused;
        } rotate;
        struct {
            int unused;
        } scale;
    } gizmos;

    float               widget_snap_to_grid;
    ta_ui_textbox_state *textbox_editing;
    ta_ui_textbox_state *textbox_dragging;
    const char          *selected_entity;
    const char          *status_msg;
} editor;

void ta_editor_init()
{
    ta_log_write(&tg_debug_log, SRC_EDITOR, "Loading editor scene\n");
    ta_scene_load_file(&editor.scene, "data/scene/editor.dml");
    editor.shader_editor_select = SYM_SHADER_EDITOR_SELECT;

    ta_font *font = ta_game_by_sym(RES_FONT, tg_font);
    ta_log_write(&tg_debug_log, SRC_EDITOR, "Initializing UI styles\n");
    ta_ui_init(font, &editor.textbox_editing, &editor.textbox_dragging);

    editor.widget = WIDGET_TRANSLATE;
}
void ta_editor_select_entity(const char *entity)
{
    editor.selected_entity = entity;
}
const char *ta_editor_selected_entity()
{
#if 0
    // Clear selection if entity has been deleted
    if (!ta_game_by_sym_try(tg_game.scene, RES_ENTITY,
        editor.selected_entity_name))
    {
        editor.selected_entity_name = 0;
    }
#endif
    return editor.selected_entity;
}

static editor_gizmo editor_gizmo_nearest(ta_ray *ray)
{
    editor_gizmo nearest_gizmo = GIZMO_NONE;

    switch (editor.widget) {
        case WIDGET_TRANSLATE: {
            // TODO: Cleanup
            DLB_ASSERT(editor.gizmos.transform.hitbox1d[0].center.x);

            if (!ray) {
                ta_ray fwd = ta_game_camera_ray();
                ray = &fwd;
            }

            float t;
            float t_min = FLT_MAX;
            for (int i = 0; i < 3; ++i) {
                if (ta_ray_v_aabb(ray, &editor.gizmos.transform.hitbox1d[i], &t) && t < t_min) {
                    t_min = t;
                    nearest_gizmo = GIZMO_TRANSLATE_X + i;
                }
            }

            for (int i = 0; i < 3; ++i) {
                if (ta_ray_v_quad(ray, &editor.gizmos.transform.hitbox2d[i], &t) && t < t_min) {
                    t_min = t;
                    nearest_gizmo = GIZMO_TRANSLATE_YZ + i;
                }
            }

            // NOTE: This feels more responsive when it takes precedence above the invisible arrow bboxes
            if (ta_ray_v_aabb(ray, &editor.gizmos.transform.hitbox3d, &t)) {  // && t < t_min) {
                t_min = t;
                nearest_gizmo = GIZMO_TRANSLATE_VIEW;
            }
            break;
        } case WIDGET_ROTATE: {
            break;
        } case WIDGET_SCALE: {
            break;
        }
    }

    return nearest_gizmo;
}
static void editor_gizmo_end(bool keep_changes)
{
    DLB_ASSERT(editor.gizmo);

    if (!keep_changes) {
        const char *selected_entity = ta_editor_selected_entity();
        if (selected_entity) {
            switch (editor.widget) {
                case WIDGET_TRANSLATE:
                case WIDGET_ROTATE: {
                    ta_transform *e_transform = ta_game_component(selected_entity, RES_COMP_TRANSFORM);
                    e_transform->xform = editor.gizmo_start_xform;
                    break;
                }
            }
        }
    }

    editor.gizmo = GIZMO_NONE;
#if _DEBUG
    // NOTE: These should never be used with gizmo is NONE, but invalidate for easier debugging
    //editor.gizmo_start_hit = VEC3_ZERO;
    editor.gizmo_start_xform.position = VEC3_ZERO;
    editor.gizmo_start_xform.orientation = QUAT_IDENT;
#endif
}
static void editor_command_select()
{
    DLB_ASSERT(!editor.gizmo && "Wtf.. how did you select something *while* using a gizmo??");

    if (!ta_mouse_captured()) {
        return;
    }

    ta_ray ray = ta_game_camera_ray();

    const char *selected_entity = ta_editor_selected_entity();
    if (selected_entity) {
        switch (editor.widget) {
            case WIDGET_TRANSLATE: {
                editor.gizmo = editor_gizmo_nearest(&ray);
                if (editor.gizmo) {
                    ta_transform *e_transform = ta_game_component(selected_entity, RES_COMP_TRANSFORM);
                    //ta_transform *cam_trans = ta_game_component(camera->entity, RES_COMP_TRANSFORM);
                    //float dist = vec3_len(vec3_sub(cam_trans->xform_world.position, e_transform->xform_world.position));
                    //float scale = MAX(0.5f, dist * 0.2f);
                    //float radius = scale / TA_PRIMITIVE_CONE_RADIUS_SCALE * 2.0f;

                    //editor.gizmo_start_hit = vec3_add(ray.origin, vec3_scalef(ray.direction, t));
                    editor.gizmo_start_xform = e_transform->xform;
                }
                break;
            } case WIDGET_ROTATE: {
                break;
            } case WIDGET_SCALE: {
                break;
            }
        }
    }

    if (!editor.gizmo) {
        float t = 0.0f;
        float t_min = FLT_MAX;
        const char *closest_entity = 0;

        ta_rigid_body *bodies = ta_game_resource_pool(RES_COMP_RIGID_BODY);
        dlb_vec_each(ta_rigid_body *, body, bodies) {
            switch (body->collider.type) {
                case TA_COLLIDER_PLANE: {
                    ta_transform *transform = ta_game_component(body->entity, RES_COMP_TRANSFORM);
                    ta_plane plane = body->collider.data.plane;
                    plane.center = vec3_add(plane.center, transform->xform_world.position);
                    if (ta_ray_v_plane(&ray, &plane, &t)) {
                        DLB_ASSERT(t >= 0.0f);
                        if (t < t_min) {
                            t_min = t;
                            closest_entity = body->entity;
                        }
                    }
                    break;
                } case TA_COLLIDER_SPHERE: {
                    ta_transform *transform = ta_game_component(body->entity, RES_COMP_TRANSFORM);
                    ta_sphere sphere = body->collider.data.sphere;
                    sphere.center = vec3_add(sphere.center, transform->xform_world.position);
                    if (ta_ray_v_sphere(&ray, &sphere, &t)) {
                        DLB_ASSERT(t >= 0.0f);
                        if (t < t_min) {
                            t_min = t;
                            closest_entity = body->entity;
                        }
                    }
                    break;
                } case TA_COLLIDER_OBB: {
                    ta_transform *transform = ta_game_component(body->entity, RES_COMP_TRANSFORM);
                    ta_obb obb = body->collider.data.obb;
                    obb.center = vec3_rotate_quat(obb.center, transform->xform_world.orientation);
                    obb.center = vec3_add(obb.center, transform->xform_world.position);
                    obb.orientation = quat_mul(transform->xform_world.orientation, obb.orientation);
                    if (ta_ray_v_obb(&ray, &obb, &t)) {
                        DLB_ASSERT(t >= 0.0f);
                        if (t < t_min) {
                            t_min = t;
                            closest_entity = body->entity;
                        }
                    }
                    break;
                } default: {
                    // Ignore unsupported colliders when picking
                    continue;
                }
            }
        }

        ta_light *lights = ta_game_resource_pool(RES_COMP_LIGHT);
        dlb_vec_each(ta_light *, light, lights) {
            ta_transform *transform = ta_game_component(light->entity, RES_COMP_TRANSFORM);
            ta_sphere sphere = { 0 };
            sphere.center = transform->xform_world.position;
            sphere.radius = 0.2f;
            if (ta_ray_v_sphere(&ray, &sphere, &t)) {
                DLB_ASSERT(t >= 0.0f);
                if (t < t_min) {
                    t_min = t;
                    closest_entity = light->entity;
                }
            }
        }

        if (closest_entity) {
            ta_vec3 t_pos = vec3_add(ray.origin, vec3_scalef(ray.direction, t_min));
            ta_sphere t_sphere = { 0 };
            t_sphere.center = t_pos;
            t_sphere.radius = 0.1f;
            ta_primitive_push_sphere(0, t_sphere, TA_COLOR_PINK);
        } else {
            ta_editor_select_entity(0);
        }

        if (closest_entity) {
            ta_editor_select_entity(closest_entity);
        }
    }
}
static void editor_command_select_release()
{
    if (editor.gizmo) {
        editor_gizmo_end(true);
    }
}
static void editor_command_cancel()
{
    if (editor.gizmo) {
        editor_gizmo_end(false);
    } else if (editor.textbox_editing) {
        ta_ui_textbox_cancel(editor.textbox_editing);
    } else {
        ta_game_state_set(ta_game_state_prev());
    }
}
static void editor_command_close()
{
    // TODO: If outstanding changes, prompt before closing editor
    if (editor.gizmo) {
        editor_gizmo_end(false);
    }
    if (editor.textbox_editing) {
        ta_ui_textbox_cancel(editor.textbox_editing);
    }
    ta_game_state_set(ta_game_state_prev());
}
static void editor_command_sim_pause_resume()
{
    if (ta_game_sim_running()) {
        ta_game_sim_pause();
    } else {
        ta_game_sim_resume();
    }
}
static void editor_command_sim_next()
{
    if (ta_game_sim_paused()) {
        ta_game_sim_step_n_frames(1);
    }
}
static void editor_command_sim_next_ten()
{
    if (ta_game_sim_paused()) {
        ta_game_sim_step_n_frames(10);
    }
}
static void editor_command_sim_while_held()
{
    if (ta_game_sim_paused()) {
        ta_game_sim_step_n_frames(1);
    }
}

static void ta_editor_textbox_event(ta_event *event)
{
    switch (event->type) {
        case INPUT_EVENT_TEXT_INPUT: {
            // TODO: How to actually handle codepoints?
            u32 codepoint = event->data.text_input.codepoint;
            if (codepoint >= 32 && codepoint < 127) {
                char chr = (char)codepoint;
                ta_ui_textbox_insert(editor.textbox_editing, chr);
            }
            event->handled = true;
            break;
        } case INPUT_EVENT_KEY_PRESS: {
            // Consume all unhandled keystrokes when text editor is active
            //if (event->data.key_press.key == GLFW_KEY_ENTER) {
            //    ta_ui_textbox_insert(editor.active_textbox, '\n');
            //}
            event->handled = true;
            break;
        } case INPUT_EVENT_KEY_RELEASE: {
            // Consume all unhandled keystrokes when text editor is active
            event->handled = true;
            break;
        }
    }
}
void ta_editor_event(ta_event *event)
{
    if (editor.textbox_editing) {
        ta_editor_textbox_event(event);
    }
}

void ta_editor_update_widgets()
{
    const char *selected_entity = ta_editor_selected_entity();
    if (!selected_entity) {
        return;
    }

    ta_camera *camera = ta_game_camera();
    ta_transform *e_transform = ta_game_component(selected_entity, RES_COMP_TRANSFORM);
    ta_transform *cam_trans = ta_game_component(camera->entity, RES_COMP_TRANSFORM);
    float dist = vec3_len(vec3_sub(cam_trans->xform_world.position, e_transform->xform_world.position));
    float scale = MAX(0.5f, dist * 0.2f);
    float radius = scale / TA_PRIMITIVE_CONE_RADIUS_SCALE * 2.0f;

    switch (editor.widget) {
        case WIDGET_TRANSLATE: {
            // One-axis arrow handles
            float midpoint1d = scale * 0.5f;
            float extent1d = scale - midpoint1d;
            editor.gizmos.transform.hitbox1d[0].center = vec3_add(e_transform->xform_world.position, vec3_scalef(VEC3_X, midpoint1d));
            editor.gizmos.transform.hitbox1d[0].extents.x = extent1d;
            editor.gizmos.transform.hitbox1d[0].extents.y = radius;
            editor.gizmos.transform.hitbox1d[0].extents.z = radius;

            editor.gizmos.transform.hitbox1d[1].center = vec3_add(e_transform->xform_world.position, vec3_scalef(VEC3_Y, midpoint1d));
            editor.gizmos.transform.hitbox1d[1].extents.x = radius;
            editor.gizmos.transform.hitbox1d[1].extents.y = extent1d;
            editor.gizmos.transform.hitbox1d[1].extents.z = radius;

            editor.gizmos.transform.hitbox1d[2].center = vec3_add(e_transform->xform_world.position, vec3_scalef(VEC3_Z, midpoint1d));
            editor.gizmos.transform.hitbox1d[2].extents.x = radius;
            editor.gizmos.transform.hitbox1d[2].extents.y = radius;
            editor.gizmos.transform.hitbox1d[2].extents.z = extent1d;

            // Two-axis plane handles
            float midpoint2d = scale * 0.5f;
            float radius_quad = radius * 1.2f;
            editor.gizmos.transform.hitbox2d[0].center = e_transform->xform_world.position;
            editor.gizmos.transform.hitbox2d[0].center.y += midpoint2d;
            editor.gizmos.transform.hitbox2d[0].center.z += midpoint2d;
            editor.gizmos.transform.hitbox2d[0].extents.x = radius_quad;
            editor.gizmos.transform.hitbox2d[0].extents.y = radius_quad;
            editor.gizmos.transform.hitbox2d[0].orientation = quat_from_axis_angle(VEC3_Y, 90.0f);

            editor.gizmos.transform.hitbox2d[1].center = e_transform->xform_world.position;
            editor.gizmos.transform.hitbox2d[1].center.x += midpoint2d;
            editor.gizmos.transform.hitbox2d[1].center.z += midpoint2d;
            editor.gizmos.transform.hitbox2d[1].extents.x = radius_quad;
            editor.gizmos.transform.hitbox2d[1].extents.y = radius_quad;
            editor.gizmos.transform.hitbox2d[1].orientation = quat_from_axis_angle(VEC3_X, -90.0f);

            editor.gizmos.transform.hitbox2d[2].center = e_transform->xform_world.position;
            editor.gizmos.transform.hitbox2d[2].center.x += midpoint2d;
            editor.gizmos.transform.hitbox2d[2].center.y += midpoint2d;
            editor.gizmos.transform.hitbox2d[2].extents.x = radius_quad;
            editor.gizmos.transform.hitbox2d[2].extents.y = radius_quad;
            editor.gizmos.transform.hitbox2d[2].orientation = QUAT_IDENT;

            // Three-axis view-plane handle
            editor.gizmos.transform.hitbox3d.center = e_transform->xform_world.position;
            editor.gizmos.transform.hitbox3d.extents.x = radius;
            editor.gizmos.transform.hitbox3d.extents.y = radius;
            editor.gizmos.transform.hitbox3d.extents.z = radius;

            // TODO: Would ray_vs_line_closest() be a better way to check this?
            ta_ray ray = ta_game_camera_ray();
            ta_plane plane = { 0 };
            plane.center = e_transform->xform_world.position;

            switch (editor.gizmo) {
                case GIZMO_TRANSLATE_X: {
                    plane.normal.y = cam_trans->xform_world.position.y - e_transform->xform_world.position.y;
                    plane.normal.z = cam_trans->xform_world.position.z - e_transform->xform_world.position.z;
                    plane.normal = vec3_normalize(plane.normal);

                    float t;
                    if (ta_ray_v_plane(&ray, &plane, &t)) {
                        ta_vec3 contact = vec3_add(ray.origin, vec3_scalef(ray.direction, t));
                        e_transform->xform.position.x = contact.x - scale * 0.5f;
                        if (editor.widget_snap_to_grid) {
                            e_transform->xform.position.x -= (float)fmod(e_transform->xform.position.x, editor.widget_snap_to_grid);
                        }
                    }
                    break;
                } case GIZMO_TRANSLATE_Y: {
                    plane.normal.x = cam_trans->xform_world.position.x - e_transform->xform_world.position.x;
                    plane.normal.z = cam_trans->xform_world.position.z - e_transform->xform_world.position.z;
                    plane.normal = vec3_normalize(plane.normal);

                    float t;
                    if (ta_ray_v_plane(&ray, &plane, &t)) {
                        ta_vec3 contact = vec3_add(ray.origin, vec3_scalef(ray.direction, t));
                        e_transform->xform.position.y = contact.y - scale * 0.5f;
                        if (editor.widget_snap_to_grid) {
                            e_transform->xform.position.y -= (float)fmod(e_transform->xform.position.y, editor.widget_snap_to_grid);
                        }
                    }
                    break;
                } case GIZMO_TRANSLATE_Z: {
                    plane.normal.x = cam_trans->xform_world.position.x - e_transform->xform_world.position.x;
                    plane.normal.y = cam_trans->xform_world.position.y - e_transform->xform_world.position.y;
                    plane.normal = vec3_normalize(plane.normal);

                    float t;
                    if (ta_ray_v_plane(&ray, &plane, &t)) {
                        ta_vec3 contact = vec3_add(ray.origin, vec3_scalef(ray.direction, t));
                        e_transform->xform.position.z = contact.z - scale * 0.5f;
                        if (editor.widget_snap_to_grid) {
                            e_transform->xform.position.z -= (float)fmod(e_transform->xform.position.z, editor.widget_snap_to_grid);
                        }
                    }
                    break;
                } case GIZMO_TRANSLATE_YZ: {
                    plane.normal.x = 1.0f;

                    float t;
                    if (ta_ray_v_plane(&ray, &plane, &t)) {
                        ta_vec3 contact = vec3_add(ray.origin, vec3_scalef(ray.direction, t));
                        e_transform->xform.position.y = contact.y - midpoint2d;
                        e_transform->xform.position.z = contact.z - midpoint2d;
                        if (editor.widget_snap_to_grid) {
                            e_transform->xform.position.y -= (float)fmod(e_transform->xform.position.y, editor.widget_snap_to_grid);
                            e_transform->xform.position.z -= (float)fmod(e_transform->xform.position.z, editor.widget_snap_to_grid);
                        }
                    }
                    break;
                } case GIZMO_TRANSLATE_XZ: {
                    plane.normal.y = 1.0f;

                    float t;
                    if (ta_ray_v_plane(&ray, &plane, &t)) {
                        ta_vec3 contact = vec3_add(ray.origin, vec3_scalef(ray.direction, t));
                        e_transform->xform.position.x = contact.x - midpoint2d;
                        e_transform->xform.position.z = contact.z - midpoint2d;
                        if (editor.widget_snap_to_grid) {
                            e_transform->xform.position.x -= (float)fmod(e_transform->xform.position.x, editor.widget_snap_to_grid);
                            e_transform->xform.position.z -= (float)fmod(e_transform->xform.position.z, editor.widget_snap_to_grid);
                        }
                    }
                    break;
                } case GIZMO_TRANSLATE_XY: {
                    plane.normal.z = 1.0f;

                    float t;
                    if (ta_ray_v_plane(&ray, &plane, &t)) {
                        ta_vec3 contact = vec3_add(ray.origin, vec3_scalef(ray.direction, t));
                        e_transform->xform.position.x = contact.x - midpoint2d;
                        e_transform->xform.position.y = contact.y - midpoint2d;
                        if (editor.widget_snap_to_grid) {
                            e_transform->xform.position.x -= (float)fmod(e_transform->xform.position.x, editor.widget_snap_to_grid);
                            e_transform->xform.position.y -= (float)fmod(e_transform->xform.position.y, editor.widget_snap_to_grid);
                        }
                    }
                    break;
                } case GIZMO_TRANSLATE_VIEW: {
                    plane.normal = vec3_neg(ray.direction);
                    plane.normal = vec3_normalize(plane.normal);

                    float t;
                    if (ta_ray_v_plane(&ray, &plane, &t)) {
                        ta_vec3 contact = vec3_add(ray.origin, vec3_scalef(ray.direction, t));

                        // TODO: Handle view-plane translation (may not be possible while camera is rotating?)
                        ta_sphere cs = { 0 };
                        cs.center = contact;
                        cs.radius = 0.1f;
                        ta_primitive_push_sphere(0, cs, TA_COLOR_YELLOW);
                        //e_transform->xform.position.x = contact.x - scale * 0.5f;
                        //e_transform->xform.position.y = contact.y - scale * 0.5f;
                        //e_transform->xform.position.z = contact.z - scale * 0.5f;
                        //if (editor.widget_snap_to_grid) {
                        //    e_transform->xform.position.x -= (float)fmod(e_transform->xform.position.x, editor.widget_snap_to_grid);
                        //    e_transform->xform.position.y -= (float)fmod(e_transform->xform.position.y, editor.widget_snap_to_grid);
                        //    e_transform->xform.position.z -= (float)fmod(e_transform->xform.position.z, editor.widget_snap_to_grid);
                        //}
                    }
                    break;
                }
            }
            break;
        } case WIDGET_ROTATE: {
            // TODO: Update this widget
            break;
        } case WIDGET_SCALE: {
            // TODO: Update this widget
            break;
        }
    }
}
void ta_editor_draw_world()
{
    // Grid and world axes
    ta_primitive_push_grid(0, VEC3_ZERO, VEC3_Y, 1000.0f, 1.0f, TA_COLOR_GRAY3);
    ta_primitive_push_axes_arrow(0, VEC3_ZERO, QUAT_IDENT, 0.3f);
    ta_primitive_render(true, false);

    const char *selected_entity = ta_editor_selected_entity();
    if (selected_entity) {
        ta_camera *camera = ta_game_camera();

        // Render selected entity as yellow wireframes
        ta_model *e_model = ta_game_component_try(selected_entity, RES_COMP_MODEL);
        if (e_model) {
            ta_shader *shader = ta_scene_find(&editor.scene, RES_SHADER, SYM(editor.shader_editor_select));
            ta_rgba wire_color = TA_COLOR_YELLOW;
            double seconds = ta_timer_elapsed_sec();
            double sine = sin(seconds * 4.0) * 0.5 + 0.5;
            wire_color.a = (float)(0.25 * (sine * sine) + 0.02);
            if (camera->debug_no_mesh) {
                wire_color.a = 0.05f;
            }
            ta_shader_set_vec4(shader, SYM_U_COLOR, (ta_vec4 *)&wire_color);

            glClear(GL_DEPTH_BUFFER_BIT);
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            ta_model_render_shader(e_model, camera, shader);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }

        // Render active widget's gizmos
        ta_transform *e_transform = ta_game_component(selected_entity, RES_COMP_TRANSFORM);
        ta_transform *cam_trans = ta_game_component(camera->entity, RES_COMP_TRANSFORM);
        float dist = vec3_len(vec3_sub(cam_trans->xform_world.position, e_transform->xform_world.position));
        float scale = MAX(0.5f, dist * 0.2f);

        switch (editor.widget) {
            case WIDGET_TRANSLATE: {
                //--------------------------------------------------------
                // Render passive gizmo details
                //--------------------------------------------------------
                static const ta_rgba gizmo_color[GIZMO_COUNT][2] = {
                    // Passive gizmo colors
                    [GIZMO_TRANSLATE_X]    [0] = { 0.7f, 0.0f, 0.0f, 1.0f },  // DARK_RED
                    [GIZMO_TRANSLATE_Y]    [0] = { 0.0f, 0.7f, 0.0f, 1.0f },  // DARK_GREEN
                    [GIZMO_TRANSLATE_Z]    [0] = { 0.0f, 0.0f, 0.7f, 1.0f },  // DARK_BLUE
                    [GIZMO_TRANSLATE_YZ]   [0] = { 0.7f, 0.0f, 0.0f, 0.7f },  // DARK_RED_ALPHA
                    [GIZMO_TRANSLATE_XZ]   [0] = { 0.0f, 0.7f, 0.0f, 0.7f },  // DARK_GREEN_ALPHA
                    [GIZMO_TRANSLATE_XY]   [0] = { 0.0f, 0.0f, 0.7f, 0.7f },  // DARK_BLUE_ALPHA
                    [GIZMO_TRANSLATE_VIEW] [0] = { 0.6f, 0.6f, 0.6f, 1.0f },  // TA_COLOR_GRAY6
                    // Active gizmo colors (highlights)
                    [GIZMO_TRANSLATE_X]    [1] = { 1.0f, 0.0f, 0.0f, 1.0f },  // TA_COLOR_RED,
                    [GIZMO_TRANSLATE_Y]    [1] = { 0.0f, 1.0f, 0.0f, 1.0f },  // TA_COLOR_GREEN,
                    [GIZMO_TRANSLATE_Z]    [1] = { 0.0f, 0.0f, 1.0f, 1.0f },  // TA_COLOR_BLUE,
                    [GIZMO_TRANSLATE_YZ]   [1] = { 1.0f, 0.0f, 0.0f, 1.0f },  // TA_COLOR_RED,
                    [GIZMO_TRANSLATE_XZ]   [1] = { 0.0f, 1.0f, 0.0f, 1.0f },  // TA_COLOR_GREEN,
                    [GIZMO_TRANSLATE_XY]   [1] = { 0.0f, 0.0f, 1.0f, 1.0f },  // TA_COLOR_BLUE,
                    [GIZMO_TRANSLATE_VIEW] [1] = { 1.0f, 1.0f, 1.0f, 1.0f },  // TA_COLOR_WHITE
                };

                // Highlight active gizmo, or nearest gizmo if none active
#define GIZMO_COLOR(gzmo) (gizmo_color[gzmo][editor.gizmo == gzmo || (!editor.gizmo && nearest_gizmo == gzmo)])
                {
                    editor_gizmo nearest_gizmo = editor_gizmo_nearest(0);

                    // 1D handles
                    ta_primitive_push_axes_arrow_color(0, e_transform->xform_world.position, QUAT_IDENT, scale,
                        GIZMO_COLOR(GIZMO_TRANSLATE_X), GIZMO_COLOR(GIZMO_TRANSLATE_Y), GIZMO_COLOR(GIZMO_TRANSLATE_Z));

                    // 2D handles
                    ta_primitive_push_quad(0, editor.gizmos.transform.hitbox2d[0], GIZMO_COLOR(GIZMO_TRANSLATE_YZ));
                    ta_primitive_push_quad(0, editor.gizmos.transform.hitbox2d[1], GIZMO_COLOR(GIZMO_TRANSLATE_XZ));
                    ta_primitive_push_quad(0, editor.gizmos.transform.hitbox2d[2], GIZMO_COLOR(GIZMO_TRANSLATE_XY));

                    // 3D handle
                    ta_primitive_push_aabb(0, editor.gizmos.transform.hitbox3d, GIZMO_COLOR(GIZMO_TRANSLATE_VIEW));
                }
#undef GIZMO_COLOR

                // Starting xform
                ta_primitive_push_axes_arrow(0, editor.gizmo_start_xform.position, editor.gizmo_start_xform.orientation, 0.5f);

                //--------------------------------------------------------
                // Render active gizmo details
                //--------------------------------------------------------
                ta_line_3d x_axis = { 0 };
                x_axis.p0 = e_transform->xform_world.position;
                x_axis.p1 = e_transform->xform_world.position;
                x_axis.p0.x = cam_trans->xform_world.position.x - 10000.0f;
                x_axis.p1.x = cam_trans->xform_world.position.x + 10000.0f;

                ta_line_3d y_axis = { 0 };
                y_axis.p0 = e_transform->xform_world.position;
                y_axis.p1 = e_transform->xform_world.position;
                y_axis.p0.y = cam_trans->xform_world.position.y - 10000.0f;
                y_axis.p1.y = cam_trans->xform_world.position.y + 10000.0f;

                ta_line_3d z_axis = { 0 };
                z_axis.p0 = e_transform->xform_world.position;
                z_axis.p1 = e_transform->xform_world.position;
                z_axis.p0.z = cam_trans->xform_world.position.z - 10000.0f;
                z_axis.p1.z = cam_trans->xform_world.position.z + 10000.0f;

                switch (editor.gizmo) {
                    case GIZMO_TRANSLATE_X: {
                        ta_primitive_push_line_3d(0, x_axis, TA_COLOR_RED, TA_COLOR_RED);
                        ta_primitive_push_arrow(0, e_transform->xform_world.position, vec3_scalef(VEC3_X, scale), TA_COLOR_RED);
                        break;
                    } case GIZMO_TRANSLATE_Y: {
                        ta_primitive_push_line_3d(0, y_axis, TA_COLOR_GREEN, TA_COLOR_GREEN);
                        ta_primitive_push_arrow(0, e_transform->xform_world.position, vec3_scalef(VEC3_Y, scale), TA_COLOR_GREEN);
                        break;
                    } case GIZMO_TRANSLATE_Z: {
                        ta_primitive_push_line_3d(0, z_axis, TA_COLOR_BLUE, TA_COLOR_BLUE);
                        ta_primitive_push_arrow(0, e_transform->xform_world.position, vec3_scalef(VEC3_Z, scale), TA_COLOR_BLUE);
                        break;
                    } case GIZMO_TRANSLATE_YZ: {
                        ta_primitive_push_line_3d(0, y_axis, TA_COLOR_GREEN, TA_COLOR_GREEN);
                        ta_primitive_push_line_3d(0, z_axis, TA_COLOR_BLUE, TA_COLOR_BLUE);
                        ta_primitive_push_quad(0, editor.gizmos.transform.hitbox2d[0], TA_COLOR_RED);
                        break;
                    } case GIZMO_TRANSLATE_XZ: {
                        ta_primitive_push_line_3d(0, x_axis, TA_COLOR_RED, TA_COLOR_RED);
                        ta_primitive_push_line_3d(0, z_axis, TA_COLOR_BLUE, TA_COLOR_BLUE);
                        ta_primitive_push_quad(0, editor.gizmos.transform.hitbox2d[1], TA_COLOR_GREEN);
                        break;
                    } case GIZMO_TRANSLATE_XY: {
                        ta_primitive_push_line_3d(0, x_axis, TA_COLOR_RED, TA_COLOR_RED);
                        ta_primitive_push_line_3d(0, y_axis, TA_COLOR_GREEN, TA_COLOR_GREEN);
                        ta_primitive_push_quad(0, editor.gizmos.transform.hitbox2d[2], TA_COLOR_BLUE);
                        break;
                    } case GIZMO_TRANSLATE_VIEW: {
                        ta_primitive_push_line_3d(0, x_axis, TA_COLOR_RED, TA_COLOR_RED);
                        ta_primitive_push_line_3d(0, y_axis, TA_COLOR_GREEN, TA_COLOR_GREEN);
                        ta_primitive_push_line_3d(0, z_axis, TA_COLOR_BLUE, TA_COLOR_BLUE);
                        break;
                    }
                }
                break;
            } case WIDGET_ROTATE: {
                ta_sphere sphere = { 0 };
                sphere.center = e_transform->xform_world.position;
                sphere.radius = scale;
                ta_primitive_push_rgb_sphere(0, sphere);
                break;
            } case WIDGET_SCALE: {
                ta_primitive_push_axes_cube(0, e_transform->xform_world.position, scale);
                break;
            }
        }

        glClear(GL_DEPTH_BUFFER_BIT);
        ta_primitive_render(true, false);
    }
}

static void ui_scene_panel()
{
    static ta_ui_panel_state scene_panel = { 0 };
    ta_ui_panel_begin(&scene_panel, TA_UI_AUTOSIZE);

    ta_ui_row_begin();
    static ta_ui_panel_state label_panel = { 0 };
    ta_ui_panel_begin(&label_panel, TA_UI_AUTOSIZE);
    ta_ui_label(CSTR("Scene"));
    ta_ui_label(CSTR("Simulation"));
    ta_ui_label(CSTR("V-Sync"));
    ta_ui_label(CSTR("Audio"));
    ta_ui_label(CSTR("Volume"));
    ta_ui_panel_end();

    static ta_ui_panel_state button_panel = { 0 };
    ta_ui_panel_begin(&button_panel, TA_UI_AUTOSIZE);
    ta_ui_next_bg_color(UI_STATE_NONE, 0.0f, 0.5f, 0.0f, 0.9f);
    ta_ui_next_bg_color(UI_STATE_INTERACT, 0.0f, 0.7f, 0.0f, 0.9f);
    if (ta_ui_button(CSTR("Save"))) {
        ta_game_save();
    }

    ta_ui_row_begin();
    if (ta_game_sim_running()) {
        ta_ui_next_bg_color(UI_STATE_NONE, 0.0f, 0.5f, 0.0f, 0.9f);
        ta_ui_next_bg_color(UI_STATE_INTERACT, 0.0f, 0.7f, 0.0f, 0.9f);
        if (ta_ui_button(CSTR("Running"))) {
            ta_game_sim_pause();
        }
    } else {
        ta_ui_next_bg_color(UI_STATE_NONE, 0.7f, 0.0f, 0.0f, 0.9f);
        ta_ui_next_bg_color(UI_STATE_INTERACT, 0.9f, 0.0f, 0.0f, 0.9f);
        if (ta_ui_button(CSTR("Paused"))) {
            ta_game_sim_resume();
        }
        ta_ui_next_bg_color(UI_STATE_NONE, 0.7f, 0.4f, 0.0f, 0.9f);
        ta_ui_next_bg_color(UI_STATE_INTERACT, 0.9f, 0.4f, 0.0f, 0.9f);
        if (ta_ui_button(CSTR("Next (1)"))) {
            ta_game_sim_step_n_frames(1);
        }
        ta_ui_next_bg_color(UI_STATE_NONE, 0.7f, 0.4f, 0.0f, 0.9f);
        ta_ui_next_bg_color(UI_STATE_INTERACT, 0.9f, 0.4f, 0.0f, 0.9f);
        if (ta_ui_button(CSTR("Next (10)"))) {
            ta_game_sim_step_n_frames(10);
        }
    }

    ta_ui_row_begin();
    if (ta_window_vsync(tg_window)) {
        ta_ui_next_bg_color(UI_STATE_NONE, 0.0f, 0.5f, 0.0f, 0.9f);
        ta_ui_next_bg_color(UI_STATE_INTERACT, 0.0f, 0.7f, 0.0f, 0.9f);
        if (ta_ui_button(CSTR("On"))) {
            ta_window_set_vsync(tg_window, false);
        }
    } else {
        ta_ui_next_bg_color(UI_STATE_NONE, 0.7f, 0.0f, 0.0f, 0.9f);
        ta_ui_next_bg_color(UI_STATE_INTERACT, 0.9f, 0.0f, 0.0f, 0.9f);
        if (ta_ui_button(CSTR("Off"))) {
            ta_window_set_vsync(tg_window, true);
        }
    }

    ta_ui_row_begin();
    if (tg_audio_listener.mute) {
        ta_ui_next_bg_color(UI_STATE_NONE, 0.7f, 0.0f, 0.0f, 0.9f);
        ta_ui_next_bg_color(UI_STATE_INTERACT, 0.9f, 0.0f, 0.0f, 0.9f);
        if (ta_ui_button(CSTR("Unmute"))) {
            ta_audio_listener_unmute(&tg_audio_listener);
        }
    } else {
        ta_ui_next_bg_color(UI_STATE_NONE, 0.0f, 0.5f, 0.0f, 0.9f);
        ta_ui_next_bg_color(UI_STATE_INTERACT, 0.0f, 0.7f, 0.0f, 0.9f);
        if (ta_ui_button(CSTR("Mute"))) {
            ta_audio_listener_mute(&tg_audio_listener);
        }
    }

    ta_ui_row_begin();
    // TODO: Make this a drag float
    static ta_ui_textbox_state volume_editor = { 0 };
    float volume = ta_audio_listener_get_volume(&tg_audio_listener);
    ta_ui_textbox_float(&volume, &volume_editor, 0);
    volume = clampf(volume, 0.0f, 1.0f);
    ta_audio_listener_set_volume(&tg_audio_listener, volume);
    ta_ui_panel_end();

    ta_ui_panel_end();
}
static void ui_texture_face(ta_texture *texture, int face, int resolution)
{
    ta_ui_next_margin(0, 0, 0, 0);
    ta_ui_next_size(resolution, resolution);
    ta_ui_next_pad(0, 0, 0, 0);
    ta_ui_image(texture, face);
}
static void ui_node_panel()
{
    //ta_ui_next_margin(2, 2, 0, 0);
    static ta_ui_panel_state node_panel = { 0 };
    ta_ui_panel_begin(&node_panel, TA_UI_AUTOSIZE);

    ta_ui_row_begin();
    ta_ui_next_size(200, 17);
    static ta_ui_textbox_state search_box = { 0 };
    if (ta_ui_textbox(0, 0, &search_box, 0)) {

    }
    ta_ui_next_margin(4, 1, 0, 0);
    if (ta_ui_button(CSTR("Clear"))) {
        ta_ui_textbox_clear(&search_box);
    }

    // TODO: Accelerate search (e.g. trie) if it gets slow
    size_t query_len = dlb_vec_len(search_box.buffer);
    if (query_len) {
        static const char **search_results = 0;
        dlb_vec_clear(search_results);
        ta_transform *transforms = ta_game_resource_pool(RES_COMP_TRANSFORM);
        dlb_vec_each(ta_transform *, transform, transforms) {
            //if (!strncmp(transform->entity, search_box.buffer, query_len)) {
            if (strstr(transform->entity, search_box.buffer)) {
                dlb_vec_push(search_results, transform->entity);
            }
        }
        dlb_vec_each(const char **, result, search_results) {
            ta_ui_row_begin();
            ta_ui_next_size(200, 0);
            if (ta_ui_button(SYM(*result))) {
                ta_editor_select_entity(*result);
            }
        }
    }

    const int header_width = 300;
    const int label_width = 150;
    const char *entity = ta_editor_selected_entity();
    if (!entity) {
        //ta_ui_spacer(0, 2);
        ta_ui_row_begin();
        ta_ui_next_size(label_width, 0);
        ta_ui_label(CSTR("name:"));
        ta_ui_label(CSTR("< nothing selected >"));
        ta_ui_panel_end();
        return;
    }

#if 0
    static ta_text_entry *uid_editor = 0;
    if (uid_editor) {
        ta_ui_next_size(100, 0);
        //ta_ui_next_pad(4, 1, 4, 1);
        ta_ui_textbox(uid_editor);
        //ta_ui_next_margin(4, 0, 0, 0);
        //ta_ui_next_pad(4, 1, 4, 1);
        ta_ui_next_bg_color(UI_STATE_INTERACT, 0.0f, 0.8f, 0.0f, 1.0f);
        if (ta_ui_label(CSTR("Save"))) {
            ta_text_entry_submit(uid_editor);
        }
        if (ta_text_entry_submitted(uid_editor)) {
            size_t text_len = 0;
            char *text = ta_text_entry_text(uid_editor, &text_len);

#if 0
            // All of this logic is specific to changing UIDs
            if (text_len)
            {
                const char **name = dlb_pool_by_id(
                    &tg_game.scene->resource_names[RES_ENTITY], entity_id);

                dlb_hash_delete(&tg_game.scene->id_by_name[RES_ENTITY], SYM(*name));
                // NOTE: This should be safe so long as nothing else holds
                // pointers to resource names.
                dlb_symbol_free(*name);

                *name = ta_symbol_intern(text, text_len);
                dlb_hash_insert(&tg_game.scene->id_by_name[RES_ENTITY], SYM(*name),
                    (void *)entity_id);

                ta_text_entry_free(&uid_editor);
            } else {
                ta_text_entry_reject(uid_editor);
            }
#else
            // TODO: Find some way to persist name changes.. we can't let
            // the user change a GUID that everything else holds a ptr to.
            ta_text_entry_free(&uid_editor);
#endif
        } else if (ta_text_entry_canceled(uid_editor)) {
            ta_text_entry_free(&uid_editor);
        }
    } else {
        //ta_ui_next_pad(4, 1, 4, 1);
        if (ta_ui_label(entity)) {
            DLB_ASSERT(!uid_editor);
            uid_editor = ta_text_entry_init();
            ta_text_entry_set_text(uid_editor, SYM(entity));
            ta_text_entry_focus(uid_editor);
        }
    }
#endif

    ta_transform *transform = ta_game_component(entity, RES_COMP_TRANSFORM);
    ta_ui_row_begin();
    ta_ui_next_size(label_width, 0);
    ta_ui_label(CSTR("Entity:"));
    ta_ui_label(SYM(transform->entity));

    ta_ui_row_end();
    ta_ui_next_size(header_width, 0);
    ta_ui_next_bg_color(UI_STATE_ALL, 0.0f, 0.5f, 0.7f, 1.0f);
    static bool transform_expanded = false;
    ta_ui_toggle_button(CSTR("Transform"), &transform_expanded);

    if (transform_expanded) {
        ta_ui_row_begin();
        ta_ui_next_size(label_width, 0);
        ta_ui_label(CSTR("position:"));
        static ta_ui_textbox_vec3_state textbox = { 0 };
        ta_ui_textbox_vec3(&transform->xform.position, &textbox, false, false, true);

        ta_ui_row_begin();
        ta_ui_next_size(label_width, 0);
        ta_ui_label(CSTR("orientation:"));
        // TODO: Can't hand edit quaternions.. they need to be normalized and
        // the components need to be in the range [0.0, 1.0]. Let's create a
        // ta_ui_label_vec4, then figure out how to edit rotations (Euler XYZ).
        static ta_ui_textbox_vec4_state orient_editors = { 0 };
        ta_ui_textbox_vec4(&transform->xform.orientation, &orient_editors, true, false, true);

        char text[256] = { 0 };
        int text_len = 0;
        ta_ui_row_begin();
        ta_ui_next_size(label_width, 0);
        ta_ui_label(CSTR("position_world:"));
        text_len = snprintf(CSTR(text),
            "%.3f, %.3f, %.3f",
            transform->xform_world.position.x,
            transform->xform_world.position.y,
            transform->xform_world.position.z);
        DLB_ASSERT(text_len < sizeof(text));
        ta_ui_label(CSTR(text));

        ta_ui_row_begin();
        ta_ui_next_size(label_width, 0);
        ta_ui_label(CSTR("orientation_world:"));
        text_len = snprintf(CSTR(text),
            "%.3f, %.3f, %.3f, %.3f",
            transform->xform_world.orientation.x,
            transform->xform_world.orientation.y,
            transform->xform_world.orientation.z,
            transform->xform_world.orientation.w);
        DLB_ASSERT(text_len < sizeof(text));
        ta_ui_label(CSTR(text));
    }

    ta_ui_row_end();
    ta_ui_next_size(header_width, 0);
    ta_ui_next_bg_color(UI_STATE_ALL, 0.0f, 0.5f, 0.7f, 1.0f);
    static bool model_expanded = false;
    ta_ui_toggle_button(CSTR("Model"), &model_expanded);

    if (model_expanded) {
        ta_model *model = ta_game_component_try(entity, RES_COMP_MODEL);
        if (model) {
            // List all of the model pieces
            dlb_vec_each(ta_piece *, piece, model->pieces) {
                ta_ui_row_begin();
                ta_ui_next_size(label_width, 0);
                ta_ui_label(CSTR("mesh:"));
                ta_ui_label(SYM(piece->mesh));
                ta_ui_row_begin();
                ta_ui_next_size(label_width, 0);
                ta_ui_label(CSTR("material:"));
                ta_ui_label(SYM(piece->material));
            }

            ta_ui_row_begin();
            ta_ui_next_size(label_width, 0);
            ta_ui_label(CSTR("visible:"));
            ta_ui_next_pad(0, 0, 0, 0);
            ta_ui_toggle_button_begin(TA_UI_AUTOSIZE);
            if (!model->invisible) {
                ta_ui_next_margin(0, 0, 0, 0);
                ta_ui_next_bg_color(UI_STATE_NONE, 0.0f, 0.5f, 0.0f, 0.9f);
                ta_ui_next_bg_color(UI_STATE_INTERACT, 0.0f, 0.7f, 0.0f, 0.9f);
                ta_ui_label(CSTR("True"));
            } else {
                ta_ui_next_margin(0, 0, 0, 0);
                ta_ui_next_bg_color(UI_STATE_NONE, 0.7f, 0.0f, 0.0f, 0.9f);
                ta_ui_next_bg_color(UI_STATE_INTERACT, 0.9f, 0.0f, 0.0f, 0.9f);
                ta_ui_label(CSTR("False"));
            }
            ta_ui_toggle_button_end(&model->invisible);

            ta_ui_row_begin();
            ta_ui_next_size(label_width, 0);
            ta_ui_label(CSTR("cast shadows:"));
            if (!model->invisible) {
                ta_ui_next_pad(0, 0, 0, 0);
                ta_ui_toggle_button_begin(TA_UI_AUTOSIZE);
                if (model->cast_shadows) {
                    ta_ui_next_margin(0, 0, 0, 0);
                    ta_ui_next_bg_color(UI_STATE_NONE, 0.0f, 0.5f, 0.0f, 0.9f);
                    ta_ui_next_bg_color(UI_STATE_INTERACT, 0.0f, 0.7f, 0.0f, 0.9f);
                    ta_ui_label(CSTR("True"));
                } else {
                    ta_ui_next_margin(0, 0, 0, 0);
                    ta_ui_next_bg_color(UI_STATE_NONE, 0.7f, 0.0f, 0.0f, 0.9f);
                    ta_ui_next_bg_color(UI_STATE_INTERACT, 0.9f, 0.0f, 0.0f, 0.9f);
                    ta_ui_label(CSTR("False"));
                }
                ta_ui_toggle_button_end(&model->cast_shadows);
            } else {
                ta_ui_label(CSTR("n/a"));
                ta_ui_label(CSTR("[?]"));
                if (ta_ui_last_state().hover) {
                    ta_ui_tooltip(CSTR("Model must be visible to cast shadows"));
                }
            }
        } else {
            if (ta_ui_button(CSTR("Add model"))) {
                // TODO: add model
            }
        }
    }

    ta_ui_row_end();
    ta_ui_next_size(header_width, 0);
    ta_ui_next_bg_color(UI_STATE_ALL, 0.0f, 0.5f, 0.7f, 1.0f);
    static bool rigid_body_expanded = false;
    ta_ui_toggle_button(CSTR("Rigid Body"), &rigid_body_expanded);

    if (rigid_body_expanded) {
        ta_rigid_body *rigid_body = ta_game_component_try(entity, RES_COMP_RIGID_BODY);
        if (rigid_body) {
            ta_ui_row_begin();
            ta_ui_next_size(label_width, 0);
            ta_ui_label(CSTR("mass:"));
            static ta_ui_textbox_state mass_editor = { 0 };
            ta_ui_textbox_float(&rigid_body->mass, &mass_editor, 0);
            rigid_body->mass = MAX(0.0f, rigid_body->mass);

            ta_ui_row_begin();
            ta_ui_next_size(label_width, 0);
            ta_ui_label(CSTR("density:"));
            static ta_ui_textbox_state density_editor = { 0 };
            ta_ui_textbox_float(&rigid_body->density, &density_editor, 0);

            char text[64] = { 0 };
            int text_len = 0;

            ta_ui_row_begin();
            ta_ui_next_size(label_width, 0);
            ta_ui_label(CSTR("velocity:"));
            text_len = snprintf(CSTR(text), "%9.6f, %9.6f, %9.6f",
                rigid_body->velocity.x,
                rigid_body->velocity.y,
                rigid_body->velocity.z);
            DLB_ASSERT(text_len < sizeof(text));
            ta_ui_label(CSTR(text));
            ta_ui_next_margin(6, 1, 0, 1);
            ta_rgba velc = TA_COLOR_DARK_RED;
            ta_ui_next_bg_color(UI_STATE_NONE, velc.r, velc.g, velc.b, velc.a);
            ta_ui_next_bg_color(UI_STATE_INTERACT, 0.8f, 0.0f, 0.0f, 0.9f);
            if (ta_ui_button(CSTR("Reset"))) {
                rigid_body->velocity = VEC3_ZERO;
                rigid_body->ang_velocity = VEC3_ZERO;
            }

            ta_ui_row_begin();
            ta_ui_next_size(label_width, 0);
            ta_ui_label(CSTR("ang. velocity:"));
            text_len = snprintf(CSTR(text), "%9.6f, %9.6f, %9.6f",
                rigid_body->ang_velocity.x,
                rigid_body->ang_velocity.y,
                rigid_body->ang_velocity.z);
            DLB_ASSERT(text_len < sizeof(text));
            ta_ui_label(CSTR(text));

            ta_ui_row_begin();
            ta_ui_next_size(label_width, 0);
            ta_ui_label(CSTR("apply gravity:"));
            ta_ui_next_pad(0, 0, 0, 0);
            ta_ui_toggle_button_begin(TA_UI_AUTOSIZE);
            if (!rigid_body->no_gravity) {
                ta_ui_next_margin(0, 0, 0, 0);
                ta_ui_next_bg_color(UI_STATE_NONE, 0.0f, 0.5f, 0.0f, 0.9f);
                ta_ui_next_bg_color(UI_STATE_INTERACT, 0.0f, 0.7f, 0.0f, 0.9f);
                ta_ui_label(CSTR("True"));
            } else {
                ta_ui_next_margin(0, 0, 0, 0);
                ta_ui_next_bg_color(UI_STATE_NONE, 0.7f, 0.0f, 0.0f, 0.9f);
                ta_ui_next_bg_color(UI_STATE_INTERACT, 0.9f, 0.0f, 0.0f, 0.9f);
                ta_ui_label(CSTR("False"));
            }
            ta_ui_toggle_button_end(&rigid_body->no_gravity);

            ta_ui_row_begin();
            ta_ui_next_size(label_width, 0);
            ta_ui_label(CSTR("broadphase collide:"));
            rigid_body->dbg_broadphase ? ta_ui_label(CSTR("True")) : ta_ui_label(CSTR("False"));

            ta_ui_row_begin();
            ta_ui_next_size(label_width, 0);
            ta_ui_label(CSTR("broad aabb center:"));
            static ta_ui_textbox_vec3_state broad_center_editor = { 0 };
            ta_ui_textbox_vec3(&rigid_body->aabb.center, &broad_center_editor, false, true, true);

            ta_ui_row_begin();
            ta_ui_next_size(label_width, 0);
            ta_ui_label(CSTR("broad aabb extents:"));
            static ta_ui_textbox_vec3_state broad_extents_editor = { 0 };
            ta_ui_textbox_vec3(&rigid_body->aabb.extents, &broad_extents_editor, false, true, false);

            ta_ui_row_begin();
            ta_ui_next_size(label_width, 0);
            ta_ui_label(CSTR("narrowphase collide:"));
            rigid_body->dbg_narrowphase ? ta_ui_label(CSTR("True")) : ta_ui_label(CSTR("False"));

            ta_ui_row_begin();
            ta_ui_next_size(label_width, 0);
            ta_ui_label(CSTR("centroid local:"));
            static ta_ui_textbox_vec3_state centroid_local_editor = { 0 };
            ta_ui_textbox_vec3(&rigid_body->centroid_local, &centroid_local_editor, false, false, false);

            ta_ui_row_begin();
            ta_ui_next_size(label_width, 0);
            ta_ui_label(CSTR("centroid global:"));
            static ta_ui_textbox_vec3_state centroid_global_editor = { 0 };
            ta_ui_textbox_vec3(&rigid_body->centroid_global, &centroid_global_editor, false, false, false);

            ta_ui_row_begin();
            ta_ui_next_margin(2, 12, 0, 4);
            ta_ui_next_size(header_width, 0);
            ta_ui_next_bg_color(UI_STATE_ALL, 0.0f, 0.5f, 0.7f, 1.0f);
            ta_ui_label(CSTR("Collider"));

            ta_ui_row_begin();
            ta_ui_next_size(label_width, 0);
            ta_ui_label(CSTR("type:"));
            const char *collider_type = ta_collider_type_str(rigid_body->collider.type);
            ta_ui_label(collider_type, strlen(collider_type));

            ta_ui_row_begin();
            ta_ui_next_size(label_width, 0);
            ta_ui_label(CSTR("center:"));
            static ta_ui_textbox_vec3_state center_editor = { 0 };
            ta_ui_textbox_vec3(&rigid_body->collider.data.center, &center_editor, false, true, true);

            switch (rigid_body->collider.type) {
                case TA_COLLIDER_PLANE: {
                    ta_ui_row_begin();
                    ta_ui_label(CSTR("normal:"));
                    static ta_ui_textbox_vec3_state normal_editor = { 0 };
                    ta_ui_textbox_vec3(&rigid_body->collider.data.plane.normal, &normal_editor, true, true, false);
                    break;
                } case TA_COLLIDER_SPHERE: {
                    ta_ui_row_begin();
                    ta_ui_next_size(label_width, 0);
                    ta_ui_label(CSTR("radius:"));
                    static ta_ui_textbox_state radius_editor = { 0 };
                    ta_ui_textbox_float(&rigid_body->collider.data.sphere.radius, &radius_editor, 0);
                    break;
                } case TA_COLLIDER_OBB: {
                    ta_ui_row_begin();
                    ta_ui_label(CSTR("extents:"));
                    static ta_ui_textbox_vec3_state extents_editor = { 0 };
                    ta_ui_textbox_vec3(&rigid_body->collider.data.obb.extents, &extents_editor, false, true, false);
                    ta_ui_row_begin();
                    ta_ui_label(CSTR("orientation:"));
                    static ta_ui_textbox_vec4_state orientation_editor = { 0 };
                    ta_ui_textbox_vec4(&rigid_body->collider.data.obb.orientation, &orientation_editor, true, true, true);
                    break;
                } default: {
                    break;
                }
            }
        } else {
            if (ta_ui_button(CSTR("Add rigid body"))) {
                // TODO: add rigidy body
            }
        }
    }

    ta_ui_row_end();
    ta_ui_next_size(header_width, 0);
    ta_ui_next_bg_color(UI_STATE_ALL, 0.0f, 0.5f, 0.7f, 1.0f);
    static bool light_expanded = false;
    ta_ui_toggle_button(CSTR("Light"), &light_expanded);

    if (light_expanded) {
        ta_light *light = ta_game_component_try(entity, RES_COMP_LIGHT);
        if (light) {
            ta_ui_row_begin();
            ta_ui_next_size(label_width, 0);
            ta_ui_label(CSTR("enabled:"));
            if (light->disabled) {
                if (ta_ui_button(CSTR("False"))) light->disabled = false;
            } else {
                if (ta_ui_button(CSTR("True"))) light->disabled = true;
            }

            ta_ui_row_begin();
            ta_ui_next_size(label_width, 0);
            ta_ui_label(CSTR("intensity:"));
            static ta_ui_textbox_state intensity_editor = { 0 };
            ta_ui_textbox_float(&light->intensity, &intensity_editor, 0);
            light->intensity = MAX(0.0f, light->intensity);

            ta_ui_row_begin();
            ta_ui_next_size(label_width, 0);
            ta_ui_label(CSTR("color:"));
            static ta_ui_textbox_vec3_state color_editor = { 0 };
            ta_ui_textbox_vec3((ta_vec3 *)&light->color,
                &color_editor, false, false, false);
            light->color.r = clampf(light->color.r, 0.0f, 1.0f);
            light->color.g = clampf(light->color.g, 0.0f, 1.0f);
            light->color.b = clampf(light->color.b, 0.0f, 1.0f);
            ta_ui_next_size(17, 17);
            ta_ui_next_bg_color(UI_STATE_ALL, light->color.r,
                light->color.g, light->color.b, 1.0f);
            ta_ui_button(0, 0);

            ta_ui_row_begin();
            ta_ui_next_size(label_width, 0);
            ta_ui_label(CSTR("shadow map:"));
            static bool show_shadow_map = true;
            if (show_shadow_map) {
                if (ta_ui_button(CSTR("Hide"))) show_shadow_map = false;
            } else {
                if (ta_ui_button(CSTR("Show"))) show_shadow_map = true;
            }

            if (show_shadow_map) {
                ta_ui_row_begin();
                ta_ui_next_pad(1, 1, 1, 1);
                static ta_ui_panel_state shadowmap_panel = { 0 };
                ta_ui_panel_begin(&shadowmap_panel, TA_UI_AUTOSIZE);

                s32 resolution = light->shadowmap.resolution / 10;
                if (light->type == TA_LIGHT_DIRECTIONAL) {
                    ui_texture_face(&light->shadowmap.texture, 0, resolution);
                } else if (light->type == TA_LIGHT_POINT) {
                    // Render cubemap with the following layout:
                    //       ┌────┐                 ┌────┐
                    //       | +Y |                 |  2 |
                    //  ┌────┼────┼────┬────┐  ┌────┼────┼────┬────┐
                    //  | -X | -Z | +X | +Z |  |  1 |  5 |  0 |  4 |
                    //  └────┼────┼────┴────┘  └────┼────┼────┴────┘
                    //       | -Y |                 |  3 |
                    //       └────┘                 └────┘
                    ta_ui_row_begin();
                    ta_ui_spacer(resolution, 0);
                    ui_texture_face(&light->shadowmap.texture, 2, resolution);
                    ta_ui_row_begin();
                    ui_texture_face(&light->shadowmap.texture, 1, resolution);
                    ui_texture_face(&light->shadowmap.texture, 5, resolution);
                    ui_texture_face(&light->shadowmap.texture, 0, resolution);
                    ui_texture_face(&light->shadowmap.texture, 4, resolution);
                    ta_ui_row_begin();
                    ta_ui_spacer(resolution, 0);
                    ui_texture_face(&light->shadowmap.texture, 3, resolution);
                }

                ta_ui_panel_end();
            }
        } else {
            if (ta_ui_button(CSTR("Add light"))) {
                // TODO: add light
            }
        }
    }

    ta_ui_panel_end();
}
static void ui_audio_panel()
{
    static const char *audio_playing_name = 0;

    //ta_ui_next_size(50, 50);
    //ta_ui_next_margin(2, 2, 0, 0);
    static ta_ui_panel_state audio_panel = { 0 };
    ta_ui_panel_begin(&audio_panel, TA_UI_AUTOSIZE);

    const char *audio_request_name = 0;

    ta_ui_row_begin();
    dlb_vec_each(ta_audio_buffer *, audio_buffer, ta_game_resource_pool(RES_AUDIO_BUFFER))
    {
#if 0
        int node_panel_id = -1;
        ta_ui_panel_begin(&TA_SIZE(60 * buf_count, 60), &node_panel_id);
        DLB_ASSERT(node_panel_id >= 0);

        ta_ui_label(buf.uid.uid);
        ta_ui_row_begin();
        ta_ui_button("Play");
        ta_ui_button("Loop");

        ta_ui_panel_end(node_panel_id);
#endif
        bool active = audio_buffer->name == audio_playing_name;
        //ta_ui_next_size(36, 36);
        //ta_ui_next_margin(0, 0, 2, 0);
        ta_ui_toggle_button_begin(TA_UI_AUTOSIZE);
        ta_ui_image(ta_game_by_sym(RES_TEXTURE, tg_tex_audio_icon), 0);
        if (ta_ui_toggle_button_end(&active)) {
            audio_request_name = audio_buffer->name;
        }
        if (ta_ui_last_state().hover) {
            ta_ui_tooltip(SYM(audio_buffer->path));
        }
    }

    if (audio_request_name) {
        ta_audio_source *bg_music_src = ta_game_component(tg_e_background_music,
            RES_COMP_AUDIO_SOURCE);
        ta_audio_source_stop(bg_music_src);
        if (audio_request_name != audio_playing_name) {
            ta_audio_source_set_buffer(bg_music_src,
                audio_request_name);
            ta_audio_source_play_loop(bg_music_src);
            audio_playing_name = audio_request_name;
        } else {
            audio_playing_name = 0;
        }
    }

    ta_ui_panel_end();
}
static void ui_camera_panel()
{
    //ta_ui_next_margin(2, 2, 0, 0);
    static ta_ui_panel_state camera_panel = { 0 };
    ta_ui_panel_begin(&camera_panel, TA_UI_AUTOSIZE);
    static const char *selected_camera = 0;

    //ta_ui_row_begin();
    dlb_vec_each(ta_camera *, camera, ta_game_resource_pool(RES_COMP_CAMERA)) {
        ta_ui_next_size(120, 0);
        bool selected = camera->name == selected_camera;
        ta_ui_toggle_button_begin(TA_UI_AUTOSIZE_H);
        ta_ui_label(SYM(camera->name));
        if (ta_ui_toggle_button_end(&selected)) {
            if (selected) {
                selected_camera = camera->name;
            }
        }
        if (ta_ui_last_state().hover) {
            // TODO: useful tooltip for camera
        }
    }

    if (selected_camera) {
        ta_camera *camera = ta_game_by_sym(RES_COMP_CAMERA, selected_camera);

        ta_ui_row_begin();
        static ta_ui_panel_state selected_camera_panel = { 0 };
        ta_ui_panel_begin(&selected_camera_panel, TA_UI_AUTOSIZE);

        ta_ui_row_begin();
        static ta_ui_panel_state label_panel = { 0 };
        ta_ui_panel_begin(&label_panel, TA_UI_AUTOSIZE);
        ta_ui_label(CSTR("Name"));
        ta_ui_label(CSTR("Entity name"));
        ta_ui_label(CSTR("Target position"));
        ta_ui_label(CSTR("Position"));
        ta_ui_label(CSTR("Position smooth"));
        ta_ui_label(CSTR("Position target vel"));
        ta_ui_label(CSTR("Yaw smooth"));
        ta_ui_label(CSTR("Pitch smooth"));
        ta_ui_label(CSTR("FOV"));
        ta_ui_label(CSTR("Z near"));
        ta_ui_label(CSTR("Debug channel"));
        ta_ui_panel_end();

        static ta_ui_panel_state button_panel = { 0 };
        ta_ui_panel_begin(&button_panel, TA_UI_AUTOSIZE);
        ta_ui_label(SYM(camera->name));
        ta_ui_label(SYM(camera->entity));
        static ta_ui_textbox_vec3_state tpos_textbox = { 0 };
        ta_ui_row_begin();
        ta_ui_textbox_vec3(&camera->target_xform.position, &tpos_textbox, false, false, true);
        static ta_ui_textbox_vec3_state pos_textbox = { 0 };
        ta_ui_row_begin();
        ta_transform *cam_trans = ta_game_component(camera->entity, RES_COMP_TRANSFORM);
        ta_ui_textbox_vec3(&cam_trans->xform.position, &pos_textbox, false, false, false);
        ta_ui_row_end();
        static ta_ui_textbox_state pos_smooth_textbox = { 0 };
        ta_ui_textbox_float(&camera->position_smooth, &pos_smooth_textbox, 0);
        ta_ui_row_begin();
        static ta_ui_textbox_state pos_target_vel_textbox = { 0 };
        ta_ui_textbox_float(&camera->position_target_vel, &pos_target_vel_textbox, 0);
        ta_ui_next_margin(8, 1, 0, 0);
        if (ta_ui_button(CSTR("Slow"))) {
            camera->position_target_vel = 0.01f;
        }
        ta_ui_next_margin(8, 1, 0, 0);
        if (ta_ui_button(CSTR("Normal"))) {
            camera->position_target_vel = 0.3f;
        }
        ta_ui_next_margin(8, 1, 0, 0);
        if (ta_ui_button(CSTR("Fast"))) {
            camera->position_target_vel = 1.0f;
        }
        ta_ui_row_end();
        static ta_ui_textbox_state yaw_smooth_textbox = { 0 };
        ta_ui_textbox_float(&camera->yaw_smooth, &yaw_smooth_textbox, 0);
        static ta_ui_textbox_state pitch_smooth_textbox = { 0 };
        ta_ui_textbox_float(&camera->pitch_smooth, &pitch_smooth_textbox, 0);
        static ta_ui_textbox_state fov_textbox = { 0 };
        ta_ui_textbox_float(&camera->fov, &fov_textbox, 0);
        static ta_ui_textbox_state znear_textbox = { 0 };
        ta_ui_row_begin();
        ta_ui_textbox_float(&camera->znear, &znear_textbox, 0);
        ta_ui_next_margin(8, 1, 0, 0);
        if (ta_ui_button(CSTR("Recalc projection matrix"))) {
            ta_camera_recalc_projection(camera);
        }

        ta_ui_row_begin();
        static struct {
            const char *text;
            u32 len;
        } dbg_modes[] = {
            [DBG_NONE]           = { CSTR("None") },
            [DBG_VTX_COLOR]      = { CSTR("Vertex color") },
            [DBG_VTX_UV]         = { CSTR("UV") },
            [DBG_VTX_NORMAL]     = { CSTR("Normal") },
            [DBG_VTX_TANGENT]    = { CSTR("Tangent") },
            [DBG_VTX_TBN_NORMAL] = { CSTR("TBN normal") },
            [DBG_NORMAL_MAP]     = { CSTR("Normal map") },
            [DBG_MTL_ALBEDO]     = { CSTR("Albedo") },
            [DBG_MTL_EMISSION]   = { CSTR("Emission") },
            [DBG_MTL_METALLIC]   = { CSTR("Metallic") },
            [DBG_MTL_ROUGHNESS]  = { CSTR("Roughness") },
            [DBG_MTL_OCCLUSION]  = { CSTR("Occlusion") },
        };

        for (int mode = 0; mode < ARRAY_COUNT(dbg_modes); ++mode) {
            if (mode && mode % 2 == 0) ta_ui_row_begin();
            ta_ui_next_size(120, 0);
            ta_ui_toggle_button_begin(TA_UI_AUTOSIZE_H);
            ta_ui_label(dbg_modes[mode].text, dbg_modes[mode].len);
            bool checked = mode == camera->dbg_channel;
            if (ta_ui_toggle_button_end(&checked)) {
                camera->dbg_channel = mode;
            }
        }
        ta_ui_panel_end();
        ta_ui_panel_end();
    }

    ta_ui_panel_end();
}
static void ui_material_panel()
{
    //ta_ui_next_margin(2, 2, 0, 0);
    static ta_ui_panel_state material_panel = { 0 };
    ta_ui_panel_begin(&material_panel, TA_UI_AUTOSIZE);
    dlb_vec_each(ta_material *, material, ta_game_resource_pool(RES_MATERIAL)) {
        ta_ui_row_begin();
        ta_ui_next_size(200, 0);
        ta_ui_next_margin(0, 2, 0, 0);
        ta_ui_next_pad(4, 4, 4, 4);
        if (ta_ui_button(SYM(material->name))) {
            const char *entity = ta_editor_selected_entity();
            if (entity) {
                ta_model *model = ta_game_component_try(entity, RES_COMP_MODEL);
                if (model && model->pieces) {
                    // TODO: Select material per piece, not per model. For now, arbitrarily set material of first piece
                    model->pieces[0].material = material->name;
                }
            }
        }
        if (ta_ui_last_state().hover) {
            char tex_buf[2048] = { 0 };
            int len = snprintf(tex_buf, sizeof(tex_buf),
                "name             : %s\n"
                "shader           : %s\n"
                "albedo_factor    : %f %f %f %f\n"
                "albedo_texture   : %s\n"
                "emission_factor  : %f %f %f\n"
                "emission_texture : %s\n"
                "metallic_factor  : %f\n"
                "metallic_texture : %s\n"
                "roughness_factor : %f\n"
                "roughness_texture: %s\n"
                "normal_texture   : %s\n"
                "occlusion_texture: %s\n"
                "height_texture   : %s",
                material->name,
                material->shader,
                material->albedo_factor.r, material->albedo_factor.g, material->albedo_factor.b, material->albedo_factor.a,
                material->albedo_texture,
                material->emission_factor.r, material->emission_factor.g, material->emission_factor.b,
                material->emission_texture,
                material->metallic_factor,
                material->metallic_texture,
                material->roughness_factor,
                material->roughness_texture,
                material->normal_texture,
                material->occlusion_texture,
                material->height_texture
            );
            DLB_ASSERT(len < sizeof(tex_buf));
            ta_ui_tooltip(tex_buf, len);
        }
    }
    ta_ui_panel_end();
}
static void ui_mesh_panel()
{
    //ta_ui_next_margin(2, 2, 0, 0);
    static ta_ui_panel_state mesh_panel = { 0 };
    ta_ui_panel_begin(&mesh_panel, TA_UI_AUTOSIZE);
    dlb_vec_each(ta_mesh *, mesh, ta_game_resource_pool(RES_MESH)) {
        ta_ui_row_begin();
        ta_ui_next_size(200, 0);
        ta_ui_next_margin(0, 2, 0, 0);
        ta_ui_next_pad(4, 4, 4, 4);
        //ta_ui_next_size(material->width, material->height);
        if (ta_ui_button(SYM(mesh->name))) {
            const char *entity = ta_editor_selected_entity();
            if (entity) {
                ta_model *model = ta_game_component_try(entity, RES_COMP_MODEL);
                if (model && model->pieces) {
                    // TODO: Select mesh per piece, not per model. For now, arbitrarily set mesh of first piece
                    model->pieces[0].mesh = mesh->name;
                }
            }
        }
        // TODO: Preview mesh in carousel while mouse hover
        if (ta_ui_last_state().hover) {
            char tex_buf[1024] = { 0 };
            int len = snprintf(tex_buf, sizeof(tex_buf),
                "name         : %s\n"
                "vertex count : %zu",
                mesh->name,
                dlb_vec_len(mesh->positions)
            );
            DLB_ASSERT(len < sizeof(tex_buf));
            ta_ui_tooltip(tex_buf, len);
        }
    }
    ta_ui_panel_end();
}
static void ui_texture_panel()
{
    //ta_ui_next_margin(2, 2, 0, 0);
    ta_ui_next_size(0, 400);
    static ta_ui_panel_state texture_panel = { 0 };
    ta_ui_panel_begin(&texture_panel, TA_UI_AUTOSIZE_W);
    int count = 0;
    dlb_vec_each(ta_texture *, texture, ta_game_resource_pool(RES_TEXTURE)) {
        if (count % 4 == 0) {
            ta_ui_row_begin();
        }
        ta_ui_next_size(68, 68);
        //ta_ui_next_margin(0, 0, 2, 0);
        ta_ui_next_pad(4, 4, 4, 4);
        //ta_ui_next_size(texture->width, texture->height);
        ta_ui_button_begin(0);
        ta_ui_next_size(68, 68);
        ta_ui_image(texture, 0);
        if (ta_ui_button_end()) {
            // TODO: Set textures in the materials tab, this is way too ambiguous
#if 0
            const char *entity = ta_editor_selected_entity();
            if (entity) {
                ta_model *model = ta_game_component_try(entity, RES_COMP_MODEL);
                if (model && model->material) {
                    ta_material *material = ta_game_by_sym(RES_MATERIAL, model->material);
                    material->albedo_texture = texture->name;
                }
            }
#endif
        }
        if (ta_ui_last_state().hover) {
            char tex_buf[256] = { 0 };
            int len = snprintf(tex_buf, sizeof(tex_buf),
                "name: %s\n"
                "path: %s\n"
                "glid: %u",
                texture->name,
                texture->data.path,
                texture->gl_id);
            DLB_ASSERT(len < sizeof(tex_buf));
            ta_ui_tooltip(tex_buf, len);
        }
        count++;
    }
    ta_ui_panel_end();
}
static void ui_textbox_panel()
{
    //ta_ui_next_margin(2, 2, 0, 0);
    static ta_ui_panel_state textbox_panel = { 0 };
    ta_ui_panel_begin(&textbox_panel, TA_UI_AUTOSIZE);

    ta_ui_row_begin();
    ta_ui_label(CSTR("Text:"));
    ta_ui_next_size(300, 0);
    //ta_ui_next_margin(4, 0, 0, 2);
    static ta_ui_textbox_state textbox = { 0 };
    static char buf[] = "The quick brown fox jumps over the lazy dog. 1234567890 |||";
    if (ta_ui_textbox(CSTR(buf), &textbox, 0)) {
        //size_t text_len = dlb_vec_len(textbox.buffer);
        //dlb_memcpy(buf, textbox.buffer, MAX(sizeof(buf) - 1, text_len));
        ta_ui_textbox_clear(&textbox);
    }
    ta_ui_row_end();

    char tb_buffer[20] = { 0 };
    snprintf(CSTR(tb_buffer), "Length: %zu", dlb_vec_len(textbox.buffer));
    ta_ui_label(CSTR(tb_buffer));

    char tb_cursor[20] = { 0 };
    snprintf(CSTR(tb_cursor), "Cursor: %zu", textbox.cursor);
    ta_ui_label(CSTR(tb_cursor));

    ta_ui_panel_end();
}
static void ui_editor_sidebar()
{
    static struct {
        const char *name;
        u32 len;
        void (*panel_method)();
    } categories[] = {
        { CSTR("Scene"),     ui_scene_panel },
    { CSTR("Node"),      ui_node_panel },
    { CSTR("Audio"),     ui_audio_panel },
    { CSTR("Cameras"),   ui_camera_panel },
    { CSTR("Materials"), ui_material_panel },
    { CSTR("Meshes"),    ui_mesh_panel },
    { CSTR("Textures"),  ui_texture_panel },
    { CSTR("Textbox"),   ui_textbox_panel },
    };
    static int category_selected = 1;

    ta_ui_row_begin();
    ta_ui_next_pad(4, 4, 4, 4);
    static ta_ui_panel_state category_panel = { 0 };
    ta_ui_panel_begin(&category_panel, TA_UI_AUTOSIZE);
    for (int i = 0; i < ARRAY_COUNT(categories); i++) {
        if (i % 4 == 0) {
            ta_ui_row_begin();
            ta_ui_next_margin(0, 0, 0, 2);
        } else {
            ta_ui_next_margin(2, 0, 0, 2);
        }
        ta_ui_next_size(100, 0);
        ta_ui_next_pad(0, 0, 0, 0);
        ta_ui_toggle_button_begin(TA_UI_AUTOSIZE_H);
        ta_ui_label(categories[i].name, categories[i].len);
        bool active = (i == category_selected);
        ta_ui_toggle_button_end(&active);
        if (active && category_selected != i) {
            category_selected = i;
            if (editor.textbox_editing) {
                ta_ui_textbox_cancel(editor.textbox_editing);
            }
        }
    }
    ta_ui_panel_end();

    ta_ui_row_begin();
    categories[category_selected].panel_method();
}
void ta_editor_draw_screen()
{
    if (editor.textbox_editing) {
        ta_ui_set_cursor(UI_CURSOR_IBEAM);
    } else {
        ta_ui_set_cursor(UI_CURSOR_ARROW);
    }

    ta_log_write(&tg_debug_log, SRC_EDITOR, "UI layout begin\n");

    ta_ui_spacer(0, 50);
    //ta_ui_next_size(400, 400);
    ta_ui_next_margin(0, 50, 0, 0);
    //ta_ui_next_pad(2, 2, 2, 2);
    static ta_ui_window_state window = { 0 };
    ta_ui_window_begin(&window, TA_UI_AUTOSIZE);

    ta_ui_panel_state hotbar_panel = { 0 };
    ta_ui_panel_begin(&hotbar_panel, TA_UI_AUTOSIZE);
    ta_ui_row_begin();
    if (ta_ui_button(CSTR("Trans."))) {
        editor.widget = WIDGET_TRANSLATE;
    }
    if (ta_ui_button(CSTR("Rot."))) {
        editor.widget = WIDGET_ROTATE;
    }
    if (ta_ui_button(CSTR("Scale"))) {
        editor.widget = WIDGET_SCALE;
    }
    ta_ui_panel_end();

    ui_editor_sidebar();

#if 0
    // Font selector (for trying a lot of fonts quickly)
    ta_ui_row_begin();
    static int cur_font_idx = 0;
    ta_font *fonts = ta_game_resource_pool(RES_FONT);
    int fonts_count = dlb_vec_len(fonts);
    cur_font_idx = MIN(cur_font_idx, fonts_count - 1);

    ta_font *font_current = &fonts[cur_font_idx];
    ta_ui_set_font(font_current);
    ta_ui_row_begin();
    ta_ui_label(CSTR("Font:"));
    ta_ui_label(SYM(font_current->name));

    ta_ui_row_begin();
    ta_ui_next_size(120, 28);
    ta_ui_button_begin(0, 0);
    ta_ui_label(CSTR("Prev Font"));
    if (ta_ui_button_end()) {
        if (cur_font_idx) {
            cur_font_idx--;
        } else {
            cur_font_idx = fonts_count - 1;
        }
    }
    ta_ui_next_size(120, 28);
    ta_ui_button_begin(0, 0);
    ta_ui_label(CSTR("Next Font"));
    if (ta_ui_button_end()) {
        if (cur_font_idx < fonts_count - 1) {
            cur_font_idx++;
        } else {
            cur_font_idx = 0;
        }
    }
#endif

    ta_ui_window_end();
    ta_log_write(&tg_debug_log, SRC_EDITOR, "UI layout end\n");

    ta_log_write(&tg_debug_log, SRC_EDITOR, "UI render begin\n");
    ta_ui_render();
    ta_log_write(&tg_debug_log, SRC_EDITOR, "UI render end\n");
}
