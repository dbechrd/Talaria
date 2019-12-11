#include "ta_editor.h"
#include "ta_ui.h"
#include "ta_game.h"
#include "ta_scene.h"
#include "ta_light.h"
#include "ta_symbol.h"
#include "ta_audio.h"
#include "ta_texture.h"
#include "ta_material.h"
#include "ta_primitive.h"
#include "ta_window.h"
#include "ta_font.h"
#include "ta_shader.h"
#include "ta_parse.h"
#include "ta_rigid_body.h"
#include "ta_event.h"
#include "ta_mouse.h"
#include "ta_camera.h"
#include "ta_keybind.h"
#include "ta_log.h"
#include "ta_position.h"
#include "ta_entity.h"
#include "ta_model.h"
#include "SDL/SDL_keycode.h"
#include "dlb/dlb_vector.h"

typedef enum editor_command {
    EDITOR_COMMAND_CLOSE,
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
    ta_ui_textbox_state *active_textbox;
    ta_keybind keybinds[EDITOR_COMMAND_COUNT];
    const char *shader_editor_select;
    ta_scene scene;
} ta_editor;

typedef struct drag_float_state {
    float *value;       // pointer to float being dragged
    bool changed;       // true if float has been dragged at all
    ta_vec3 cam_offset; // offset of camera from selected object (for chase cam)
    float cam_position_smooth;      // original position_smooth
    float cam_position_target_vel;  // original position_target_vel
} drag_float_state;
static drag_float_state drag_float;

static ta_editor editor;

void ta_editor_init()
{
    ta_font *font = ta_game_by_name(RES_FONT, tg_font);
    ta_log_write(&tg_debug_log, SRC_EDITOR, "Initializing UI styles\n");
    ta_ui_init(font);

    ta_log_write(&tg_debug_log, SRC_EDITOR, "Loading editor scene\n");
    ta_scene_load_file(&editor.scene, "data/scene/editor.dml");
    editor.shader_editor_select = SYM_SHADER_EDITOR_SELECT;

    ta_log_write(&tg_debug_log, SRC_EDITOR, "Initializing key binds\n");

    // TODO: Read keybinds from file
    //dlb_vec_reserve(tg_keybinds, 16);

    // TODO: How to handle mapping multiple keybinds to the same event type? We
    // may be able to just handle escape key as special case?
    //ta_keybind_init1(&editor.keybinds[EDITOR_COMMAND_CLOSE           ], TA_KEYBIND_RELEASE, SDL_SCANCODE_GRAVE);
    ta_keybind_init1(&editor.keybinds[EDITOR_COMMAND_CLOSE           ], TA_KEYBIND_RELEASE, SDL_SCANCODE_ESCAPE);
    ta_keybind_init1(&editor.keybinds[EDITOR_COMMAND_SELECT          ], TA_KEYBIND_PRESS,   SDL_SCANCODE_MOUSE_LEFT);
    ta_keybind_init1(&editor.keybinds[EDITOR_COMMAND_SIM_PAUSE_RESUME], TA_KEYBIND_PRESS,   SDL_SCANCODE_F5);
    ta_keybind_init1(&editor.keybinds[EDITOR_COMMAND_SIM_NEXT        ], TA_KEYBIND_PRESS,   SDL_SCANCODE_F6);
    ta_keybind_init1(&editor.keybinds[EDITOR_COMMAND_SIM_NEXT_10     ], TA_KEYBIND_PRESS,   SDL_SCANCODE_F7);
    ta_keybind_init1(&editor.keybinds[EDITOR_COMMAND_SIM_WHILE_HELD  ], TA_KEYBIND_HOLD,    SDL_SCANCODE_F8);
}
void ta_editor_select_node(const char *entity_name)
{
    editor.selected_entity = entity_name;
}
const char *ta_editor_selected_entity()
{
#if 0
    // Clear selection if entity has been deleted
    if (!ta_game_by_name_try(tg_game.scene, RES_ENTITY,
        editor.selected_entity_name))
    {
        editor.selected_entity_name = 0;
    }
#endif
    return editor.selected_entity;
}

