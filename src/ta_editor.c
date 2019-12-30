#include "ta_audio.h"
#include "ta_camera.h"
#include "ta_editor.h"
#include "ta_entity.h"
#include "ta_event.h"
#include "ta_font.h"
#include "ta_game.h"
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
#include "SDL/SDL_keycode.h"
#include "SDL/SDL.h"

typedef enum editor_command {
    EDITOR_COMMAND_CLOSE1,
    EDITOR_COMMAND_CLOSE2,
    EDITOR_COMMAND_SELECT,
    EDITOR_COMMAND_SIM_PAUSE_RESUME,
    EDITOR_COMMAND_SIM_NEXT,
    EDITOR_COMMAND_SIM_NEXT_10,
    EDITOR_COMMAND_SIM_WHILE_HELD,
    EDITOR_COMMAND_COUNT
} editor_command;

typedef struct ta_editor {
    const char *status_msg;
    const char *selected_entity;
    ta_ui_textbox_state *textbox_editing;
    ta_ui_textbox_state *textbox_dragging;
    ta_keybind keybinds[EDITOR_COMMAND_COUNT];
    const char *shader_editor_select;
    ta_scene scene;
} ta_editor;

#if 0
typedef struct drag_float_state {
    float *value;       // pointer to float being dragged
    bool changed;       // true if float has been dragged at all
    ta_vec3 cam_offset; // offset of camera from selected object (for chase cam)
    float cam_position_smooth;      // original position_smooth
    float cam_position_target_vel;  // original position_target_vel
} drag_float_state;
static drag_float_state drag_float;
#endif
static ta_editor editor;

