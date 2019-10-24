#include "ta_editor.h"
#include "ta_ui.h"
#include "ta_game.h"
#include "ta_scene.h"
#include "ta_node.h"
#include "ta_light.h"
#include "ta_symbol.h"
#include "ta_audio.h"
#include "ta_texture.h"
#include "ta_material.h"
#include "ta_primitive.h"
#include "ta_window.h"
#include "ta_font.h"
#include "ta_text_entry.h"
#include "ta_shader.h"
#include "ta_parse.h"
#include "ta_rigid_body.h"
#include "ta_event.h"
#include "ta_mouse.h"
#include "ta_camera.h"
#include "ta_keybind.h"
#include "ta_text_entry.h"
#include "ta_log.h"
#include "ta_position.h"
#include "ta_entity.h"
#include "SDL/SDL_keycode.h"
#include "dlb/dlb_vector.h"

typedef struct ta_editor {
    const char *status_msg;
    u32 selected_entity_id;
    ta_text_entry *text_entry;
    ta_keybind *keybinds;
    ta_keybind *keybinds_text_entry;
    u32 shader_editor_select_id;
    ta_scene *scene;
} ta_editor;

static ta_editor editor;

void ta_editor_init()
{
    ta_log_write(&tg_debug_log, "[Editor] Initializing editor\n");
    ta_ui_init();

    editor.scene = ta_scene_load_file("data/scene/editor.dml");
    DLB_ASSERT(editor.scene);

    editor.shader_editor_select_id = (u32)dlb_hash_search(
        &editor.scene->id_by_name[RES_SHADER], CSTR("shader_editor_select"), 0);
    DLB_ASSERT(editor.shader_editor_select_id);

    ta_log_write(&tg_debug_log, "[Editor] Initializing key binds\n");

#undef DELETE
#define BIND1(keybinds, e, key_state, key1) \
    ta_keybind_bind1(&editor.keybinds, e, TA_KEYBIND_##key_state, \
    SDL_SCANCODE_##key1)
#define BIND2(keybinds, e, key_state, key1, key2) \
    ta_keybind_bind2(&editor.keybinds, e, TA_KEYBIND_##key_state, \
    SDL_SCANCODE_##key1, SDL_SCANCODE_##key2)

    // TODO: Read keybinds from file
    //dlb_vec_reserve(tg_keybinds, 16);

    //--------------------------------------------------------------------------
    // EDITOR

    BIND1(keybinds, TA_EVENT_EDITOR_CLOSE,  RELEASE, GRAVE);
    BIND1(keybinds, TA_EVENT_EDITOR_CLOSE,  RELEASE, ESCAPE);
    BIND1(keybinds, TA_EVENT_EDITOR_SELECT, PRESS, MOUSE_LEFT);

    BIND1(keybinds, TA_EVENT_CAMERA_MOVE_FORWARD,       HOLD, W);
    BIND1(keybinds, TA_EVENT_CAMERA_MOVE_BACKWARD,      HOLD, S);
    BIND1(keybinds, TA_EVENT_CAMERA_MOVE_RIGHT,         HOLD, D);
    BIND1(keybinds, TA_EVENT_CAMERA_MOVE_LEFT,          HOLD, A);
    BIND1(keybinds, TA_EVENT_CAMERA_MOVE_UP,            HOLD, E);
    BIND1(keybinds, TA_EVENT_CAMERA_MOVE_DOWN,          HOLD, Q);

    BIND1(keybinds, TA_EVENT_GAME_PLAYER_MOVE_FORWARD,  HOLD, I);
    BIND1(keybinds, TA_EVENT_GAME_PLAYER_MOVE_BACKWARD, HOLD, K);
    BIND1(keybinds, TA_EVENT_GAME_PLAYER_MOVE_RIGHT,    HOLD, L);
    BIND1(keybinds, TA_EVENT_GAME_PLAYER_MOVE_LEFT,     HOLD, J);

    BIND1(keybinds, TA_EVENT_DEBUG_MOUSE_LOCK,          PRESS,   MOUSE_RIGHT);
    BIND1(keybinds, TA_EVENT_DEBUG_MOUSE_UNLOCK,        RELEASE, MOUSE_RIGHT);
    BIND1(keybinds, TA_EVENT_DEBUG_MOUSE_LOCK_TOGGLE,   PRESS, M);
    BIND1(keybinds, TA_EVENT_DEBUG_TOGGLE_WIREFRAME,    PRESS, 2);
    BIND1(keybinds, TA_EVENT_DEBUG_TOGGLE_BBOX,         PRESS, 3);
    BIND1(keybinds, TA_EVENT_DEBUG_TOGGLE_NORMALS,      PRESS, 4);
    BIND1(keybinds, TA_EVENT_DEBUG_TOGGLE_MESH,         PRESS, 5);

    //--------------------------------------------------------------------------
    // TEXT_ENTRY

    BIND1(keybinds_text_entry, TA_EVENT_EDITOR_TXT_NEWLINE,      PRESS, RETURN);
    BIND1(keybinds_text_entry, TA_EVENT_EDITOR_TXT_SUBMIT,       PRESS, RETURN);
    BIND1(keybinds_text_entry, TA_EVENT_EDITOR_TXT_CANCEL,       RELEASE, ESCAPE);
    BIND1(keybinds_text_entry, TA_EVENT_EDITOR_TXT_BACKSPACE,    PRESS, BACKSPACE);
    BIND1(keybinds_text_entry, TA_EVENT_EDITOR_TXT_DELETE,       PRESS, DELETE);
    BIND1(keybinds_text_entry, TA_EVENT_EDITOR_TXT_CURSOR_RIGHT, HOLD, RIGHT);
    BIND1(keybinds_text_entry, TA_EVENT_EDITOR_TXT_CURSOR_LEFT,  HOLD, LEFT);
    BIND1(keybinds_text_entry, TA_EVENT_EDITOR_TXT_CURSOR_DOWN,  PRESS, DOWN);
    BIND1(keybinds_text_entry, TA_EVENT_EDITOR_TXT_CURSOR_UP,    PRESS, UP);
    BIND1(keybinds_text_entry, TA_EVENT_EDITOR_TXT_CURSOR_BOL,   PRESS, HOME);
    BIND1(keybinds_text_entry, TA_EVENT_EDITOR_TXT_CURSOR_EOL,   PRESS, END);
    BIND2(keybinds_text_entry, TA_EVENT_EDITOR_TXT_CURSOR_BOF,   PRESS, LSHIFT, HOME);
    BIND2(keybinds_text_entry, TA_EVENT_EDITOR_TXT_CURSOR_EOF,   PRESS, LSHIFT, END);

    //--------------------------------------------------------------------------

#undef BIND1
#undef BIND2

    ta_log_write(&tg_debug_log, "[Editor] Game initialized\n");
}

void ta_editor_set_active_text_entry(ta_text_entry *text_entry)
{
    editor.text_entry = text_entry;
}
ta_text_entry *ta_editor_active_text_entry()
{
    return editor.text_entry;
}
void ta_editor_select_node(u32 entity_id)
{
    editor.selected_entity_id = entity_id;
}
u32 ta_editor_selected_node()
{
    // Clear selection if entity has been deleted
    if (!ta_scene_find_by_id_try(tg_game.scene, RES_ENTITY, editor.selected_entity_id)) {
        editor.selected_entity_id = 0;
    }
    return editor.selected_entity_id;
}

static void ui_node_panel()
{
    u32 node_panel_id;
    //ta_ui_next_margin(2, 2, 0, 0);
    ta_ui_panel_begin(0, &node_panel_id);

    static int label_width = 150;
    u32 entity_id = ta_editor_selected_node();
    if (entity_id) {
        //ta_ui_spacer(0, 2);
        ta_ui_row_begin();
        ta_ui_next_size(label_width, 0);
        ta_ui_label(0, "UID:");
        static ta_text_entry *uid_editor = 0;
        if (uid_editor) {
            ta_ui_next_size(100, 0);
            //ta_ui_next_pad(4, 1, 4, 1);
            ta_ui_textbox(0, uid_editor);
            //ta_ui_next_margin(4, 0, 0, 0);
            //ta_ui_next_pad(4, 1, 4, 1);
            ta_ui_next_bg_color(UI_STATE_ALL, 0.0f, 0.8f, 0.0f, 1.0f);
            if (ta_ui_label(0, "Save")) {
                ta_text_entry_submit(uid_editor);
            }
            if (ta_text_entry_submitted(uid_editor)) {
                u32 text_len = 0;
                char *text = ta_text_entry_text(uid_editor, &text_len);

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
            } else if (ta_text_entry_canceled(uid_editor)) {
                ta_text_entry_free(&uid_editor);
            }
        } else {
            //ta_ui_next_pad(4, 1, 4, 1);
            const char **name = dlb_pool_by_id(
                &tg_game.scene->resource_names[RES_ENTITY], entity_id);
            if (ta_ui_label(0, *name)) {
                DLB_ASSERT(!uid_editor);
                uid_editor = ta_text_entry_init();
                ta_text_entry_set_text(uid_editor, SYM(*name));
                ta_text_entry_focus(uid_editor);
            }
        }

        //ta_ui_spacer(0, 2);
        ta_ui_row_begin();
        ta_ui_next_size(label_width, 0);

        ta_entity *entity = ta_scene_find_by_id(tg_game.scene, RES_ENTITY, entity_id);
        ta_position *position = ta_scene_entity_component(tg_game.scene, entity, RES_COMP_POSITION);
        ta_rigid_body *rigid_body = ta_scene_entity_component(tg_game.scene, entity, RES_COMP_RIGID_BODY);
        float *pos_values = 0;
        if (rigid_body) {
            ta_ui_label(0, "RB Position:");
            pos_values = (float *)&rigid_body->position;
        } else {
            ta_ui_label(0, "Position:");
            pos_values = (float *)&position->transform.position;
        }
        const char *pos_labels[3] = { "x: ", " y: ", " z: " };
        static ta_text_entry *pos_editors[3] = { 0 };
        for (int i = 0; i < 3; i++) {
            ta_ui_label(0, pos_labels[i]);
            char pos_buf[10] = { 0 };
            int len = snprintf(pos_buf, sizeof(pos_buf), "%3.4f", pos_values[i]);
            DLB_ASSERT(len < sizeof(pos_buf));
            if (pos_editors[i]) {
                //ta_ui_next_pad(4, 1, 4, 1);
                ta_ui_textbox(0, pos_editors[i]);
                if (ta_text_entry_valid(pos_editors[i])) {
                    u32 text_len = 0;
                    char *text = ta_text_entry_text(pos_editors[i], &text_len);
                    pos_values[i] = parse_float(text);
                }
#if 1
               // ta_ui_next_margin(4, 0, 0, 0);
                //ta_ui_next_pad(4, 1, 4, 1);
                ta_ui_next_bg_color(UI_STATE_ALL, 0.0f, 0.8f, 0.0f, 1.0f);
                if (ta_ui_label(0, "Save")) {
                    ta_text_entry_submit(pos_editors[i]);
                }
#endif
                if (ta_text_entry_submitted(pos_editors[i])) {
                    ta_text_entry_unfocus(pos_editors[i]);
                    ta_text_entry_free(&pos_editors[i]);
                } else if (ta_text_entry_canceled(pos_editors[i])) {
                    ta_text_entry_free(&pos_editors[i]);
                }
            } else {
                //ta_ui_next_pad(4, 1, 4, 1);
                if (ta_ui_label(0, pos_buf)) {
                    DLB_ASSERT(!pos_editors[i]);
                    pos_editors[i] = ta_text_entry_init();
                    ta_text_entry_set_text(pos_editors[i], pos_buf, len);
                    ta_text_entry_focus(pos_editors[i]);
                }
            }
        }

        //ta_ui_spacer(0, 2);
        ta_ui_row_begin();
        ta_ui_next_size(label_width, 0);
        ta_ui_label(0, "Orientation:");
        char orient_buf[64] = { 0 };
        int len = snprintf(orient_buf, sizeof(orient_buf),
            "x: %3.4f, y: %3.4f, z: %3.4f, w: %3.4f",
            position->transform.orientation.x,
            position->transform.orientation.y,
            position->transform.orientation.z,
            position->transform.orientation.w);
        DLB_ASSERT(len < sizeof(orient_buf));
        ta_ui_label(0, orient_buf);
    } else {
        //ta_ui_spacer(0, 2);
        ta_ui_row_begin();
        ta_ui_next_size(label_width, 0);
        ta_ui_label(0, "UID:");
        ta_ui_label(0, "Nothing selected");
    }

    ta_ui_panel_end(node_panel_id);
}
static void ui_audio_panel()
{
    static u32 audio_playing_id = 0;

    u32 audio_panel_id;
    //ta_ui_next_size(50, 50);
    //ta_ui_next_margin(2, 2, 0, 0);
    ta_ui_panel_begin(0, &audio_panel_id);

    u32 audio_request_id = 0;

    ta_ui_row_begin();
    dlb_pool *audio_buffers = &tg_game.scene->resource_data[RES_AUDIO_BUFFER];
    for (u32 i = 0; i < audio_buffers->size; ++i) {
        ta_audio_buffer *audio_buffer = dlb_pool_at(audio_buffers, i);
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
        bool active = audio_buffer->id == audio_playing_id;
        //ta_ui_next_size(36, 36);
        //ta_ui_next_margin(0, 0, 2, 0);
        ta_ui_button_toggle(0, tg_game.tex_audio_icon, &active);
        if (ta_ui_last_frame_state().pressed) {
            audio_request_id = audio_buffer->id;
        }
        if (ta_ui_last_frame_state().hover) {
            ta_ui_tooltip(SYM(audio_buffer->path));
        }
    }

    if (audio_request_id) {
        ta_audio_source_stop(tg_game.background_music);
        if (audio_request_id != audio_playing_id) {
            ta_audio_source_set_buffer(tg_game.background_music, audio_request_id);
            ta_audio_source_play_loop(tg_game.background_music);
            audio_playing_id = audio_request_id;
        } else {
            audio_playing_id = 0;
        }
    }

    ta_ui_panel_end(audio_panel_id);
}
static void ui_texture_panel()
{
    u32 texture_panel_id;
    //ta_ui_next_margin(2, 2, 0, 0);
    ta_ui_panel_begin(0, &texture_panel_id);
    ta_ui_row_begin();
    dlb_pool *textures = &tg_game.scene->resource_data[RES_TEXTURE];
    for (u32 i = 0; i < textures->size; ++i) {
        ta_texture *texture = dlb_pool_at(textures, i);
        ta_ui_next_size(68, 68);
        //ta_ui_next_margin(0, 0, 2, 0);
        //ta_ui_next_pad(2, 2, 2, 2);
        if (ta_ui_button(0, texture)) {
            u32 entity_id = ta_editor_selected_node();
            if (entity_id) {
                ta_entity *entity = ta_scene_find_by_id(tg_game.scene,
                    RES_ENTITY, entity_id);
                u32 material_id = entity->components[RES_MATERIAL];
                if (material_id) {
                    ta_material *material = ta_scene_find_by_id(tg_game.scene,
                        RES_MATERIAL, entity->components[RES_MATERIAL]);
                    material->tex_albedo_id = texture->id;
                }
            }
        }
        if (ta_ui_last_frame_state().hover) {
            char tex_buf[128] = { 0 };
            int len = snprintf(tex_buf, sizeof(tex_buf), "%s (id: %d)(gl_id: %d)",
                texture->path, texture->id, texture->gl_id);
            DLB_ASSERT(len < sizeof(tex_buf));
            ta_ui_tooltip(tex_buf, len);
        }
    }
    ta_ui_panel_end(texture_panel_id);
}
static void ui_textbox_panel()
{
    u32 textbox_panel_id;
    //ta_ui_next_margin(2, 2, 0, 0);
    ta_ui_panel_begin(0, &textbox_panel_id);

    static ta_text_entry *text_entry = 0;
    if (!text_entry) {
        text_entry = ta_text_entry_init();
        ta_text_entry_set_text(text_entry, CSTR("This is a test."));
    }
    ta_ui_row_begin();
    ta_ui_label(0, "Text:");
    ta_ui_next_size(300, 0);
    //ta_ui_next_margin(4, 0, 0, 2);
    ta_ui_textbox(0, text_entry);
    ta_ui_panel_end(textbox_panel_id);
}
static void ui_editor_sidebar()
{
    enum {
        CATEGORY_NODE,
        CATEGORY_AUDIO,
        CATEGORY_TEXTURES,
        CATEGORY_TEXTBOX,
        CATEGORY_COUNT
    };
    const char *category_names[CATEGORY_COUNT] = { 0 };
    category_names[CATEGORY_NODE]     = INTERN(STRING(CATEGORY_NODE));
    category_names[CATEGORY_AUDIO]    = INTERN(STRING(CATEGORY_AUDIO));
    category_names[CATEGORY_TEXTURES] = INTERN(STRING(CATEGORY_TEXTURES));
    category_names[CATEGORY_TEXTBOX]  = INTERN(STRING(CATEGORY_TEXTBOX));
    static int category_selected = CATEGORY_NODE;

    ta_ui_row_begin();
    ta_ui_next_size(50, 50);
    //ta_ui_next_pad(2, 2, 2, 2);
    u32 category_panel_id = 0;
    ta_ui_panel_begin(INTERN("editor_sidebar"), &category_panel_id);
    for (int i = 0; i < CATEGORY_COUNT; i++) {
        ta_ui_row_begin();
        ta_ui_next_size(50, 50);
        //ta_ui_next_margin(0, 0, 0, 2);
        bool active = (i == category_selected);
        ta_ui_button_toggle(category_names[i], 0, &active);
        if (active) {
            category_selected = i;
        }
        if (ta_ui_last_frame_state().hover) {
            ta_ui_tooltip(SYM(category_names[i]));
        }
    }
    ta_ui_panel_end(category_panel_id);

    switch (category_selected) {
        case CATEGORY_NODE: {
            ui_node_panel();
            break;
        } case CATEGORY_AUDIO: {
            ui_audio_panel();
            break;
        } case CATEGORY_TEXTURES: {
            ui_texture_panel();
            break;
        } case CATEGORY_TEXTBOX: {
            ui_textbox_panel();
            break;
        } default: {
            break;
        }
    }
}
static void ui_statusbar()
{
    if (editor.status_msg) {
        ta_ui_statusbar();

        static ta_rect_uv *status_rects = 0;
        ta_rectf status_rect = ta_font_push_text(&status_rects, tg_game.font,
            SYM(editor.status_msg), true, 0, 0, 0, 0);
        dlb_vec_each(ta_rect_uv *, rect, status_rects) {
            ta_primitive_push_rect_uv(&quads_queue, *rect, TA_COLOR_WHITE, 0,
                true, false);
        }
        dlb_vec_zero(status_rects);

        int status_halfw = WINDOW_W / 2 - (int)status_rect.w / 2;
        const int status_pad_bottom = 20;
        ta_font_render(quads_queue, tg_game.font, (float)status_halfw,
            (float)(WINDOW_H - (tg_game.font->ascent + status_pad_bottom)),
            UI_LAYER_TIP, true, true);

        editor.status_msg = 0;
    }
}
void ta_editor_draw(float alpha)
{
    // Stencil selected node
    u32 selected_entity_id = ta_editor_selected_node();
    if (selected_entity_id) {
        ta_entity *entity = ta_scene_find_by_id(tg_game.scene, RES_ENTITY,
            selected_entity_id);
        ta_camera *camera = ta_scene_find_by_id(tg_game.scene, RES_COMP_CAMERA,
            tg_game.camera_active_id);

        glEnable(GL_STENCIL_TEST);
        glStencilFunc(GL_ALWAYS, 1, 0xFF);
        glStencilMask(0xFF);
        // Stencil the outline and any occluded fragments
        glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
        // Stencil just the outline
        //glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
        glDepthMask(GL_FALSE);
        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
        ta_node_render(entity, camera, alpha);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glDepthMask(GL_TRUE);

        glClear(GL_DEPTH_BUFFER_BIT);

        // Outline selected node
        glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
        glStencilMask(0x00);
        ta_shader *shader = ta_scene_find_by_id(editor.scene, RES_SHADER,
            editor.shader_editor_select_id);
        ta_shader_set_vec4(shader, SYM_U_COLOR, (ta_vec4 *)&TA_COLOR_YELLOW);
        ta_node_render_shader(entity, camera, shader, alpha, 1.1f);
        glDisable(GL_STENCIL_TEST);
    } else {
        glClear(GL_DEPTH_BUFFER_BIT);
    }

    // TODO: Remove x,y coords from init() methods and only store size. Pass x,y
    //       at render time (make sure to update viewport correctly).
    ta_ui_spacer(0, 50);
    ta_ui_next_size(300, 400);
    //ta_ui_next_margin(0, 50, 0, 0);
    //ta_ui_next_pad(2, 2, 2, 2);
    ta_ui_window_begin(INTERN("test_window"), 0);
    ui_editor_sidebar();
    ta_ui_window_end();

    ui_statusbar();

    // Render tooltips
    ta_primitive_render_quads(tooltip_bg_queue, tg_shader_quads, true, true);
    ta_font_render(tooltip_fg_queue, tg_game.font, 0, 0, UI_LAYER_TIP, true, true);
}

static void editor_ray_pick()
{
    ta_camera *camera = ta_scene_find_by_id(tg_game.scene, RES_COMP_CAMERA,
        tg_game.camera_active_id);

    ta_ray ray;
    ray.origin = camera->position;
    ray.direction = camera->front;

    float t_min = 9999.0f;
    u32 closest_entity_id = 0;
    ta_light *closest_light = 0;  // TODO: Lights and cameras should be nodes

    dlb_pool *entities = &tg_game.scene->resource_data[RES_ENTITY];
    for (u32 i = 0; i < entities->size; ++i) {
        ta_entity *entity = dlb_pool_at(entities, i);
        u32 rigid_body_id = entity->components[RES_COMP_RIGID_BODY];
        if (!rigid_body_id) {
            continue;
        }

        // TODO: Handle types other than spheres
        ta_rigid_body *body = ta_scene_find_by_id_try(tg_game.scene,
            RES_COMP_RIGID_BODY, rigid_body_id);
        if (body->collider.type != TA_COLLIDER_SPHERE) {
            continue;
        }

        ta_sphere sphere = body->collider.data.sphere;
        sphere.center = vec3_add(sphere.center, body->centroid_global);
        float t;
        if (ta_intersect_ray_sphere(ray, sphere, &t)) {
            if (t >= 0.0f && t < t_min) {
                t_min = t;
                closest_entity_id = entity->id;
            }
        }
    }

    dlb_pool *lights = &tg_game.scene->resource_data[RES_COMP_LIGHT];
    for (u32 i = 0; i < lights->size; ++i) {
        ta_light *light = dlb_pool_at(lights, i);
        ta_sphere sphere = { 0 };
        sphere.center = light->position;
        sphere.radius = 0.2f;
        float t;
        if (ta_intersect_ray_sphere(ray, sphere, &t)) {
            if (t >= 0.0f && t < t_min) {
                t_min = t;
                closest_light = light;
                closest_entity_id = 0;
            }
        }
    }

    if (closest_entity_id) {
        ta_editor_select_node(closest_entity_id);
    } else if (closest_light) {
        closest_light->disabled = !closest_light->disabled;
    }
}

void ta_editor_hotkeys()
{
    if (editor.text_entry) {
        ta_keybind_trigger(editor.keybinds_text_entry);
    } else {
        ta_keybind_trigger(editor.keybinds);
    }
}

void ta_editor_event(ta_event *event)
{
    if (editor.text_entry) {
        ta_text_entry_event(editor.text_entry, event);
        if (event->handled) return;
    }

    bool handled = true;

    switch (event->type) {
        case TA_EVENT_EDITOR_CLOSE: {
            ta_game_state_set(&tg_game, tg_game.state_prev);
            break;
        } case TA_EVENT_EDITOR_SELECT: {
            // TODO: Move mouse capture requirement into keybind settings
            if (ta_mouse_captured()) {
                editor_ray_pick();
            }
            break;
        } default: {
            handled = false;
        }
    }

    event->handled = handled;
}

#if 0
// 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
if (show_demo_window)
ImGui::ShowDemoWindow(&show_demo_window);

// 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
{
    static float f = 0.0f;
    static int counter = 0;

    ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

    ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
    ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
    ImGui::Checkbox("Another Window", &show_another_window);

    ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
    ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

    if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
        counter++;
    ImGui::SameLine();
    ImGui::Text("counter = %d", counter);

    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    ImGui::End();
}

// 3. Show another simple window.
if (show_another_window)
{
    ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
    ImGui::Text("Hello from another window!");
    if (ImGui::Button("Close Me"))
        show_another_window = false;
    ImGui::End();
}

#endif