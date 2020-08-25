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

typedef struct ta_editor {
    ta_scene        scene;
    const char      *shader_editor_select;
    editor_widget   widget;
    editor_gizmo    gizmo;
    ta_vec3         gizmo_start_hit;    // if gizmo active, starting contact point of gizmo in world space
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
    ta_game_state       prev_state;
} ta_editor;
ta_editor editor;

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
void ta_editor_selected_entity(const char **out_entity)
{
#if 0
    // Clear selection if entity has been deleted
    if (!ta_game_by_sym_try(tg_game.scene, RES_ENTITY,
        editor.selected_entity_name))
    {
        editor.selected_entity_name = 0;
    }
#endif
    *out_entity = editor.selected_entity;
}
bool ta_editor_textbox_editing()
{
    return editor.textbox_editing != 0;
}

static editor_gizmo editor_gizmo_nearest(ta_ray *ray)
{
    editor_gizmo nearest_gizmo = GIZMO_NONE;
    float t_min = FLT_MAX;

    switch (editor.widget) {
        case WIDGET_TRANSLATE: {
            // TODO: Cleanup
            DLB_ASSERT(editor.gizmos.transform.hitbox1d[0].center.x);

            float t;
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
        const char *selected_entity = 0;
        ta_editor_selected_entity(&selected_entity);
        if (selected_entity) {
            switch (editor.widget) {
                case WIDGET_TRANSLATE:
                case WIDGET_ROTATE:
                case WIDGET_SCALE: {
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
    editor.gizmo_start_hit = VEC3_ZERO;
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

    const char *selected_entity = 0;
    ta_editor_selected_entity(&selected_entity);
    if (selected_entity) {
        switch (editor.widget) {
            case WIDGET_TRANSLATE: {
                editor.gizmo = editor_gizmo_nearest(&ray);
                if (editor.gizmo) {
                    ta_transform *e_transform = ta_game_component(selected_entity, RES_COMP_TRANSFORM);
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
        ta_game_state_set(editor.prev_state);
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
    ta_game_state_set(editor.prev_state);
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

void ta_editor_textbox_event(ta_event *event)
{
    if (!ta_editor_textbox_editing()) {
        return;
    }

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
        } case INPUT_EVENT_KEY_PRESS: case INPUT_EVENT_KEY_REPEAT: {
            // TODO: Use this event, which has a repeat flag, to handle textbox keybinds like backspace/arrow keys to
            // ensure we're respecting the user's OS key delay/repeat settings.
            //event->data.key.repeat;
            switch (event->data.key.scancode) {
                case SDL_SCANCODE_RIGHT: {
                    textbox_command_cursor_right();
                    break;
                } case SDL_SCANCODE_LEFT: {
                    textbox_command_cursor_left();
                    break;
                } case SDL_SCANCODE_DOWN: {
                    textbox_command_cursor_down();
                    break;
                } case SDL_SCANCODE_UP: {
                    textbox_command_cursor_up();
                    break;
                } case SDL_SCANCODE_HOME: {
                    if (event->data.key.mods & KMOD_SHIFT) {
                        textbox_command_cursor_bof();
                    } else {
                        textbox_command_cursor_bol();
                    }
                    break;
                } case SDL_SCANCODE_END: {
                    if (event->data.key.mods & KMOD_SHIFT) {
                        textbox_command_cursor_eof();
                    } else {
                        textbox_command_cursor_eol();
                    }
                    break;
                } case SDL_SCANCODE_DELETE: {
                    textbox_command_delete();
                    break;
                } case SDL_SCANCODE_BACKSPACE: {
                    textbox_command_backspace();
                    break;
                } case SDL_SCANCODE_RETURN: case SDL_SCANCODE_KP_ENTER: {
                    textbox_command_submit();
                    break;
                } case SDL_SCANCODE_ESCAPE: {
                    textbox_command_cancel();
                    break;
                }
            }

            // Consume all unhandled keystrokes when text editor is active
            //if (event->data.key_press.key == SDL_SCANCODE_ENTER) {
            //    ta_ui_textbox_insert(editor.active_textbox, '\n');
            //}
            event->handled = true;
            break;
        } case INPUT_EVENT_KEY_RELEASE: {
            // Consume all unhandled keystrokes when text editor is active
            event->handled = true;
            break;
        } default: {
            break;
        }
    }
}

void ta_editor_update_widgets()
{
    const char *selected_entity = 0;
    ta_editor_selected_entity(&selected_entity);
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
                        if (vec3_zero(editor.gizmo_start_hit)) {
                            editor.gizmo_start_hit = vec3_sub(contact, e_transform->xform.position);
                        }
                        e_transform->xform.position.x = contact.x - editor.gizmo_start_hit.x;
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
                        if (vec3_zero(editor.gizmo_start_hit)) {
                            editor.gizmo_start_hit = vec3_sub(contact, e_transform->xform.position);
                        }
                        e_transform->xform.position.y = contact.y - editor.gizmo_start_hit.y;
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
                        if (vec3_zero(editor.gizmo_start_hit)) {
                            editor.gizmo_start_hit = vec3_sub(contact, e_transform->xform.position);
                        }
                        e_transform->xform.position.z = contact.z - editor.gizmo_start_hit.z;
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
                        if (vec3_zero(editor.gizmo_start_hit)) {
                            editor.gizmo_start_hit = vec3_sub(contact, e_transform->xform.position);
                        }
                        e_transform->xform.position.y = contact.y - editor.gizmo_start_hit.y;
                        e_transform->xform.position.z = contact.z - editor.gizmo_start_hit.z;
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
                        if (vec3_zero(editor.gizmo_start_hit)) {
                            editor.gizmo_start_hit = vec3_sub(contact, e_transform->xform.position);
                        }
                        e_transform->xform.position.x = contact.x - editor.gizmo_start_hit.x;
                        e_transform->xform.position.z = contact.z - editor.gizmo_start_hit.z;
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
                        if (vec3_zero(editor.gizmo_start_hit)) {
                            editor.gizmo_start_hit = vec3_sub(contact, e_transform->xform.position);
                        }
                        e_transform->xform.position.x = contact.x - editor.gizmo_start_hit.x;
                        e_transform->xform.position.y = contact.y - editor.gizmo_start_hit.y;
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
                        if (vec3_zero(editor.gizmo_start_hit)) {
                            editor.gizmo_start_hit = vec3_sub(contact, e_transform->xform.position);
                        }
                        // TODO: Handle view-plane translation (may not be possible while camera is rotating?)
                        ta_sphere cs = { 0 };
                        cs.center = contact;
                        cs.radius = 0.1f;
                        ta_primitive_push_sphere(0, cs, TA_COLOR_YELLOW);
                        //e_transform->xform.position.x = contact.x - editor.gizmo_start_hit.x;
                        //e_transform->xform.position.y = contact.y - editor.gizmo_start_hit.y;
                        //e_transform->xform.position.z = contact.z - editor.gizmo_start_hit.z;
                        //if (editor.widget_snap_to_grid) {
                        //    e_transform->xform.position.x -= (float)fmod(e_transform->xform.position.x, editor.widget_snap_to_grid);
                        //    e_transform->xform.position.y -= (float)fmod(e_transform->xform.position.y, editor.widget_snap_to_grid);
                        //    e_transform->xform.position.z -= (float)fmod(e_transform->xform.position.z, editor.widget_snap_to_grid);
                        //}
                    }
                    break;
                } default: {
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

    const char *selected_entity = 0;
    ta_editor_selected_entity(&selected_entity);
    if (selected_entity) {
        ta_camera *camera = ta_game_camera();

#if 1
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
#endif

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
                    ta_ray fwd = ta_game_camera_ray();
                    editor_gizmo nearest_gizmo = editor_gizmo_nearest(&fwd);

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
                    } default: {
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
    ta_ui_label(CSTR("Scene"), 0);
    ta_ui_label(CSTR("Simulation"), 0);
    ta_ui_label(CSTR("V-Sync"), 0);
    ta_ui_label(CSTR("Audio"), 0);
    ta_ui_label(CSTR("Volume"), 0);
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
    bool vsync;
    ta_window_get_vsync(tg_window, &vsync);
    if (vsync) {
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
static void ui_texture(const char *texture, int resolution)
{
    ta_ui_next_margin(0, 0, 0, 0);
    ta_ui_next_size(resolution, resolution);
    ta_ui_next_pad(0, 0, 0, 0);
    ta_ui_image(texture);
}
static void ui_node_panel()
{
    //ta_ui_next_margin(2, 2, 0, 0);
    static ta_ui_panel_state node_panel = { 0 };
    ta_ui_panel_begin(&node_panel, TA_UI_AUTOSIZE);

    ta_ui_row_begin();
    ta_ui_next_size(200, 15);
    static ta_ui_textbox_state search_box = { 0 };
    if (ta_ui_textbox(0, 0, &search_box, 0)) {

    }
    ta_ui_next_margin(4, 1, 0, 0);
    if (ta_ui_button(CSTR("Clear"))) {
        ta_ui_textbox_clear(&search_box);
    }

    static const char *res_type_lookup[RES_COUNT] = {
        [RES_COMP_AUDIO_SOURCE] = "audiosource",
        [RES_COMP_BUTTON      ] = "button",
        [RES_COMP_CAMERA      ] = "camera",
        [RES_COMP_GUN         ] = "gun",
        [RES_COMP_LIGHT       ] = "light",
        [RES_COMP_MODEL       ] = "model",
        [RES_COMP_PLAYER      ] = "player",
        [RES_COMP_TRANSFORM   ] = "transform",
        [RES_COMP_RIGID_BODY  ] = "rigidbody",
        //[RES_AUDIO_BUFFER     ] = "RES_AUDIO_BUFFER",
        //[RES_FONT             ] = "RES_FONT",
        //[RES_MATERIAL         ] = "RES_MATERIAL",
        //[RES_MESH             ] = "RES_MESH",
        //[RES_SHADER           ] = "RES_SHADER",
        //[RES_TEXTURE          ] = "RES_TEXTURE",
        //[RES_ANIMATION        ] = "RES_ANIMATION"
    };

    // TODO: Accelerate search (e.g. trie) if it gets slow
    size_t query_len = dlb_vec_len(search_box.buffer);
    if (query_len) {
        ta_res_type res_filter_pos = RES_COMP_COUNT;
        ta_res_type res_filter_neg = RES_COMP_COUNT;

        bool match_all = false;
        switch (search_box.buffer[0]) {
            case '*':
                match_all = query_len == 1;
                break;
            case '+': case '-':
                for (ta_res_type res_type = 0; res_type < RES_COMP_COUNT; res_type++) {
                    if (!strncmp(search_box.buffer + 1, res_type_lookup[res_type], query_len - 1) &&
                        query_len - 1 == strlen(res_type_lookup[res_type]))
                    {
                        if (search_box.buffer[0] == '+') {
                            res_filter_pos = res_type;
                            break;
                        } else if (search_box.buffer[0] == '-') {
                            res_filter_neg = res_type;
                            break;
                        }
                    }
                }
                break;
        }

        static const char **search_results = 0;
        dlb_vec_clear(search_results);
        ta_transform *transforms = ta_game_resource_pool(RES_COMP_TRANSFORM);
        dlb_vec_each(ta_transform *, transform, transforms) {
            //if (!strncmp(transform->entity, search_box.buffer, query_len)) {
            bool match_res_pos = false;
            bool match_res_neg = false;
            if (res_filter_pos < RES_COMP_COUNT) {
                match_res_pos = ta_game_component_try(transform->entity, res_filter_pos) != 0;
            }
            if (res_filter_neg < RES_COMP_COUNT) {
                match_res_neg = ta_game_component_try(transform->entity, res_filter_neg) == 0;
            }
            if (match_all || match_res_pos || match_res_neg || strstr(transform->entity, search_box.buffer)) {
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
    const char *selected_entity = 0;
    ta_editor_selected_entity(&selected_entity);
    if (!selected_entity) {
        //ta_ui_spacer(0, 2);
        ta_ui_row_begin();
        ta_ui_next_size(label_width, 0);
        ta_ui_label(CSTR("name:"), 0);
        ta_ui_label(CSTR("< nothing selected >"), 0);
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
        if (ta_ui_label(selected_entity)) {
            DLB_ASSERT(!uid_editor);
            uid_editor = ta_text_entry_init();
            ta_text_entry_set_text(uid_editor, SYM(selected_entity));
            ta_text_entry_focus(uid_editor);
        }
    }
#endif

    ta_transform *transform = ta_game_component(selected_entity, RES_COMP_TRANSFORM);
    ta_ui_row_begin();
    ta_ui_next_size(label_width, 0);
    ta_ui_label(CSTR("Entity:"), 0);
    ta_ui_label(SYM(transform->entity), 0);

    ta_ui_row_end();
    ta_ui_next_size(header_width, 0);
    ta_ui_next_bg_color(UI_STATE_ALL, 0.0f, 0.5f, 0.7f, 1.0f);
    static bool transform_expanded = false;
    ta_ui_toggle_button(CSTR("[+] Transform"), CSTR("[-] Transform"), &transform_expanded);

    if (transform_expanded) {
        ta_ui_row_begin();
        ta_ui_next_size(label_width, 0);
        ta_ui_label(CSTR("position:"), 0);
        static ta_ui_textbox_vec3_state textbox = { 0 };
        ta_ui_textbox_vec3(&transform->xform.position, &textbox, false, false, true);

        ta_ui_row_begin();
        ta_ui_next_size(label_width, 0);
        ta_ui_label(CSTR("orientation:"), 0);
        // TODO: Can't hand edit quaternions.. they need to be normalized and
        // the components need to be in the range [0.0, 1.0]. Let's create a
        // ta_ui_label_vec4, then figure out how to edit rotations (Euler XYZ).
        static ta_ui_textbox_vec4_state orient_editors = { 0 };
        ta_ui_textbox_vec4(&transform->xform.orientation, &orient_editors, true, false, true);

        char text[256] = { 0 };
        size_t text_len = 0;
        ta_ui_row_begin();
        ta_ui_next_size(label_width, 0);
        ta_ui_label(CSTR("position_world:"), 0);
        text_len = snprintf(CSTR(text),
            "%.3f, %.3f, %.3f",
            transform->xform_world.position.x,
            transform->xform_world.position.y,
            transform->xform_world.position.z);
        DLB_ASSERT(text_len < sizeof(text));
        ta_ui_label(text, text_len, 0);

        ta_ui_row_begin();
        ta_ui_next_size(label_width, 0);
        ta_ui_label(CSTR("orientation_world:"), 0);
        text_len = snprintf(CSTR(text),
            "%.3f, %.3f, %.3f, %.3f",
            transform->xform_world.orientation.x,
            transform->xform_world.orientation.y,
            transform->xform_world.orientation.z,
            transform->xform_world.orientation.w);
        DLB_ASSERT(text_len < sizeof(text));
        ta_ui_label(text, text_len, 0);

        // TODO: Everything has parent of "root" except root itself? Each sub-scene has the scene name as parent? Hmm..
        if (transform->parent) {
            ta_ui_row_begin();
            ta_ui_next_size(label_width, 0);
            ta_ui_label(CSTR("parent:"), 0);
            if (ta_ui_button(SYM(transform->parent))) {
                ta_editor_select_entity(transform->parent);
            }
        }
        if (transform->children) {
            ta_ui_row_begin();
            ta_ui_next_size(label_width, 0);
            ta_ui_label(CSTR("children:"), 0);
            dlb_vec_each(const char **, child, transform->children) {
                ta_ui_row_begin();
                ta_ui_next_margin_left(32);
                if (ta_ui_button(SYM(*child))) {
                    ta_editor_select_entity(*child);
                }
            }
        }
    }

    ta_ui_row_end();
    ta_ui_next_size(header_width, 0);
    ta_ui_next_bg_color(UI_STATE_ALL, 0.0f, 0.5f, 0.7f, 1.0f);
    static bool model_expanded = false;
    ta_ui_toggle_button(CSTR("[+] Model"), CSTR("[-] Model"), &model_expanded);

    if (model_expanded) {
        ta_model *model = ta_game_component_try(selected_entity, RES_COMP_MODEL);
        if (model) {
            ta_ui_row_begin();
            ta_ui_next_size(label_width, 0);
            ta_ui_label(CSTR("name:"), 0);
            ta_ui_label(SYM(model->name), 0);

            // List all of the model pieces
            dlb_vec_each(ta_piece *, piece, model->pieces) {
                ta_ui_row_begin();
                ta_ui_next_size(label_width, 0);
                ta_ui_label(CSTR("piece:"), 0);

                ta_ui_row_begin();
                ta_ui_next_margin_left(16);
                ta_ui_next_size(label_width, 0);
                ta_ui_label(CSTR("mesh:"), 0);
                ta_ui_label(SYM(piece->mesh), 0);

                ta_ui_row_begin();
                ta_ui_next_margin_left(16);
                ta_ui_next_size(label_width, 0);
                ta_ui_label(CSTR("material:"), 0);
                ta_ui_label(SYM(piece->material), 0);

                ta_ui_row_begin();
                ta_ui_next_margin_left(16);
                ta_ui_next_size(label_width, 0);
                ta_ui_label(CSTR("anim_targets:"), 0);

                // List all of the piece animation targets
                if (piece->anim_targets) {
                    dlb_vec_each(const char **, anim_target, piece->anim_targets) {
                        ta_ui_row_begin();
                        ta_ui_next_margin_left(32);
                        ta_ui_next_size(label_width, 0);
                        ta_ui_label(SYM(*anim_target), 0);
                    }
                } else {
                    ta_ui_row_begin();
                    ta_ui_next_margin_left(32);
                    ta_ui_next_size(label_width, 0);
                    ta_ui_label(CSTR("none"), 0);
                }
            }

            // List all of the model animation targets
            dlb_vec_each(const char **, anim_target, model->anim_targets) {
                ta_ui_row_begin();
                ta_ui_next_size(label_width, 0);
                ta_ui_label(CSTR("anim_target:"), 0);
                ta_ui_label(SYM(*anim_target), 0);
            }

            ta_ui_row_begin();
            ta_ui_next_size(label_width, 0);
            ta_ui_label(CSTR("visible:"), 0);
            ta_ui_next_pad(0, 0, 0, 0);
            ta_ui_toggle_button_begin(TA_UI_AUTOSIZE);
            if (!model->invisible) {
                ta_ui_next_margin(0, 0, 0, 0);
                ta_ui_next_bg_color(UI_STATE_NONE, 0.0f, 0.5f, 0.0f, 0.9f);
                ta_ui_next_bg_color(UI_STATE_INTERACT, 0.0f, 0.7f, 0.0f, 0.9f);
                ta_ui_label(CSTR("True"), 0);
            } else {
                ta_ui_next_margin(0, 0, 0, 0);
                ta_ui_next_bg_color(UI_STATE_NONE, 0.7f, 0.0f, 0.0f, 0.9f);
                ta_ui_next_bg_color(UI_STATE_INTERACT, 0.9f, 0.0f, 0.0f, 0.9f);
                ta_ui_label(CSTR("False"), 0);
            }
            ta_ui_toggle_button_end(&model->invisible);

            ta_ui_row_begin();
            ta_ui_next_size(label_width, 0);
            ta_ui_label(CSTR("cast shadows:"), 0);
            if (!model->invisible) {
                ta_ui_next_pad(0, 0, 0, 0);
                ta_ui_toggle_button_begin(TA_UI_AUTOSIZE);
                if (model->cast_shadows) {
                    ta_ui_next_margin(0, 0, 0, 0);
                    ta_ui_next_bg_color(UI_STATE_NONE, 0.0f, 0.5f, 0.0f, 0.9f);
                    ta_ui_next_bg_color(UI_STATE_INTERACT, 0.0f, 0.7f, 0.0f, 0.9f);
                    ta_ui_label(CSTR("True"), 0);
                } else {
                    ta_ui_next_margin(0, 0, 0, 0);
                    ta_ui_next_bg_color(UI_STATE_NONE, 0.7f, 0.0f, 0.0f, 0.9f);
                    ta_ui_next_bg_color(UI_STATE_INTERACT, 0.9f, 0.0f, 0.0f, 0.9f);
                    ta_ui_label(CSTR("False"), 0);
                }
                ta_ui_toggle_button_end(&model->cast_shadows);
            } else {
                ta_ui_label(CSTR("n/a"), 0);
                ta_ui_label(CSTR("[?]"), 0);
                if (ta_ui_last_state().hover) {
                    ta_ui_tooltip(CSTR("Model must be visible to cast shadows"));
                }
            }
        } else {
            if (ta_ui_button(CSTR("Add model"))) {
                ta_game_component_add(selected_entity, RES_COMP_MODEL, SYM(selected_entity));
            }
        }
    }

    ta_ui_row_end();
    ta_ui_next_size(header_width, 0);
    ta_ui_next_bg_color(UI_STATE_ALL, 0.0f, 0.5f, 0.7f, 1.0f);
    static bool rigid_body_expanded = false;
    ta_ui_toggle_button(CSTR("[+] Rigid Body"), CSTR("[-] Rigid Body"), &rigid_body_expanded);

    if (rigid_body_expanded) {
        ta_rigid_body *rigid_body = ta_game_component_try(selected_entity, RES_COMP_RIGID_BODY);
        if (rigid_body) {
            ta_ui_row_begin();
            ta_ui_next_size(label_width, 0);
            ta_ui_label(CSTR("mass:"), 0);
            static ta_ui_textbox_state mass_editor = { 0 };
            ta_ui_textbox_float(&rigid_body->mass, &mass_editor, 0);
            rigid_body->mass = MAX(0.0f, rigid_body->mass);

            ta_ui_row_begin();
            ta_ui_next_size(label_width, 0);
            ta_ui_label(CSTR("density:"), 0);
            static ta_ui_textbox_state density_editor = { 0 };
            ta_ui_textbox_float(&rigid_body->density, &density_editor, 0);

            char text[64] = { 0 };
            size_t text_len = 0;

            ta_ui_row_begin();
            ta_ui_next_size(label_width, 0);
            ta_ui_label(CSTR("velocity:"), 0);
            text_len = snprintf(CSTR(text), "%9.6f, %9.6f, %9.6f",
                rigid_body->velocity.x,
                rigid_body->velocity.y,
                rigid_body->velocity.z);
            DLB_ASSERT(text_len < sizeof(text));
            ta_ui_label(text, text_len, 0);
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
            ta_ui_label(CSTR("ang. velocity:"), 0);
            text_len = snprintf(CSTR(text), "%9.6f, %9.6f, %9.6f",
                rigid_body->ang_velocity.x,
                rigid_body->ang_velocity.y,
                rigid_body->ang_velocity.z);
            DLB_ASSERT(text_len < sizeof(text));
            ta_ui_label(text, text_len, 0);

            ta_ui_row_begin();
            ta_ui_next_size(label_width, 0);
            ta_ui_label(CSTR("apply gravity:"), 0);
            ta_ui_next_pad(0, 0, 0, 0);
            ta_ui_toggle_button_begin(TA_UI_AUTOSIZE);
            if (!rigid_body->no_gravity) {
                ta_ui_next_margin(0, 0, 0, 0);
                ta_ui_next_bg_color(UI_STATE_NONE, 0.0f, 0.5f, 0.0f, 0.9f);
                ta_ui_next_bg_color(UI_STATE_INTERACT, 0.0f, 0.7f, 0.0f, 0.9f);
                ta_ui_label(CSTR("True"), 0);
            } else {
                ta_ui_next_margin(0, 0, 0, 0);
                ta_ui_next_bg_color(UI_STATE_NONE, 0.7f, 0.0f, 0.0f, 0.9f);
                ta_ui_next_bg_color(UI_STATE_INTERACT, 0.9f, 0.0f, 0.0f, 0.9f);
                ta_ui_label(CSTR("False"), 0);
            }
            ta_ui_toggle_button_end(&rigid_body->no_gravity);

            ta_ui_row_begin();
            ta_ui_next_size(label_width, 0);
            ta_ui_label(CSTR("broadphase collide:"), 0);
            rigid_body->dbg_broadphase ? ta_ui_label(CSTR("True"), 0) : ta_ui_label(CSTR("False"), 0);

            ta_ui_row_begin();
            ta_ui_next_size(label_width, 0);
            ta_ui_label(CSTR("broad aabb center:"), 0);
            static ta_ui_textbox_vec3_state broad_center_editor = { 0 };
            ta_ui_textbox_vec3(&rigid_body->aabb.center, &broad_center_editor, false, true, true);

            ta_ui_row_begin();
            ta_ui_next_size(label_width, 0);
            ta_ui_label(CSTR("broad aabb extents:"), 0);
            static ta_ui_textbox_vec3_state broad_extents_editor = { 0 };
            ta_ui_textbox_vec3(&rigid_body->aabb.extents, &broad_extents_editor, false, true, false);

            ta_ui_row_begin();
            ta_ui_next_size(label_width, 0);
            ta_ui_label(CSTR("narrowphase collide:"), 0);
            rigid_body->dbg_narrowphase ? ta_ui_label(CSTR("True"), 0) : ta_ui_label(CSTR("False"), 0);

            ta_ui_row_begin();
            ta_ui_next_size(label_width, 0);
            ta_ui_label(CSTR("centroid local:"), 0);
            static ta_ui_textbox_vec3_state centroid_local_editor = { 0 };
            ta_ui_textbox_vec3(&rigid_body->centroid_local, &centroid_local_editor, false, false, false);

            ta_ui_row_begin();
            ta_ui_next_size(label_width, 0);
            ta_ui_label(CSTR("centroid global:"), 0);
            static ta_ui_textbox_vec3_state centroid_global_editor = { 0 };
            ta_ui_textbox_vec3(&rigid_body->centroid_global, &centroid_global_editor, false, false, false);

            ta_ui_row_begin();
            ta_ui_next_margin(2, 12, 0, 4);
            ta_ui_next_size(header_width, 0);
            ta_ui_next_bg_color(UI_STATE_ALL, 0.0f, 0.5f, 0.7f, 1.0f);
            ta_ui_label(CSTR("Collider"), 0);

            ta_ui_row_begin();
            ta_ui_next_size(label_width, 0);
            ta_ui_label(CSTR("type:"), 0);
            const char *collider_type = ta_collider_type_str(rigid_body->collider.type);
            ta_ui_label(collider_type, strlen(collider_type), 0);

            ta_ui_row_begin();
            ta_ui_next_size(label_width, 0);
            ta_ui_label(CSTR("center:"), 0);
            static ta_ui_textbox_vec3_state center_editor = { 0 };
            ta_ui_textbox_vec3(&rigid_body->collider.data.center, &center_editor, false, true, true);

            switch (rigid_body->collider.type) {
                case TA_COLLIDER_PLANE: {
                    ta_ui_row_begin();
                    ta_ui_label(CSTR("normal:"), 0);
                    static ta_ui_textbox_vec3_state normal_editor = { 0 };
                    ta_ui_textbox_vec3(&rigid_body->collider.data.plane.normal, &normal_editor, true, true, false);
                    break;
                } case TA_COLLIDER_SPHERE: {
                    ta_ui_row_begin();
                    ta_ui_next_size(label_width, 0);
                    ta_ui_label(CSTR("radius:"), 0);
                    static ta_ui_textbox_state radius_editor = { 0 };
                    ta_ui_textbox_float(&rigid_body->collider.data.sphere.radius, &radius_editor, 0);
                    break;
                } case TA_COLLIDER_OBB: {
                    ta_ui_row_begin();
                    ta_ui_label(CSTR("extents:"), 0);
                    static ta_ui_textbox_vec3_state extents_editor = { 0 };
                    ta_ui_textbox_vec3(&rigid_body->collider.data.obb.extents, &extents_editor, false, true, false);
                    ta_ui_row_begin();
                    ta_ui_label(CSTR("orientation:"), 0);
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
    ta_ui_toggle_button(CSTR("[+] Light"), CSTR("[-] Light"), &light_expanded);

    if (light_expanded) {
        ta_light *light = ta_game_component_try(selected_entity, RES_COMP_LIGHT);
        if (light) {
            ta_ui_row_begin();
            ta_ui_next_size(label_width, 0);
            ta_ui_label(CSTR("enabled:"), 0);
            ta_ui_toggle_button(CSTR("False"), CSTR("True"), &light->enabled);

            ta_ui_row_begin();
            ta_ui_next_size(label_width, 0);
            ta_ui_label(CSTR("intensity:"), 0);
            static ta_ui_textbox_state intensity_editor = { 0 };
            ta_ui_textbox_float(&light->intensity, &intensity_editor, 0);
            light->intensity = MAX(0.0f, light->intensity);

            ta_ui_row_begin();
            ta_ui_next_size(label_width, 0);
            ta_ui_label(CSTR("color:"), 0);
            static ta_ui_textbox_vec3_state color_editor = { 0 };
            ta_ui_textbox_vec3((ta_vec3 *)&light->color, &color_editor, false, false, false);
            light->color.r = clampf(light->color.r, 0.0f, 1.0f);
            light->color.g = clampf(light->color.g, 0.0f, 1.0f);
            light->color.b = clampf(light->color.b, 0.0f, 1.0f);
            ta_ui_next_size(17, 17);
            ta_ui_next_bg_color(UI_STATE_ALL, light->color.r, light->color.g, light->color.b, 1.0f);
            ta_ui_button(0, 0);

            switch (light->type) {
                case TA_LIGHT_DIRECTIONAL: {
                    ta_ui_row_begin();
                    ta_ui_next_size(label_width, 0);
                    ta_ui_label(CSTR("cast shadows:"), 0);
                    ta_ui_toggle_button(CSTR("False"), CSTR("True"), &light->data.directional.cast_shadows);

                    ta_ui_row_begin();
                    ta_ui_next_size(label_width, 0);
                    static bool show_shadow_map = true;
                    ta_ui_toggle_button(CSTR("[+] Shadow map"), CSTR("[-] Shadow map"), &show_shadow_map);

                    if (show_shadow_map) {
                        ta_ui_row_begin();
                        ta_ui_next_pad(1, 1, 1, 1);
                        static ta_ui_panel_state shadowmap_panel = { 0 };
                        ta_ui_panel_begin(&shadowmap_panel, TA_UI_AUTOSIZE);
                        s32 resolution = (s32)(light->data.directional.shadow_properties.resolution / 10);
                        ui_texture(light->data.directional.shadow_map, resolution);
                        ta_ui_panel_end();
                    }
                    break;
                } case TA_LIGHT_POINT: {
                    ta_ui_row_begin();
                    ta_ui_next_size(label_width, 0);
                    ta_ui_label(CSTR("cast shadows:"), 0);
                    ta_ui_toggle_button(CSTR("False"), CSTR("True"), &light->data.point.cast_shadows);

                    ta_ui_row_begin();
                    //ta_ui_next_size(label_width, 0);
                    static bool show_shadow_map = true;
                    ta_ui_toggle_button(CSTR("[+] Shadow map"), CSTR("[-] Shadow map"), &show_shadow_map);

                    if (show_shadow_map) {
                        ta_ui_row_begin();
                        ta_ui_next_pad(1, 1, 1, 1);
                        static ta_ui_panel_state shadowmap_panel = { 0 };
                        ta_ui_panel_begin(&shadowmap_panel, TA_UI_AUTOSIZE);
                        s32 resolution = (s32)(light->data.point.shadow_properties.resolution / 10);
                        // Render cubemap with the following layout:
                        //       ┌────┐
                        //       | +Y |
                        //  ┌────┼────┼────┬────┐
                        //  | -X | -Z | +X | +Z |
                        //  └────┼────┼────┴────┘
                        //       | -Y |
                        //       └────┘
                        ta_ui_row_begin();
                        ta_ui_spacer(resolution, 0);
                        ui_texture(light->data.point.shadow_map.textures[TA_CUBEMAP_FACE_POSITIVE_Y], resolution);
                        ta_ui_row_begin();
                        ui_texture(light->data.point.shadow_map.textures[TA_CUBEMAP_FACE_NEGATIVE_X], resolution);
                        ui_texture(light->data.point.shadow_map.textures[TA_CUBEMAP_FACE_NEGATIVE_Z], resolution);
                        ui_texture(light->data.point.shadow_map.textures[TA_CUBEMAP_FACE_POSITIVE_X], resolution);
                        ui_texture(light->data.point.shadow_map.textures[TA_CUBEMAP_FACE_POSITIVE_Z], resolution);
                        ta_ui_row_begin();
                        ta_ui_spacer(resolution, 0);
                        ui_texture(light->data.point.shadow_map.textures[TA_CUBEMAP_FACE_NEGATIVE_Y], resolution);
                        ta_ui_panel_end();
                    }
                    break;
                } case TA_LIGHT_SPOT: {
                    ta_ui_row_begin();
                    //ta_ui_next_size(label_width, 0);
                    ta_ui_toggle_button(CSTR("Shadows: Off"), CSTR("Shadows: On"), &light->data.spot.cast_shadows);

                    ta_ui_row_begin();
                    //ta_ui_next_size(label_width, 0);
                    static bool show_shadow_map = true;
                    ta_ui_toggle_button(CSTR("[+] Shadow map"), CSTR("[-] Shadow map"), &show_shadow_map);

                    if (show_shadow_map) {
                        ta_ui_row_begin();
                        ta_ui_next_pad(1, 1, 1, 1);
                        static ta_ui_panel_state shadowmap_panel = { 0 };
                        ta_ui_panel_begin(&shadowmap_panel, TA_UI_AUTOSIZE);
                        s32 resolution = (s32)(light->data.spot.shadow_properties.resolution / 10);
                        ui_texture(light->data.spot.shadow_map, resolution);
                        ta_ui_panel_end();
                    }
                    break;
                }
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
        ta_ui_image(tg_tex_audio_icon);
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
            ta_audio_source_set_buffer(bg_music_src, audio_request_name);
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
        ta_ui_label(SYM(camera->name), 0);
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
        ta_ui_label(CSTR("Name"), 0);
        ta_ui_label(CSTR("Entity name"), 0);
        ta_ui_label(CSTR("Target position"), 0);
        ta_ui_label(CSTR("Position"), 0);
        ta_ui_label(CSTR("Position smooth"), 0);
        ta_ui_label(CSTR("Position target vel"), 0);
        ta_ui_label(CSTR("Yaw smooth"), 0);
        ta_ui_label(CSTR("Pitch smooth"), 0);
        ta_ui_label(CSTR("FOV"), 0);
        ta_ui_label(CSTR("Z near"), 0);
        ta_ui_label(CSTR("Debug channel"), 0);
        ta_ui_panel_end();

        static ta_ui_panel_state button_panel = { 0 };
        ta_ui_panel_begin(&button_panel, TA_UI_AUTOSIZE);
        ta_ui_label(SYM(camera->name), 0);
        ta_ui_label(SYM(camera->entity), 0);
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
            camera->position_target_vel = 0.15f;
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
            [DBG_SHADOW_0]       = { CSTR("Shadow[0]") },
            [DBG_SHADOW_1]       = { CSTR("Shadow[1]") },
            [DBG_SHADOW_2]       = { CSTR("Shadow[2]") },
            [DBG_SHADOW_3]       = { CSTR("Shadow[3]") },
        };

        for (size_t mode = 0; mode < ARRAY_SIZE(dbg_modes); ++mode) {
            if (mode && mode % 2 == 0) ta_ui_row_begin();
            ta_ui_next_size(120, 0);
            ta_ui_toggle_button_begin(TA_UI_AUTOSIZE_H);
            ta_ui_label(dbg_modes[mode].text, dbg_modes[mode].len, 0);
            bool checked = mode == (size_t)camera->dbg_channel;
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
            const char *selected_entity = 0;
            ta_editor_selected_entity(&selected_entity);
            if (selected_entity) {
                ta_model *model = ta_game_component_try(selected_entity, RES_COMP_MODEL);
                if (model && model->pieces) {
                    // TODO: Select material per piece, not per model. For now, arbitrarily set material of first piece
                    model->pieces[0].material = material->name;
                }
            }
        }
        if (ta_ui_last_state().hover) {
            char tex_buf[2048] = { 0 };
            size_t len = snprintf(tex_buf, sizeof(tex_buf),
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
            const char *selected_entity = 0;
            ta_editor_selected_entity(&selected_entity);
            if (selected_entity) {
                ta_model *model = ta_game_component_try(selected_entity, RES_COMP_MODEL);
                if (model && model->pieces) {
                    // TODO: Select mesh per piece, not per model. For now, arbitrarily set mesh of first piece
                    model->pieces[0].mesh = mesh->name;
                }
            }
        }
        // TODO: Preview mesh in carousel while mouse hover
        if (ta_ui_last_state().hover) {
            char tex_buf[1024] = { 0 };
            size_t len = snprintf(tex_buf, sizeof(tex_buf), "%s\n", mesh->name);
            DLB_ASSERT(len < sizeof(tex_buf));

            for (int i = 0; i < TA_VERTEX_ATTRIB_COUNT; i++) {
                len += snprintf(tex_buf + len, sizeof(tex_buf) - len,
                    "[%3u] %s %zu\n",
                    mesh->gl_vertex_buffer,
                    ta_vertex_attrib_type_str(i),
                    dlb_vec_len(mesh->buffers[i])
                );
                DLB_ASSERT(len < sizeof(tex_buf));
            }
            dlb_vec_each(ta_mesh_index_array *, index_array, mesh->indexes) {
                len += snprintf(tex_buf + len, sizeof(tex_buf) - len,
                    "[%3u] %s %zu\n",
                    mesh->gl_index_buffer,
                    "TA_INDEX_BUFFER                 ",
                    dlb_vec_len(index_array->values)
                );
                DLB_ASSERT(len < sizeof(tex_buf));
            }
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
        ta_ui_image(texture->name);
        if (ta_ui_button_end()) {
            // TODO: Set textures in the materials tab, this is way too ambiguous
#if 0
            const char *selected_entity = 0;
            ta_editor_selected_entity(&selected_entity);
            if (selected_entity) {
                ta_model *model = ta_game_component_try(selected_entity, RES_COMP_MODEL);
                if (model && model->material) {
                    ta_material *material = ta_game_by_sym(RES_MATERIAL, model->material);
                    material->albedo_texture = texture->name;
                }
            }
#endif
        }
        if (ta_ui_last_state().hover) {
            char tex_buf[512] = { 0 };
            size_t len = snprintf(tex_buf, sizeof(tex_buf),
                "name               : %s\n"
                "type               : %s\n"
                "path               : %s\n"
                "width              : %u\n"
                "height             : %u\n"
                "channels           : %u\n"
                "pixels_format      : %-18s (0x%4x / %5u)\n"
                "pixels_type        : %-18s (0x%4x / %5u)\n"
                "gl_internal_format : %-18s (0x%4x / %5u)\n"
                "gl_pool            : %u\n"
                "gl_layer           : %u",
                texture->name,
                ta_texture_type_str(texture->type),
                texture->path,
                texture->width,
                texture->height,
                (u32)texture->channels,
                ta_gl_pixels_format_str(texture->pixels_format), texture->pixels_format, texture->pixels_format,
                ta_gl_pixels_type_str(texture->pixels_type), texture->pixels_type, texture->pixels_type,
                ta_gl_pixels_format_str(texture->gl_internal_format), texture->gl_internal_format, texture->gl_internal_format,
                texture->gl_texture_pool_index,
                texture->gl_texture_pool_layer);
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
    ta_ui_label(CSTR("Text:"), 0);
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
    size_t tb_buffer_len = snprintf(CSTR(tb_buffer), "Length: %zu", dlb_vec_len(textbox.buffer));
    DLB_ASSERT(tb_buffer_len < sizeof(tb_buffer));
    ta_ui_label(tb_buffer, tb_buffer_len, 0);

    char tb_cursor[20] = { 0 };
    size_t tb_cursor_len = snprintf(CSTR(tb_cursor), "Cursor: %zu", textbox.cursor);
    DLB_ASSERT(tb_cursor_len < sizeof(tb_cursor));
    ta_ui_label(tb_cursor, tb_cursor_len, 0);

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
    static size_t category_selected = 1;

    ta_ui_row_begin();
    ta_ui_next_pad(4, 4, 4, 4);
    static ta_ui_panel_state category_panel = { 0 };
    ta_ui_panel_begin(&category_panel, TA_UI_AUTOSIZE);
    for (size_t i = 0; i < ARRAY_SIZE(categories); i++) {
        if (i % 4 == 0) {
            ta_ui_row_begin();
            ta_ui_next_margin(0, 0, 0, 2);
        } else {
            ta_ui_next_margin(2, 0, 0, 2);
        }
        ta_ui_next_size(100, 0);
        ta_ui_next_pad(0, 0, 0, 0);
        ta_ui_toggle_button_begin(TA_UI_AUTOSIZE_H);
        ta_ui_label(categories[i].name, categories[i].len, 0);
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
        ta_ui_set_cursor(TA_CURSOR_IBEAM);
    } else {
        ta_ui_set_cursor(TA_CURSOR_ARROW);
    }

    ta_log_write(&tg_debug_log, SRC_EDITOR, "UI layout begin\n");

    ta_ui_spacer(0, 50);
    //ta_ui_next_size(400, 400);
    ta_ui_next_margin(0, 50, 0, 0);
    //ta_ui_next_pad(2, 2, 2, 2);
    static ta_ui_window_state window = { 0 };
    ta_ui_window_begin(&window, TA_UI_AUTOSIZE);

    ta_ui_panel_state collapse_panel = { 0 };
    ta_ui_panel_begin(&collapse_panel, TA_UI_AUTOSIZE);
    ta_ui_row_begin();

    static bool collapsed = false;
    if (collapsed) {
        if (ta_ui_button(CSTR(">"))) {
            collapsed = false;
        }
    } else {
        ta_ui_panel_state hotbar_panel = { 0 };
        ta_ui_panel_begin(&hotbar_panel, TA_UI_AUTOSIZE);
        ta_ui_row_begin();
        if (ta_ui_button(CSTR("<"))) {
            if (editor.textbox_editing) {
                ta_ui_textbox_cancel(editor.textbox_editing);
            }
            collapsed = true;
        }
        if (ta_ui_button(CSTR("Trans."))) {
            editor.widget = WIDGET_TRANSLATE;
        }
        if (ta_ui_button(CSTR("Rot."))) {
            editor.widget = WIDGET_ROTATE;
        }
        if (ta_ui_button(CSTR("Scale"))) {
            editor.widget = WIDGET_SCALE;
        }
        //ta_ui_row_begin();
        //static ta_ui_textbox_state hard_morph_state = { 0 };
        //ta_ui_textbox_float(&tg_hard_morph, &hard_morph_state, 0);
        //tg_hard_morph = clampf(tg_hard_morph, 0.0f, 1.0f);
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
    }

    ta_ui_panel_end();
    ta_ui_window_end();
    ta_log_write(&tg_debug_log, SRC_EDITOR, "UI layout end\n");

    ta_log_write(&tg_debug_log, SRC_EDITOR, "UI render begin\n");
    ta_ui_render();
    ta_log_write(&tg_debug_log, SRC_EDITOR, "UI render end\n");
}