void ta_editor_init()
{
    ta_font *font = ta_game_by_sym(RES_FONT, tg_font);
    ta_log_write(&tg_debug_log, SRC_EDITOR, "Initializing UI styles\n");
    ta_ui_init(font, &editor.textbox_editing, &editor.textbox_dragging);

    ta_log_write(&tg_debug_log, SRC_EDITOR, "Loading editor scene\n");
    ta_scene_load_file(&editor.scene, "data/scene/editor.dml");
    editor.shader_editor_select = SYM_SHADER_EDITOR_SELECT;

    ta_log_write(&tg_debug_log, SRC_EDITOR, "Initializing key binds\n");

    // TODO: Read keybinds from file
    //dlb_vec_reserve(tg_keybinds, 16);

    ta_keybind_init1(&editor.keybinds[EDITOR_COMMAND_CLOSE1          ], TA_KEYBIND_RELEASE, SDL_SCANCODE_GRAVE);
    ta_keybind_init1(&editor.keybinds[EDITOR_COMMAND_CLOSE2          ], TA_KEYBIND_RELEASE, SDL_SCANCODE_ESCAPE);
    ta_keybind_init1(&editor.keybinds[EDITOR_COMMAND_SELECT          ], TA_KEYBIND_PRESS,   SDL_SCANCODE_MOUSE_LEFT);
    ta_keybind_init1(&editor.keybinds[EDITOR_COMMAND_SIM_PAUSE_RESUME], TA_KEYBIND_PRESS,   SDL_SCANCODE_F5);
    ta_keybind_init1(&editor.keybinds[EDITOR_COMMAND_SIM_NEXT        ], TA_KEYBIND_PRESS,   SDL_SCANCODE_F6);
    ta_keybind_init1(&editor.keybinds[EDITOR_COMMAND_SIM_NEXT_10     ], TA_KEYBIND_PRESS,   SDL_SCANCODE_F7);
    ta_keybind_init1(&editor.keybinds[EDITOR_COMMAND_SIM_WHILE_HELD  ], TA_KEYBIND_HOLD,    SDL_SCANCODE_F8);
}
void ta_editor_select_entity(const char *entity_name)
{
    editor.selected_entity = entity_name;
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

static void ui_scene_panel()
{
    static ta_ui_panel_state scene_panel = { 0 };
    ta_ui_panel_begin(0, &scene_panel, TA_UI_AUTOSIZE);

    ta_ui_row_begin();
    static ta_ui_panel_state label_panel = { 0 };
    ta_ui_panel_begin(0, &label_panel, TA_UI_AUTOSIZE);
    ta_ui_label(0, CSTR("Scene"));
    ta_ui_label(0, CSTR("Simulation"));
    ta_ui_label(0, CSTR("V-Sync"));
    ta_ui_label(0, CSTR("Audio"));
    ta_ui_label(0, CSTR("Volume"));
    ta_ui_panel_end();

    static ta_ui_panel_state button_panel = { 0 };
    ta_ui_panel_begin(0, &button_panel, TA_UI_AUTOSIZE);
    ta_ui_next_bg_color(UI_STATE_NONE, 0.0f, 0.5f, 0.0f, 0.9f);
    ta_ui_next_bg_color(UI_STATE_INTERACT, 0.0f, 0.7f, 0.0f, 0.9f);
    if (ta_ui_button(0, CSTR("Save"))) {
        ta_game_save();
    }

    ta_ui_row_begin();
    if (ta_game_sim_running()) {
        ta_ui_next_bg_color(UI_STATE_NONE, 0.0f, 0.5f, 0.0f, 0.9f);
        ta_ui_next_bg_color(UI_STATE_INTERACT, 0.0f, 0.7f, 0.0f, 0.9f);
        if (ta_ui_button(0, CSTR("Running"))) {
            ta_game_sim_pause();
        }
    } else {
        ta_ui_next_bg_color(UI_STATE_NONE, 0.7f, 0.0f, 0.0f, 0.9f);
        ta_ui_next_bg_color(UI_STATE_INTERACT, 0.9f, 0.0f, 0.0f, 0.9f);
        if (ta_ui_button(0, CSTR("Paused"))) {
            ta_game_sim_resume();
        }
        ta_ui_next_bg_color(UI_STATE_NONE, 0.7f, 0.4f, 0.0f, 0.9f);
        ta_ui_next_bg_color(UI_STATE_INTERACT, 0.9f, 0.4f, 0.0f, 0.9f);
        if (ta_ui_button(0, CSTR("Next (1)"))) {
            ta_game_sim_step_n_frames(1);
        }
        ta_ui_next_bg_color(UI_STATE_NONE, 0.7f, 0.4f, 0.0f, 0.9f);
        ta_ui_next_bg_color(UI_STATE_INTERACT, 0.9f, 0.4f, 0.0f, 0.9f);
        if (ta_ui_button(0, CSTR("Next (10)"))) {
            ta_game_sim_step_n_frames(10);
        }
    }

    ta_ui_row_begin();
    if (ta_window_vsync(tg_window)) {
        ta_ui_next_bg_color(UI_STATE_NONE, 0.0f, 0.5f, 0.0f, 0.9f);
        ta_ui_next_bg_color(UI_STATE_INTERACT, 0.0f, 0.7f, 0.0f, 0.9f);
        if (ta_ui_button(0, CSTR("On"))) {
            ta_window_set_vsync(tg_window, false);
        }
    } else {

        ta_ui_next_bg_color(UI_STATE_NONE, 0.7f, 0.0f, 0.0f, 0.9f);
        ta_ui_next_bg_color(UI_STATE_INTERACT, 0.9f, 0.0f, 0.0f, 0.9f);
        if (ta_ui_button(0, CSTR("Off"))) {
            ta_window_set_vsync(tg_window, true);
        }
    }

    ta_ui_row_begin();
    if (tg_audio.muted) {
        ta_ui_next_bg_color(UI_STATE_NONE, 0.7f, 0.0f, 0.0f, 0.9f);
        ta_ui_next_bg_color(UI_STATE_INTERACT, 0.9f, 0.0f, 0.0f, 0.9f);
        if (ta_ui_button(0, CSTR("Unmute"))) {
            ta_audio_listener_unmute(&tg_audio);
        }
    } else {
        ta_ui_next_bg_color(UI_STATE_NONE, 0.0f, 0.5f, 0.0f, 0.9f);
        ta_ui_next_bg_color(UI_STATE_INTERACT, 0.0f, 0.7f, 0.0f, 0.9f);
        if (ta_ui_button(0, CSTR("Mute"))) {
            ta_audio_listener_mute(&tg_audio);
        }
    }

    ta_ui_row_begin();
    // TODO: Make this a drag float
    char tex_buf[16] = { 0 };
    int len = snprintf(tex_buf, sizeof(tex_buf), "%.2f",
        ta_audio_listener_get_volume(&tg_audio));
    DLB_ASSERT(len < sizeof(tex_buf));
    ta_ui_label(0, tex_buf, len);
    ta_ui_panel_end();

    ta_ui_panel_end();
}
static void ui_node_panel()
{
    //ta_ui_next_margin(2, 2, 0, 0);
    static ta_ui_panel_state node_panel = { 0 };
    ta_ui_panel_begin(0, &node_panel, TA_UI_AUTOSIZE);

    ta_ui_row_begin();
    ta_ui_next_size(200, 17);
    static ta_ui_textbox_state search_box = { 0 };
    if (ta_ui_textbox(0, 0, 0, &search_box, 0)) {

    }
    ta_ui_next_margin(4, 1, 0, 0);
    if (ta_ui_button(0, CSTR("Clear"))) {
        ta_ui_textbox_clear(&search_box);
    }

    // TODO: Accelerate search (e.g. trie) if it gets slow
    u32 query_len = dlb_vec_len(search_box.buffer);
    if (query_len) {
        static const char **search_results = 0;
        dlb_vec_clear(search_results);
        ta_transform *transforms = ta_game_resource_pool(RES_COMP_TRANSFORM);
        dlb_vec_each(ta_transform *, transform, transforms) {
            //if (!strncmp(transform->entity_name, search_box.buffer, query_len)) {
            if (strstr(transform->entity_name, search_box.buffer)) {
                dlb_vec_push(search_results, transform->entity_name);
            }
        }
        dlb_vec_each(const char **, result, search_results) {
            ta_ui_row_begin();
            ta_ui_next_size(200, 0);
            if (ta_ui_button(0, SYM(*result))) {
                ta_editor_select_entity(*result);
            }
        }
    }

    const int header_width = 300;
    const int label_width = 150;
    const char *entity_name = ta_editor_selected_entity();
    if (!entity_name) {
        //ta_ui_spacer(0, 2);
        ta_ui_row_begin();
        ta_ui_next_size(label_width, 0);
        ta_ui_label(0, CSTR("name:"));
        ta_ui_label(0, CSTR("< nothing selected >"));
        ta_ui_panel_end();
        return;
    }

#if 0
    static ta_text_entry *uid_editor = 0;
    if (uid_editor) {
        ta_ui_next_size(100, 0);
        //ta_ui_next_pad(4, 1, 4, 1);
        ta_ui_textbox(0, uid_editor);
        //ta_ui_next_margin(4, 0, 0, 0);
        //ta_ui_next_pad(4, 1, 4, 1);
        ta_ui_next_bg_color(UI_STATE_INTERACT, 0.0f, 0.8f, 0.0f, 1.0f);
        if (ta_ui_label(0, CSTR("Save"))) {
            ta_text_entry_submit(uid_editor);
        }
        if (ta_text_entry_submitted(uid_editor)) {
            u32 text_len = 0;
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
        if (ta_ui_label(0, entity_name)) {
            DLB_ASSERT(!uid_editor);
            uid_editor = ta_text_entry_init();
            ta_text_entry_set_text(uid_editor, SYM(entity_name));
            ta_text_entry_focus(uid_editor);
        }
    }
#endif

    ta_transform *transform = ta_game_component_try(RES_COMP_TRANSFORM, entity_name);
    if (transform) {
        ta_ui_row_begin();
        ta_ui_next_margin(2, 12, 0, 4);
        ta_ui_next_size(header_width, 0);
        ta_ui_next_bg_color(UI_STATE_ALL, 0.0f, 0.5f, 0.7f, 1.0f);
        ta_ui_label(0, CSTR("Transform"));

        ta_ui_row_begin();
        ta_ui_next_size(label_width, 0);
        ta_ui_label(0, CSTR("name:"));
        ta_ui_label(0, SYM(transform->entity_name));

        ta_ui_row_begin();
        ta_ui_next_size(label_width, 0);
        ta_ui_label(0, CSTR("position:"));
        static ta_ui_textbox_vec3_state textbox = { 0 };
        ta_ui_textbox_vec3(&transform->xform.position, &textbox, false, true, true);

        ta_ui_spacer(0, 6);
        ta_ui_row_begin();
        ta_ui_next_size(label_width, 0);
        ta_ui_label(0, CSTR("orientation:"));
        // TODO: Can't hand edit quaternions.. they need to be normalized and
        // the components need to be in the range [0.0, 1.0]. Let's create a
        // ta_ui_label_vec4, then figure out how to edit rotations (Euler XYZ).
        static ta_ui_textbox_vec4_state orient_editors = { 0 };
        ta_ui_textbox_vec4(&transform->xform.orientation, &orient_editors, true,
            true, true);
    } else {
        ta_ui_row_begin();
        ta_ui_next_size(label_width, 0);
        ta_ui_label(0, CSTR("name:"));
        ta_ui_label(0, SYM(entity_name));
    }

    ta_model *model = ta_game_component_try(RES_COMP_MODEL, entity_name);
    if (model) {
        ta_ui_row_begin();
        ta_ui_next_margin(2, 12, 0, 4);
        ta_ui_next_size(header_width, 0);
        ta_ui_next_bg_color(UI_STATE_ALL, 0.0f, 0.5f, 0.7f, 1.0f);
        ta_ui_label(0, CSTR("Model"));

        // TODO: List all the meshes
        DLB_ASSERT(dlb_vec_len(model->meshes) > 0);
        ta_ui_row_begin();
        ta_ui_next_size(label_width, 0);
        ta_ui_label(0, CSTR("meshes[0]:"));
        ta_ui_label(0, SYM(model->meshes[0]));

        ta_ui_row_begin();
        ta_ui_next_size(label_width, 0);
        ta_ui_label(0, CSTR("material:"));
        ta_ui_label(0, SYM(model->material));

        ta_ui_row_begin();
        ta_ui_next_size(label_width, 0);
        ta_ui_label(0, CSTR("visible:"));
        ta_ui_next_pad(0, 0, 0, 0);
        ta_ui_toggle_button_begin(0, TA_UI_AUTOSIZE);
        if (!model->invisible) {
            ta_ui_next_margin(0, 0, 0, 0);
            ta_ui_next_bg_color(UI_STATE_NONE, 0.0f, 0.5f, 0.0f, 0.9f);
            ta_ui_next_bg_color(UI_STATE_INTERACT, 0.0f, 0.7f, 0.0f, 0.9f);
            ta_ui_label(0, CSTR("True"));
        } else {
            ta_ui_next_margin(0, 0, 0, 0);
            ta_ui_next_bg_color(UI_STATE_NONE, 0.7f, 0.0f, 0.0f, 0.9f);
            ta_ui_next_bg_color(UI_STATE_INTERACT, 0.9f, 0.0f, 0.0f, 0.9f);
            ta_ui_label(0, CSTR("False"));
        }
        ta_ui_toggle_button_end(&model->invisible);

        ta_ui_row_begin();
        ta_ui_next_size(label_width, 0);
        ta_ui_label(0, CSTR("cast shadows:"));
        if (!model->invisible) {
            ta_ui_next_pad(0, 0, 0, 0);
            ta_ui_toggle_button_begin(0, TA_UI_AUTOSIZE);
            if (model->cast_shadows) {
                ta_ui_next_margin(0, 0, 0, 0);
                ta_ui_next_bg_color(UI_STATE_NONE, 0.0f, 0.5f, 0.0f, 0.9f);
                ta_ui_next_bg_color(UI_STATE_INTERACT, 0.0f, 0.7f, 0.0f, 0.9f);
                ta_ui_label(0, CSTR("True"));
            } else {
                ta_ui_next_margin(0, 0, 0, 0);
                ta_ui_next_bg_color(UI_STATE_NONE, 0.7f, 0.0f, 0.0f, 0.9f);
                ta_ui_next_bg_color(UI_STATE_INTERACT, 0.9f, 0.0f, 0.0f, 0.9f);
                ta_ui_label(0, CSTR("False"));
            }
            ta_ui_toggle_button_end(&model->cast_shadows);
        } else {
            ta_ui_label(0, CSTR("n/a"));
            ta_ui_label(0, CSTR("[?]"));
            if (ta_ui_last_frame_state().hover) {
                ta_ui_tooltip(CSTR("Model must be visible to cast shadows"));
            }
        }
    }

    ta_rigid_body *rigid_body = ta_game_component_try(RES_COMP_RIGID_BODY, entity_name);
    if (rigid_body) {
        ta_ui_row_begin();
        ta_ui_next_margin(2, 12, 0, 4);
        ta_ui_next_size(header_width, 0);
        ta_ui_next_bg_color(UI_STATE_ALL, 0.0f, 0.5f, 0.7f, 1.0f);
        ta_ui_label(0, CSTR("Rigid Body"));

        ta_ui_row_begin();
        ta_ui_next_size(label_width, 0);
        ta_ui_label(0, CSTR("mass:"));
        static ta_ui_textbox_state mass_editor = { 0 };
        ta_ui_textbox_float(0, &rigid_body->mass, &mass_editor, 0);
        rigid_body->mass = MAX(0.0f, rigid_body->mass);

        ta_ui_row_begin();
        ta_ui_next_size(label_width, 0);
        ta_ui_label(0, CSTR("density:"));
        static ta_ui_textbox_state density_editor = { 0 };
        ta_ui_textbox_float(0, &rigid_body->density, &density_editor, 0);

        char text[64] = { 0 };
        int text_len = 0;

        ta_ui_row_begin();
        ta_ui_next_size(label_width, 0);
        ta_ui_label(0, CSTR("velocity:"));
        text_len = snprintf(CSTR(text), "%9.6f, %9.6f, %9.6f",
            rigid_body->velocity.x,
            rigid_body->velocity.y,
            rigid_body->velocity.z);
        DLB_ASSERT(text_len < sizeof(text));
        ta_ui_label(0, CSTR(text));
        ta_ui_next_margin(6, 1, 0, 1);
        ta_rgba velc = TA_COLOR_DARK_RED;
        ta_ui_next_bg_color(UI_STATE_NONE, velc.r, velc.g, velc.b, velc.a);
        ta_ui_next_bg_color(UI_STATE_INTERACT, 0.8f, 0.0f, 0.0f, 0.9f);
        if (ta_ui_button(0, CSTR("Reset"))) {
            rigid_body->velocity = VEC3_ZERO;
            rigid_body->ang_velocity = VEC3_ZERO;
        }

        ta_ui_row_begin();
        ta_ui_next_size(label_width, 0);
        ta_ui_label(0, CSTR("ang. velocity:"));
        text_len = snprintf(CSTR(text), "%9.6f, %9.6f, %9.6f",
            rigid_body->ang_velocity.x,
            rigid_body->ang_velocity.y,
            rigid_body->ang_velocity.z);
        DLB_ASSERT(text_len < sizeof(text));
        ta_ui_label(0, CSTR(text));

        ta_ui_row_begin();
        ta_ui_next_size(label_width, 0);
        ta_ui_label(0, CSTR("apply gravity:"));
        ta_ui_next_pad(0, 0, 0, 0);
        ta_ui_toggle_button_begin(0, TA_UI_AUTOSIZE);
        if (!rigid_body->no_gravity) {
            ta_ui_next_margin(0, 0, 0, 0);
            ta_ui_next_bg_color(UI_STATE_NONE, 0.0f, 0.5f, 0.0f, 0.9f);
            ta_ui_next_bg_color(UI_STATE_INTERACT, 0.0f, 0.7f, 0.0f, 0.9f);
            ta_ui_label(0, CSTR("True"));
        } else {
            ta_ui_next_margin(0, 0, 0, 0);
            ta_ui_next_bg_color(UI_STATE_NONE, 0.7f, 0.0f, 0.0f, 0.9f);
            ta_ui_next_bg_color(UI_STATE_INTERACT, 0.9f, 0.0f, 0.0f, 0.9f);
            ta_ui_label(0, CSTR("False"));
        }
        ta_ui_toggle_button_end(&rigid_body->no_gravity);

        ta_ui_row_begin();
        ta_ui_next_size(label_width, 0);
        ta_ui_label(0, CSTR("broadphase collide:"));
        rigid_body->dbg_broadphase ? ta_ui_label(0, SYM(SYM_TRUE)) : ta_ui_label(0, SYM(SYM_FALSE));

        ta_ui_row_begin();
        ta_ui_next_size(label_width, 0);
        ta_ui_label(0, CSTR("broad aabb center:"));
        static ta_ui_textbox_vec3_state broad_center_editor = { 0 };
        ta_ui_textbox_vec3(&rigid_body->aabb.center, &broad_center_editor, false, true, true);

        ta_ui_row_begin();
        ta_ui_next_size(label_width, 0);
        ta_ui_label(0, CSTR("broad aabb extents:"));
        static ta_ui_textbox_vec3_state broad_extents_editor = { 0 };
        ta_ui_textbox_vec3(&rigid_body->aabb.extents, &broad_extents_editor, false, true, false);

        ta_ui_row_begin();
        ta_ui_next_size(label_width, 0);
        ta_ui_label(0, CSTR("narrowphase collide:"));
        rigid_body->dbg_narrowphase ? ta_ui_label(0, SYM(SYM_TRUE)) : ta_ui_label(0, SYM(SYM_FALSE));

        ta_ui_row_begin();
        ta_ui_next_size(label_width, 0);
        ta_ui_label(0, CSTR("centroid local:"));
        static ta_ui_textbox_vec3_state centroid_local_editor = { 0 };
        ta_ui_textbox_vec3(&rigid_body->centroid_local, &centroid_local_editor, false, false, false);

        ta_ui_row_begin();
        ta_ui_next_size(label_width, 0);
        ta_ui_label(0, CSTR("centroid global:"));
        static ta_ui_textbox_vec3_state centroid_global_editor = { 0 };
        ta_ui_textbox_vec3(&rigid_body->centroid_global, &centroid_global_editor, false, false, false);

        ta_ui_row_begin();
        ta_ui_next_margin(2, 12, 0, 4);
        ta_ui_next_size(header_width, 0);
        ta_ui_next_bg_color(UI_STATE_ALL, 0.0f, 0.5f, 0.7f, 1.0f);
        ta_ui_label(0, CSTR("Collider"));

        ta_ui_row_begin();
        ta_ui_next_size(label_width, 0);
        ta_ui_label(0, CSTR("type:"));
        const char *collider_type = ta_collider_type_str(rigid_body->collider.type);
        ta_ui_label(0, collider_type, strlen(collider_type));

        ta_ui_row_begin();
        ta_ui_next_size(label_width, 0);
        ta_ui_label(0, CSTR("center:"));
        static ta_ui_textbox_vec3_state center_editor = { 0 };
        ta_ui_textbox_vec3(&rigid_body->collider.data.center, &center_editor, false, true, true);

        switch (rigid_body->collider.type) {
            case TA_COLLIDER_PLANE: {
                ta_ui_row_begin();
                ta_ui_label(0, CSTR("normal:"));
                static ta_ui_textbox_vec3_state normal_editor = { 0 };
                ta_ui_textbox_vec3(&rigid_body->collider.data.plane.normal, &normal_editor, true, true, false);
                break;
            } case TA_COLLIDER_SPHERE: {
                ta_ui_row_begin();
                ta_ui_next_size(label_width, 0);
                ta_ui_label(0, CSTR("radius:"));
                static ta_ui_textbox_state radius_editor = { 0 };
                ta_ui_textbox_float(0, &rigid_body->collider.data.sphere.radius, &radius_editor, 0);
                break;
            } case TA_COLLIDER_OBB: {
                ta_ui_row_begin();
                ta_ui_label(0, CSTR("extents:"));
                static ta_ui_textbox_vec3_state extents_editor = { 0 };
                ta_ui_textbox_vec3(&rigid_body->collider.data.obb.extents, &extents_editor, false, true, false);
                ta_ui_row_begin();
                ta_ui_label(0, CSTR("orientation:"));
                static ta_ui_textbox_vec4_state orientation_editor = { 0 };
                ta_ui_textbox_vec4(&rigid_body->collider.data.obb.orientation, &orientation_editor, true, true, true);
                break;
            } default: {
                break;
            }
        }
    }

    ta_light *light = ta_game_component_try(RES_COMP_LIGHT, entity_name);
    if (light) {
        ta_ui_row_begin();
        ta_ui_next_margin(2, 12, 0, 4);
        ta_ui_next_size(header_width, 0);
        ta_ui_next_bg_color(UI_STATE_ALL, 0.0f, 0.5f, 0.7f, 1.0f);
        ta_ui_label(0, CSTR("Light"));

        ta_ui_row_begin();
        ta_ui_next_size(label_width, 0);
        ta_ui_label(0, CSTR("enabled:"));
        if (light->disabled) {
            if (ta_ui_button(0, CSTR("False"))) light->disabled = false;
        } else {
            if (ta_ui_button(0, CSTR("True"))) light->disabled = true;
        }

        ta_ui_row_begin();
        ta_ui_next_size(label_width, 0);
        ta_ui_label(0, CSTR("shadow map:"));
        static bool show_shadow_map = true;
        if (show_shadow_map) {
            if (ta_ui_button(0, CSTR("Hide"))) show_shadow_map = false;
        } else {
            if (ta_ui_button(0, CSTR("Show"))) show_shadow_map = true;
        }

        if (show_shadow_map) {
            ta_ui_row_begin();
            ta_ui_next_pad(1, 1, 1, 1);
            static ta_ui_panel_state shadowmap_panel = { 0 };
            ta_ui_panel_begin(0, &shadowmap_panel, TA_UI_AUTOSIZE);

            // Render cubemap with the following layout:
            //       ┌────┐                 ┌────┐
            //       | +Y |                 |  2 |
            //  ┌────┼────┼────┬────┐  ┌────┼────┼────┬────┐
            //  | -X | -Z | +X | +Z |  |  1 |  5 |  0 |  4 |
            //  └────┼────┼────┴────┘  └────┼────┼────┴────┘
            //       | -Y |                 |  3 |
            //       └────┘                 └────┘
            s32 resolution = light->shadowmap.resolution / 10;

            #define face_image(face) \
                ta_ui_next_size(resolution, resolution); \
                ta_ui_next_margin(0, 0, 0, 0); \
                ta_ui_next_pad(0, 0, 0, 0); \
                ta_ui_image(0, &light->shadowmap.texture, face);

            ta_ui_row_begin();
            ta_ui_spacer(resolution, 0);
            face_image(2);
            ta_ui_row_begin();
            face_image(1);
            face_image(5);
            face_image(0);
            face_image(4);
            ta_ui_row_begin();
            ta_ui_spacer(resolution, 0);
            face_image(3);

            #undef face_image

            ta_ui_panel_end();
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
    ta_ui_panel_begin(0, &audio_panel, TA_UI_AUTOSIZE);

    const char *audio_request_name = 0;

    ta_ui_row_begin();
    dlb_vec_each(ta_audio_buffer *, audio_buffer,
        ta_game_resource_pool(RES_AUDIO_BUFFER))
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
        ta_ui_toggle_button_begin(0, TA_UI_AUTOSIZE);
        ta_ui_image(0, ta_game_by_sym(RES_TEXTURE, tg_tex_audio_icon), 0);
        if (ta_ui_toggle_button_end(&active)) {
            audio_request_name = audio_buffer->name;
        }
        if (ta_ui_last_frame_state().hover) {
            ta_ui_tooltip(SYM(audio_buffer->path));
        }
    }

    if (audio_request_name) {
        ta_audio_source *bg_music_src = ta_game_component(RES_COMP_AUDIO_SOURCE,
            tg_e_background_music);
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
    ta_ui_panel_begin(0, &camera_panel, TA_UI_AUTOSIZE);
    static const char *selected_camera = 0;

    //ta_ui_row_begin();
    dlb_vec_each(ta_camera *, camera, ta_game_resource_pool(RES_COMP_CAMERA)) {
        ta_ui_next_size(120, 0);
        bool selected = camera->name == selected_camera;
        ta_ui_toggle_button_begin(0, TA_UI_AUTOSIZE_H);
        ta_ui_label(0, SYM(camera->name));
        if (ta_ui_toggle_button_end(&selected)) {
            if (selected) {
                selected_camera = camera->name;
            }
        }
        if (ta_ui_last_frame_state().hover) {
            // TODO: useful tooltip for camera
        }
    }

    if (selected_camera) {
        ta_camera *camera = ta_game_by_sym(RES_COMP_CAMERA, selected_camera);

        ta_ui_row_begin();
        static ta_ui_panel_state selected_camera_panel = { 0 };
        ta_ui_panel_begin(0, &selected_camera_panel, TA_UI_AUTOSIZE);

        ta_ui_row_begin();
        static ta_ui_panel_state label_panel = { 0 };
        ta_ui_panel_begin(0, &label_panel, TA_UI_AUTOSIZE);
        ta_ui_label(0, CSTR("Name"));
        ta_ui_label(0, CSTR("Entity name"));
        ta_ui_label(0, CSTR("Position"));
        ta_ui_label(0, CSTR("Position smooth"));
        ta_ui_label(0, CSTR("Position target vel"));
        ta_ui_label(0, CSTR("Yaw smooth"));
        ta_ui_label(0, CSTR("Pitch smooth"));
        ta_ui_label(0, CSTR("FOV"));
        ta_ui_label(0, CSTR("Z near"));
        ta_ui_label(0, CSTR("Debug channel"));
        ta_ui_panel_end();

        static ta_ui_panel_state button_panel = { 0 };
        ta_ui_panel_begin(0, &button_panel, TA_UI_AUTOSIZE);
        ta_ui_label(0, SYM(camera->name));
        ta_ui_label(0, SYM(camera->entity_name));
        static ta_ui_textbox_vec3_state pos_textbox = { 0 };
        ta_ui_row_begin();
        ta_ui_textbox_vec3(&camera->position, &pos_textbox, false, false, true);
        ta_ui_row_end();
        static ta_ui_textbox_state pos_smooth_textbox = { 0 };
        ta_ui_textbox_float(0, &camera->position_smooth, &pos_smooth_textbox, 0);
        ta_ui_row_begin();
        static ta_ui_textbox_state pos_target_vel_textbox = { 0 };
        ta_ui_textbox_float(0, &camera->position_target_vel, &pos_target_vel_textbox, 0);
        ta_ui_next_margin(8, 1, 0, 0);
        if (ta_ui_button(0, CSTR("Slow"))) {
            camera->position_target_vel = 0.01f;
        }
        ta_ui_next_margin(8, 1, 0, 0);
        if (ta_ui_button(0, CSTR("Normal"))) {
            camera->position_target_vel = 0.3f;
        }
        ta_ui_next_margin(8, 1, 0, 0);
        if (ta_ui_button(0, CSTR("Fast"))) {
            camera->position_target_vel = 1.0f;
        }
        ta_ui_row_end();
        static ta_ui_textbox_state yaw_smooth_textbox = { 0 };
        ta_ui_textbox_float(0, &camera->yaw_smooth, &yaw_smooth_textbox, 0);
        static ta_ui_textbox_state pitch_smooth_textbox = { 0 };
        ta_ui_textbox_float(0, &camera->pitch_smooth, &pitch_smooth_textbox, 0);
        static ta_ui_textbox_state fov_textbox = { 0 };
        ta_ui_textbox_float(0, &camera->fov, &fov_textbox, 0);
        static ta_ui_textbox_state znear_textbox = { 0 };
        ta_ui_row_begin();
        ta_ui_textbox_float(0, &camera->znear, &znear_textbox, 0);
        ta_ui_next_margin(8, 1, 0, 0);
        if (ta_ui_button(0, CSTR("Recalc projection matrix"))) {
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
            [DBG_MTL_METALLIC]   = { CSTR("Metallic") },
            [DBG_MTL_ROUGHNESS]  = { CSTR("Roughness") },
            [DBG_MTL_OCCLUSION]  = { CSTR("Occlusion") },
        };

        for (int mode = 0; mode < ARRAY_COUNT(dbg_modes); ++mode) {
            if (mode % 2 == 1) ta_ui_row_begin();
            if (mode == 0) {
                ta_ui_next_size(246, 0);
            } else {
                ta_ui_next_size(120, 0);
            }
            ta_ui_toggle_button_begin(0, TA_UI_AUTOSIZE_H);
            ta_ui_label(0, dbg_modes[mode].text, dbg_modes[mode].len);
            bool checked = mode == camera->debug_channel;
            if (ta_ui_toggle_button_end(&checked)) {
                camera->debug_channel = mode;
            }
            if (mode == 0) {
                ta_ui_row_begin();
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
    ta_ui_panel_begin(0, &material_panel, TA_UI_AUTOSIZE);
    dlb_vec_each(ta_material *, material, ta_game_resource_pool(RES_MATERIAL)) {
        ta_ui_row_begin();
        ta_ui_next_size(200, 0);
        ta_ui_next_margin(0, 2, 0, 0);
        ta_ui_next_pad(4, 4, 4, 4);
        //ta_ui_next_size(material->width, material->height);
        if (ta_ui_button(0, SYM(material->name))) {
            const char *entity_name = ta_editor_selected_entity();
            if (entity_name) {
                ta_model *model = ta_game_component_try(RES_COMP_MODEL,
                    entity_name);
                if (model) {
                    model->material = material->name;
                }
            }
        }
        if (ta_ui_last_frame_state().hover) {
            char tex_buf[1024] = { 0 };
            int len = snprintf(tex_buf, sizeof(tex_buf),
                "name         : %s\n"
                "shader       : %s\n"
                "tex_albedo   : %s\n"
                "tex_height   : %s\n"
                "tex_metallic : %s\n"
                "tex_normal   : %s\n"
                "tex_occlusion: %s\n"
                "tex_roughness: %s",
                material->name,
                material->shader,
                material->tex_albedo,
                material->tex_height,
                material->tex_metallic,
                material->tex_normal,
                material->tex_occlusion,
                material->tex_roughness
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
    ta_ui_panel_begin(0, &mesh_panel, TA_UI_AUTOSIZE);
    dlb_vec_each(ta_mesh *, mesh, ta_game_resource_pool(RES_MESH)) {
        ta_ui_row_begin();
        ta_ui_next_size(200, 0);
        ta_ui_next_margin(0, 2, 0, 0);
        ta_ui_next_pad(4, 4, 4, 4);
        //ta_ui_next_size(material->width, material->height);
        if (ta_ui_button(0, SYM(mesh->name))) {
            const char *entity_name = ta_editor_selected_entity();
            if (entity_name) {
                ta_model *model = ta_game_component_try(RES_COMP_MODEL,
                    entity_name);
                if (model) {
                    dlb_vec_clear(model->meshes);
                    dlb_vec_push(model->meshes, mesh->name);
                }
            }
        }
        if (ta_ui_last_frame_state().hover) {
            char tex_buf[1024] = { 0 };
            int len = snprintf(tex_buf, sizeof(tex_buf),
                "name         : %s\n"
                "vertex count : %u",
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
    ta_ui_panel_begin(0, &texture_panel, TA_UI_AUTOSIZE_W);
    int count = 0;
    dlb_vec_each(ta_texture *, texture, ta_game_resource_pool(RES_TEXTURE)) {
        if (count % 4 == 0) {
            ta_ui_row_begin();
        }
        ta_ui_next_size(68, 68);
        //ta_ui_next_margin(0, 0, 2, 0);
        ta_ui_next_pad(4, 4, 4, 4);
        //ta_ui_next_size(texture->width, texture->height);
        ta_ui_button_begin(0, 0);
        ta_ui_next_size(68, 68);
        ta_ui_image(0, texture, 0);
        if (ta_ui_button_end()) {
            const char *entity_name = ta_editor_selected_entity();
            if (entity_name) {
                ta_model *model = ta_game_component_try(RES_COMP_MODEL,
                    entity_name);
                if (model && model->material) {
                    ta_material *material = ta_game_by_sym(RES_MATERIAL,
                        model->material);
                    material->tex_albedo = texture->name;
                }
            }
        }
        if (ta_ui_last_frame_state().hover) {
            char tex_buf[256] = { 0 };
            int len = snprintf(tex_buf, sizeof(tex_buf),
                "name: %s\n"
                "path: %s\n"
                "glid: %d",
                texture->name,
                texture->path,
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
    ta_ui_panel_begin(0, &textbox_panel, TA_UI_AUTOSIZE);

    ta_ui_row_begin();
    ta_ui_label(0, CSTR("Text:"));
    ta_ui_next_size(300, 0);
    //ta_ui_next_margin(4, 0, 0, 2);
    static ta_ui_textbox_state textbox = { 0 };
    static char buf[] = "The quick brown fox jumps over the lazy dog. 1234567890 |||";
    if (ta_ui_textbox(0, CSTR(buf), &textbox, 0)) {
        u32 text_len = dlb_vec_len(textbox.buffer);
        //dlb_memcpy(buf, textbox.buffer, MAX(sizeof(buf) - 1, text_len));
        ta_ui_textbox_clear(&textbox);
    }
    ta_ui_row_end();

    char tb_buffer[20] = { 0 };
    snprintf(CSTR(tb_buffer), "Length: %d", dlb_vec_len(textbox.buffer));
    ta_ui_label(0, CSTR(tb_buffer));

    char tb_cursor[20] = { 0 };
    snprintf(CSTR(tb_cursor), "Cursor: %d", textbox.cursor);
    ta_ui_label(0, CSTR(tb_cursor));

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
    ta_ui_panel_begin(INTERN("editor_sidebar"), &category_panel, TA_UI_AUTOSIZE);
    for (int i = 0; i < ARRAY_COUNT(categories); i++) {
        if (i % 4 == 0) {
            ta_ui_row_begin();
            ta_ui_next_margin(0, 0, 0, 2);
        } else {
            ta_ui_next_margin(2, 0, 0, 2);
        }
        ta_ui_next_size(100, 0);
        ta_ui_next_pad(0, 0, 0, 0);
        ta_ui_toggle_button_begin(0, TA_UI_AUTOSIZE_H);
        ta_ui_label(0, categories[i].name, categories[i].len);
        bool active = (i == category_selected);
        ta_ui_toggle_button_end(&active);
        if (active && category_selected != i) {
            category_selected = i;
            if (editor.textbox_editing) {
                ta_ui_textbox_clear(editor.textbox_editing);
            }
        }
    }
    ta_ui_panel_end();

    ta_ui_row_begin();
    categories[category_selected].panel_method();
}
void ta_editor_draw(float alpha)
{
    GLint polygon_mode = 0;
    glGetIntegerv(GL_POLYGON_MODE, &polygon_mode);

    if (editor.textbox_editing) {
        ta_ui_set_cursor(UI_CURSOR_IBEAM);
    } else {
        ta_ui_set_cursor(UI_CURSOR_ARROW);
    }

    // Render selected entity as yellow wireframes
    const char *selected_entity = ta_editor_selected_entity();
    if (selected_entity) {
        ta_camera *camera = ta_game_camera();
        ta_model *model = ta_game_component_try(RES_COMP_MODEL, selected_entity);
        if (model) {
            glClear(GL_DEPTH_BUFFER_BIT);
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

            ta_shader *shader = ta_scene_find_by_name(&editor.scene, RES_SHADER,
                SYM(editor.shader_editor_select));
            ta_rgba wire_color = TA_COLOR_YELLOW;
            double seconds = ta_timer_elapsed_sec();
            double sine = sin(seconds * 4.0) * 0.5 + 0.5;
            wire_color.a = (float)(0.25 * (sine * sine) + 0.02);
            if (camera->debug_no_mesh) {
                wire_color.a = 0.05f;
            }
            ta_shader_set_vec4(shader, SYM_U_COLOR, (ta_vec4 *)&wire_color);
            ta_model_render_shader(model, camera, shader, alpha, 1.0f);
        }
    }
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    ta_ui_spacer(0, 50);
    //ta_ui_next_size(400, 400);
    ta_ui_next_margin(0, 50, 0, 0);
    //ta_ui_next_pad(2, 2, 2, 2);
    static ta_ui_window_state window = { 0 };
    ta_ui_window_begin(INTERN("test_window"), &window, TA_UI_AUTOSIZE);
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
    ta_ui_label(0, CSTR("Font:"));
    ta_ui_label(0, SYM(font_current->name));

    ta_ui_row_begin();
    ta_ui_next_size(120, 28);
    ta_ui_button_begin(0, 0);
    ta_ui_label(0, CSTR("Prev Font"));
    if (ta_ui_button_end()) {
        if (cur_font_idx) {
            cur_font_idx--;
        } else {
            cur_font_idx = fonts_count - 1;
        }
    }
    ta_ui_next_size(120, 28);
    ta_ui_button_begin(0, 0);
    ta_ui_label(0, CSTR("Next Font"));
    if (ta_ui_button_end()) {
        if (cur_font_idx < fonts_count - 1) {
            cur_font_idx++;
        } else {
            cur_font_idx = 0;
        }
    }
#endif

    ta_ui_window_end();

    glClear(GL_DEPTH_BUFFER_BIT);
    ta_ui_render();

    glPolygonMode(GL_FRONT_AND_BACK, polygon_mode);
}

void editor_command_close()
{
    // NOTE: This can't happen at the moment because textbox cancel and editor
    // close are both bound to Escape. Just to be safe.
    if (editor.textbox_editing) {
        ta_ui_textbox_clear(editor.textbox_editing);
    }
    ta_game_state_set(ta_game_state_prev());
}
void editor_command_select()
{
    if (!ta_mouse_captured())
        return;

    ta_camera *camera = ta_game_camera();
    ta_ray ray;
    ray.origin = camera->position;
    ray.direction = camera->front;

    float t_min = 9999.0f;
    const char *closest_entity = 0;

    ta_rigid_body *bodies = ta_game_resource_pool(RES_COMP_RIGID_BODY);
    dlb_vec_each(ta_rigid_body *, body, bodies) {
        ta_sphere sphere = { 0 };
        switch (body->collider.type) {
            case TA_COLLIDER_SPHERE: {
                sphere.center = body->centroid_global;
                sphere.radius = body->collider.data.sphere.radius;
                break;
            } case TA_COLLIDER_OBB: {
                // TODO: Ray vs. OBB for more accurate picking
                sphere.center = body->centroid_global;
                sphere.radius = vec3_len(body->collider.data.obb.extents);
                break;
            } default: {
                // Ignore unsupported colliders when picking
                continue;
            }
        }

        float t;
        if (ta_intersect_ray_sphere(ray, sphere, &t)) {
            if (t >= 0.0f && t < t_min) {
                t_min = t;
                closest_entity = body->entity_name;
            }
        }
    }

    ta_light *lights = ta_game_resource_pool(RES_COMP_LIGHT);
    dlb_vec_each(ta_light *, light, lights) {
        ta_transform *transform = ta_game_component(RES_COMP_TRANSFORM,
            light->entity_name);
        ta_sphere sphere = { 0 };
        sphere.center = transform->xform.position;
        sphere.radius = 0.2f;
        float t;
        if (ta_intersect_ray_sphere(ray, sphere, &t)) {
            if (t >= 0.0f && t < t_min) {
                t_min = t;
                closest_entity = light->entity_name;
            }
        }
    }

    if (!closest_entity) {
        static const char *chamber_0001 = 0;
        if (!chamber_0001) {
            chamber_0001 = INTERN("chamber_0001");
        }
        closest_entity = chamber_0001;
    }

    if (closest_entity) {
        ta_editor_select_entity(closest_entity);
    }
}
void editor_command_sim_pause_resume()
{
    if (ta_game_sim_running()) {
        ta_game_sim_pause();
    } else {
        ta_game_sim_resume();
    }
}
void editor_command_sim_next()
{
    if (ta_game_sim_paused()) {
        ta_game_sim_step_n_frames(1);
    }
}
void editor_command_sim_next_ten()
{
    if (ta_game_sim_paused()) {
        ta_game_sim_step_n_frames(10);
    }
}
void editor_command_sim_while_held()
{
    if (ta_game_sim_paused()) {
        ta_game_sim_step_n_frames(1);
    }
}
void ta_editor_hotkeys()
{
    // Don't trigger any hotkeys while a textbox is focused
    if (editor.textbox_editing || editor.textbox_dragging)
        return;

    static void (*commands[EDITOR_COMMAND_COUNT])() = {
        [EDITOR_COMMAND_CLOSE1]           = editor_command_close,
        [EDITOR_COMMAND_CLOSE2]           = editor_command_close,
        [EDITOR_COMMAND_SELECT]           = editor_command_select,
        [EDITOR_COMMAND_SIM_PAUSE_RESUME] = editor_command_sim_pause_resume,
        [EDITOR_COMMAND_SIM_NEXT]         = editor_command_sim_next,
        [EDITOR_COMMAND_SIM_NEXT_10]      = editor_command_sim_next_ten,
        [EDITOR_COMMAND_SIM_WHILE_HELD]   = editor_command_sim_while_held,
    };

    for (editor_command cmd = 0; cmd < EDITOR_COMMAND_COUNT; ++cmd) {
        ta_keybind_update(&editor.keybinds[cmd]);
        if (ta_keybind_triggered(&editor.keybinds[cmd])) {
            commands[cmd]();
        }
    }

    // Allow game hotkeys to be triggered in editor mode as well
    ta_game_hotkeys();
}

static bool ta_editor_textbox_event(ta_event *event)
{
    bool handled = false;

    switch (event->type) {
        case INPUT_EVENT_TEXT_INPUT: {
            ta_ui_textbox_insert(editor.textbox_editing, event->data.text_input.chr);
            handled = true;
            break;
        } case INPUT_EVENT_KEY_PRESS: {
            // Consume all unhandled keystrokes when text editor is active
            //if (event->data.key_press.scancode == SDL_SCANCODE_RETURN) {
            //    ta_ui_textbox_insert(editor.active_textbox, '\n');
            //}
            handled = true;
            break;
        } case INPUT_EVENT_KEY_RELEASE: {
            // Consume all unhandled keystrokes when text editor is active
            handled = true;
            break;
        } default: {
            handled = false;
        }
    }

    return handled;
}

void ta_editor_event(ta_event *event)
{
    if (editor.textbox_editing) {
        event->handled = ta_editor_textbox_event(event);
    }
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