static void drag_float_begin(float *f)
{
    drag_float.value = f;
    drag_float.changed = false;
    ta_position *position = ta_game_component_try(RES_COMP_POSITION,
        editor.selected_entity);
    if (position) {
        ta_camera *active_cam = ta_game_camera();
        drag_float.cam_position_smooth = active_cam->position_smooth;
        drag_float.cam_position_target_vel = active_cam->position_target_vel;
        active_cam->position_smooth = 0.9f;
        active_cam->position_target_vel = 0.9f;
        drag_float.cam_offset = vec3_sub(active_cam->position,
            position->transform.position);
    } else {
        drag_float.cam_offset = VEC3_ZERO;
    }
    ta_mouse_drag_begin();
}
static void drag_float_update(float delta)
{
    if (!drag_float.value) return;

    int mouse_dx = ta_mouse_dx();
    if (mouse_dx) {
        float acc = 1.0f + ((int)vec3_len(drag_float.cam_offset) / 10);
        *drag_float.value += mouse_dx * delta * acc;
        drag_float.changed = true;

        ta_position *position = ta_game_component_try(RES_COMP_POSITION,
            editor.selected_entity);
        if (position) {
            ta_camera *active_cam = ta_game_camera();
            ta_vec3 cam_pos = vec3_add(position->transform.position,
                drag_float.cam_offset);
#if 1
            ta_camera_set_target_pos_absolute(active_cam, cam_pos);
#else
            ta_camera_set_position(active_cam, cam_pos.x, cam_pos.y, cam_pos.z);
#endif
        }
    }
}
static void drag_float_end()
{
    if (drag_float.value) {
        drag_float.value = 0;
        drag_float.changed = false;
        drag_float.cam_offset = VEC3_ZERO;
        if (drag_float.cam_position_smooth) {
            ta_camera *active_cam = ta_game_camera();
            active_cam->position_smooth = drag_float.cam_position_smooth;
            active_cam->position_target_vel = drag_float.cam_position_target_vel;
        }
        drag_float.cam_position_smooth = 0.0f;
        ta_mouse_drag_end();
    }
}
static void ui_node_panel()
{
    //ta_ui_next_margin(2, 2, 0, 0);
    static ta_ui_panel_state node_panel = { 0 };
    ta_ui_panel_begin(0, &node_panel, TA_UI_AUTOSIZE);

    static int label_width = 180;
    const char *entity_name = ta_editor_selected_entity();
    if (!entity_name) {
        //ta_ui_spacer(0, 2);
        ta_ui_row_begin();
        ta_ui_next_size(label_width, 0);
        ta_ui_label(0, CSTR("UID:"));
        ta_ui_label(0, CSTR("Nothing selected"));
        ta_ui_panel_end();
        return;
    }

    //ta_ui_spacer(0, 2);
    ta_ui_row_begin();
    ta_ui_next_size(label_width, 0);
    ta_ui_label(0, CSTR("Name:"));
#if 1
    ta_ui_label(0, SYM(entity_name));
#else
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

    //ta_ui_spacer(0, 2);
    ta_ui_row_begin();
    ta_ui_next_size(label_width, 0);

    ta_position *position = ta_game_component_try(RES_COMP_POSITION, entity_name);
    ta_rigid_body *rigid_body = ta_game_component_try(RES_COMP_RIGID_BODY, entity_name);
    ta_light *light = ta_game_component_try(RES_COMP_LIGHT, entity_name);
    ta_vec3 *pos_values = 0;
    if (rigid_body) {
        ta_ui_label(0, CSTR("Rigid Body Position:"));
        pos_values = &rigid_body->position;
    } else if (position) {
        ta_ui_label(0, CSTR("Position:"));
        pos_values = &position->transform.position;
    } else if (light) {
        ta_ui_label(0, CSTR("Light Position:"));
        pos_values = &light->position;
    }
    if (pos_values) {
        // TODO: Refactor this into ta_ui_vec3
        char x_str[16] = { 0 };
        int x_len = snprintf(x_str, sizeof(x_str), "%3.4f", pos_values->x);
        DLB_ASSERT(x_len < sizeof(x_str));
        ta_ui_label(0, CSTR("x:"));
        static ta_ui_textbox_state entry_x = { 0 };
        ta_ui_textbox(0, x_str, x_len, &entry_x, 0);

        if (entry_x.focused) {
            editor.active_textbox = &entry_x;
        } else if (editor.active_textbox == &entry_x) {
            editor.active_textbox = 0;
        }

        char y_str[16] = { 0 };
        int y_len = snprintf(y_str, sizeof(y_str), "%3.4f", pos_values->y);
        DLB_ASSERT(y_len < sizeof(y_str));
        ta_ui_label(0, CSTR("y:"));
        static ta_ui_textbox_state entry_y = { 0 };
        ta_ui_textbox(0, y_str, y_len, &entry_y, 0);

        char z_str[16] = { 0 };
        int z_len = snprintf(z_str, sizeof(z_str), "%3.4f", pos_values->z);
        DLB_ASSERT(z_len < sizeof(z_str));
        ta_ui_label(0, CSTR("z:"));
        static ta_ui_textbox_state entry_z = { 0 };
        ta_ui_textbox(0, z_str, z_len, &entry_z, 0);

#if 0

        //char pos_buf[32] = { 0 };
        //int len = snprintf(pos_buf, sizeof(pos_buf), "%3.4f", pos_values[i]);
        //DLB_ASSERT(len < sizeof(pos_buf));
        if (entry) {
            //ta_ui_next_pad(4, 1, 4, 1);

            if (ta_text_entry_valid(entry)) {
                ta_buffer text2 = ta_text_entry_text(entry);
                pos_values[i] = parse_float(text2);
            }
            // ta_ui_next_margin(4, 0, 0, 0);
            //ta_ui_next_pad(4, 1, 4, 1);
            ta_ui_next_bg_color(UI_STATE_INTERACT, 0.0f, 0.8f, 0.0f, 1.0f);
            if (ta_ui_label(0, CSTR("Save"))) {
                ta_text_entry_submit(entry);
            }
            if (ta_text_entry_submitted(entry)) {
                ta_text_entry_unfocus(entry);
                ta_text_entry_free(&entry);
            } else if (ta_text_entry_canceled(entry)) {
                ta_text_entry_free(&entry);
            }
        } else {
            //ta_ui_next_pad(4, 1, 4, 1);
            if (ta_ui_label(0, text, text_len)) {
                drag_float_begin(&pos_values[i]);
            } else if (drag_float.value == &pos_values[i] &&
                ta_key_released(SDL_SCANCODE_MOUSE_LEFT))
            {
                if (!drag_float.changed) {
                    DLB_ASSERT(!entry);
                    ta_text_entry_init(entry);
                    ta_text_entry_set_text(entry, text, text_len);
                    ta_text_entry_focus(entry);
                }
                drag_float_end();
            }
        }
#endif



    }

    drag_float_update(0.01f);

    if (position) {
        //ta_ui_spacer(0, 2);
        ta_ui_row_begin();
        ta_ui_next_size(label_width, 0);
        ta_ui_label(0, CSTR("Orientation:"));
        char orient_buf[64] = { 0 };
        int len = snprintf(orient_buf, sizeof(orient_buf),
            "x: %3.4f, y: %3.4f, z: %3.4f, w: %3.4f",
            position->transform.orientation.x,
            position->transform.orientation.y,
            position->transform.orientation.z,
            position->transform.orientation.w);
        DLB_ASSERT(len < sizeof(orient_buf));
        ta_ui_label(0, orient_buf, len);
    }

    if (light) {
        ta_ui_row_begin();
        ta_ui_next_size(label_width, 0);
        ta_ui_label(0, CSTR("Enabled:"));
        if (light->disabled) {
            if (ta_ui_label(0, CSTR("False"))) light->disabled = false;
        } else {
            if (ta_ui_label(0, CSTR("True"))) light->disabled = true;
        }

        ta_ui_row_begin();
        ta_ui_next_size(label_width, 0);
        ta_ui_label(0, CSTR("Shadow Map:"));
        static bool show_shadow_map = true;
        if (show_shadow_map) {
            if (ta_ui_label(0, CSTR("Hide"))) show_shadow_map = false;
        } else {
            if (ta_ui_label(0, CSTR("Show"))) show_shadow_map = true;
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
        ta_ui_button_toggle_begin(0, TA_UI_AUTOSIZE);
        ta_ui_image(0, ta_game_by_name(RES_TEXTURE, tg_tex_audio_icon), 0);
        ta_ui_button_toggle_end(&active);
        if (ta_ui_last_frame_state().pressed) {
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
static void ui_material_panel()
{
    //ta_ui_next_margin(2, 2, 0, 0);
    static ta_ui_panel_state material_panel = { 0 };
    ta_ui_panel_begin(0, &material_panel, TA_UI_AUTOSIZE);
    ta_ui_row_begin();
    dlb_vec_each(ta_material *, material, ta_game_resource_pool(RES_MATERIAL)) {
        ta_ui_next_size(68, 68);
        //ta_ui_next_margin(0, 0, 2, 0);
        //ta_ui_next_pad(2, 2, 2, 2);
        //ta_ui_next_size(material->width, material->height);
        if (ta_ui_button(0)) {
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
        ta_ui_next_pad(2, 2, 2, 2);
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
                    ta_material *material = ta_game_by_name(RES_MATERIAL,
                        model->material);
                    material->tex_albedo = texture->name;
                }
            }
        }
        if (ta_ui_last_frame_state().hover) {
            char tex_buf[256] = { 0 };
            int len = snprintf(tex_buf, sizeof(tex_buf),
                "name : %s\n"
                "path : %s\n"
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

    static ta_ui_textbox_state textbox = { 0 };
    ta_ui_row_begin();
    ta_ui_label(0, CSTR("Text:"));
    ta_ui_next_size(300, 0);
    //ta_ui_next_margin(4, 0, 0, 2);
    ta_ui_textbox(0, CSTR("This is a test."), &textbox, 0);
    ta_ui_panel_end();
}
static void ui_scene_panel()
{
    static ta_ui_panel_state scene_panel = { 0 };
    ta_ui_panel_begin(0, &scene_panel, TA_UI_AUTOSIZE);

    int col1_w = 90;
    int col2_w = 70;

    ta_ui_row_begin();
    ta_ui_next_size(col1_w, 17);
    ta_ui_label(0, CSTR("     Scene"));
    ta_ui_next_size(col2_w, 17);
    ta_ui_next_bg_color(UI_STATE_NONE, 0.0f, 0.5f, 0.0f, 0.9f);
    ta_ui_next_bg_color(UI_STATE_INTERACT, 0.0f, 0.7f, 0.0f, 0.9f);
    if (ta_ui_label(0, CSTR("Save"))) {
        ta_game_save();
    }

    ta_ui_row_begin();
    ta_ui_next_size(col1_w, 17);
    ta_ui_label(0, CSTR("Simulation"));
    ta_ui_next_size(col2_w, 17);
    if (ta_game_sim_running()) {
        ta_ui_next_bg_color(UI_STATE_NONE, 0.0f, 0.5f, 0.0f, 0.9f);
        ta_ui_next_bg_color(UI_STATE_INTERACT, 0.0f, 0.7f, 0.0f, 0.9f);
        if (ta_ui_label(0, CSTR("Running"))) {
            ta_game_sim_pause();
        }
    } else {
        ta_ui_next_bg_color(UI_STATE_NONE, 0.7f, 0.0f, 0.0f, 0.9f);
        ta_ui_next_bg_color(UI_STATE_INTERACT, 0.9f, 0.0f, 0.0f, 0.9f);
        if (ta_ui_label(0, CSTR("Paused"))) {
            ta_game_sim_resume();
        }
        ta_ui_next_bg_color(UI_STATE_NONE, 0.7f, 0.4f, 0.0f, 0.9f);
        ta_ui_next_bg_color(UI_STATE_INTERACT, 0.9f, 0.4f, 0.0f, 0.9f);
        if (ta_ui_label(0, CSTR("Next (1)"))) {
            ta_game_sim_step_n_frames(1);
        }
        ta_ui_next_bg_color(UI_STATE_NONE, 0.7f, 0.4f, 0.0f, 0.9f);
        ta_ui_next_bg_color(UI_STATE_INTERACT, 0.9f, 0.4f, 0.0f, 0.9f);
        if (ta_ui_label(0, CSTR("Next (10)"))) {
            ta_game_sim_step_n_frames(10);
        }
    }

    ta_ui_row_begin();
    ta_ui_next_size(col1_w, 17);
    ta_ui_label(0, CSTR("    V-Sync"));
    ta_ui_next_size(col2_w, 17);
    if (ta_window_vsync(tg_window)) {
        ta_ui_next_bg_color(UI_STATE_NONE, 0.0f, 0.5f, 0.0f, 0.9f);
        ta_ui_next_bg_color(UI_STATE_INTERACT, 0.0f, 0.7f, 0.0f, 0.9f);
        if (ta_ui_label(0, CSTR("On"))) {
            ta_window_set_vsync(tg_window, false);
        }
    } else {

        ta_ui_next_bg_color(UI_STATE_NONE, 0.7f, 0.0f, 0.0f, 0.9f);
        ta_ui_next_bg_color(UI_STATE_INTERACT, 0.9f, 0.0f, 0.0f, 0.9f);
        if (ta_ui_label(0, CSTR("Off"))) {
            ta_window_set_vsync(tg_window, true);
        }
    }

    ta_ui_row_begin();
    ta_ui_next_size(col1_w, 17);
    ta_ui_label(0, CSTR("     Audio"));
    ta_ui_next_size(col2_w, 17);
    if (tg_audio.muted) {
        ta_ui_next_bg_color(UI_STATE_NONE, 0.0f, 0.5f, 0.0f, 0.9f);
        ta_ui_next_bg_color(UI_STATE_INTERACT, 0.0f, 0.7f, 0.0f, 0.9f);
        if (ta_ui_label(0, CSTR("Unmute"))) {
            ta_audio_listener_unmute(&tg_audio);
        }
    } else {
        ta_ui_next_bg_color(UI_STATE_NONE, 0.7f, 0.0f, 0.0f, 0.9f);
        ta_ui_next_bg_color(UI_STATE_INTERACT, 0.9f, 0.0f, 0.0f, 0.9f);
        if (ta_ui_label(0, CSTR("Mute"))) {
            ta_audio_listener_mute(&tg_audio);
        }
    }

    char tex_buf[16] = { 0 };
    int len = snprintf(tex_buf, sizeof(tex_buf), "%.2f",
        ta_audio_listener_get_volume(&tg_audio));
    DLB_ASSERT(len < sizeof(tex_buf));

    ta_ui_row_begin();
    ta_ui_label(0, CSTR("    Volume"));
    if (tg_audio.muted) {
        ta_ui_label(0, tex_buf, len);
    }

    ta_ui_panel_end();
}
static void ui_editor_sidebar()
{
    enum {
        CATEGORY_NODE,
        CATEGORY_AUDIO,
        CATEGORY_MATERIALS,
        CATEGORY_TEXTURES,
        CATEGORY_TEXTBOX,
        CATEGORY_SCENE,
        CATEGORY_COUNT
    };
    const char *category_names[CATEGORY_COUNT] = { 0 };
    category_names[CATEGORY_NODE]      = INTERN(STRING(CATEGORY_NODE));
    category_names[CATEGORY_AUDIO]     = INTERN(STRING(CATEGORY_AUDIO));
    category_names[CATEGORY_MATERIALS] = INTERN(STRING(CATEGORY_MATERIALS));
    category_names[CATEGORY_TEXTURES]  = INTERN(STRING(CATEGORY_TEXTURES));
    category_names[CATEGORY_TEXTBOX]   = INTERN(STRING(CATEGORY_TEXTBOX));
    category_names[CATEGORY_SCENE]     = INTERN(STRING(CATEGORY_SCENE));
    static int category_selected = CATEGORY_NODE;

    ta_ui_row_begin();

    //ta_ui_next_size(50, 50);
    //ta_ui_next_pad(2, 2, 2, 2);
    static ta_ui_panel_state category_panel = { 0 };
    ta_ui_panel_begin(INTERN("editor_sidebar"), &category_panel, TA_UI_AUTOSIZE);
    for (int i = 0; i < CATEGORY_COUNT; i++) {
        ta_ui_row_begin();
        ta_ui_next_size(50, 50);
        //ta_ui_next_margin(0, 0, 0, 2);
        bool active = (i == category_selected);
        ta_ui_button_toggle(category_names[i], &active);
        if (active) {
            category_selected = i;
        }
        if (ta_ui_last_frame_state().hover) {
            ta_ui_tooltip(SYM(category_names[i]));

        }
    }
    ta_ui_panel_end();

    switch (category_selected) {
        case CATEGORY_NODE: {
            ui_node_panel();
            break;
        } case CATEGORY_AUDIO: {
            ui_audio_panel();
            break;
        } case CATEGORY_MATERIALS: {
            ui_material_panel();
            break;
        } case CATEGORY_TEXTURES: {
            ui_texture_panel();
            break;
        } case CATEGORY_TEXTBOX: {
            ui_textbox_panel();
            break;
        } case CATEGORY_SCENE: {
            ui_scene_panel();
        } default: {
            break;
        }
    }
}
// TODO: Move this to ta_ui_statusbar
static void ui_statusbar()
{
    if (editor.status_msg) {
        static ta_rect_uv *status_rects = 0;
        ta_font *font = ta_game_by_name(RES_FONT, tg_font);
        ta_rectf status_rect = ta_font_push_text(&status_rects, font,
            SYM(editor.status_msg), true, 0, 0, 0, 0);
        dlb_vec_each(ta_rect_uv *, rect, status_rects) {
            ta_primitive_push_rect_uv(&quads_queue, *rect, TA_COLOR_WHITE, 0,
                true, false);
        }
        dlb_vec_zero(status_rects);

        int status_halfw = WINDOW_W / 2 - (int)status_rect.w / 2;
        const int status_pad_bottom = 20;
        ta_font_render(quads_queue, font, (float)status_halfw,
            (float)(WINDOW_H - (font->ascent + status_pad_bottom)),
            UI_LAYER_TIP, true, true);

        editor.status_msg = 0;
    }
}
void ta_editor_draw(float alpha)
{
    // TODO: Render as yellow wireframe
    // Stencil selected entity
    const char *selected_entity = ta_editor_selected_entity();
    if (selected_entity) {
        ta_camera *camera = ta_game_camera();
        ta_model *model = ta_game_component_try(RES_COMP_MODEL, selected_entity);
        if (model) {
            glEnable(GL_STENCIL_TEST);
            glStencilFunc(GL_ALWAYS, 1, 0xFF);
            glStencilMask(0xFF);
            // Stencil the outline and any occluded fragments
            glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
            // Stencil just the outline
            //glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
            glDepthMask(GL_FALSE);
            glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
            ta_model_render(model, camera, alpha);
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            glDepthMask(GL_TRUE);

            glClear(GL_DEPTH_BUFFER_BIT);

            // Outline selected node
            glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
            glStencilMask(0x00);
            ta_shader *shader = ta_scene_find_by_name(&editor.scene, RES_SHADER,
                editor.shader_editor_select);
            ta_shader_set_vec4(shader, SYM_U_COLOR, (ta_vec4 *)&TA_COLOR_YELLOW);
            ta_model_render_shader(model, camera, shader, alpha, 1.1f);
            glDisable(GL_STENCIL_TEST);
        }
    }

    ta_ui_spacer(0, 50);
    //ta_ui_next_size(400, 400);
    ta_ui_next_margin(0, 50, 0, 0);
    //ta_ui_next_pad(2, 2, 2, 2);
    static ta_ui_window_state window = { 0 };
    ta_ui_window_begin(INTERN("test_window"), &window, TA_UI_AUTOSIZE);
    ui_editor_sidebar();
    ta_ui_window_end();

    glClear(GL_DEPTH_BUFFER_BIT);
    ta_ui_render();
}

void editor_command_close()
{
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
        // TODO: Handle types other than spheres
        if (body->collider.type != TA_COLLIDER_SPHERE) {
            continue;
        }

        ta_sphere sphere = body->collider.data.sphere;
        sphere.center = vec3_add(sphere.center, body->centroid_global);
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
        ta_sphere sphere = { 0 };
        sphere.center = light->position;
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
        ta_editor_select_node(closest_entity);
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
    static void (*commands[EDITOR_COMMAND_COUNT])() = {
        [EDITOR_COMMAND_CLOSE]            = editor_command_close,
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
}

static bool ta_editor_textbox_event(ta_event *event)
{
    bool handled = false;

    switch (event->type) {
        case INPUT_EVENT_TEXT_INPUT: {
            ta_ui_textbox_insert(editor.active_textbox, event->data.text_input.chr);
            handled = true;
            break;
        } case INPUT_EVENT_KEY_PRESS: {
            // Consume all unhandled keystrokes when text editor is active
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
    if (editor.active_textbox) {
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