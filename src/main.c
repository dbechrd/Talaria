/* JSMN_PARENT_LINKS is necessary to make parsing large structures linear in input size */
#define JSMN_PARENT_LINKS
/* JSMN_STRICT is necessary to reject invalid JSON documents */
#define JSMN_STRICT

#include "ta_timer.h"
#include "ta_log.h"
#include "ta_window.h"
#include "ta_audio.h"
#include "ta_render.h"
#include "ta_file.h"
#include "ta_scene.h"
#include "ta_shader.h"
#include "ta_ui_barchart.h"
#include "ta_texture.h"
#include "ta_mesh.h"
#include "ta_camera.h"
#include "ta_viewport.h"
#include "ta_event.h"
#include "ta_game.h"
#include "ta_keybind.h"
#include "ta_mouse.h"
#include "ta_light.h"
#include "ta_schema.h"
#include "ta_parse.h"
#include "ta_symbol.h"
#include "ta_font.h"
#include "ta_primitive.h"
#include "ta_editor.h"
#include "ta_rigid_body.h"
#include "ta_position.h"
#include "ta_model.h"
#include "ta_entity.h"
#include "ta_player.h"
#include "ta_json.h"
#include "ta_gltf.h"

#include "dlb/dlb_types.h"
#define DLB_MURMUR3_IMPLEMENTATION
#include "dlb/dlb_murmur3.h"
#define DLB_VECTOR_IMPLEMENTATION
#include "dlb/dlb_vector.h"
#define DLB_HASH_IMPLEMENTATION
#define DLB_HASH_TEST
#include "dlb/dlb_hash.h"
#define DLB_BITSET_TEST
#include "dlb/dlb_bitset.h"
#define DLB_INDEX_IMPLEMENTATION
#define DLB_INDEX_TEST
#include "dlb/dlb_index.h"
#include "misc/gl3w.h"
#include "SDL/SDL.h"

DLB_ASSERT_HANDLER(handle_assert)
{
    tg_debug_log.flush = true;
    ta_log_write(&tg_debug_log, SRC_ASSERT,
        "\n---[DLB_ASSERT_HANDLER]-----------------\n"
        "Source file: %s:%d\n\n"
        "%s\n"
        "----------------------------------------\n",
        filename, line, expr
    );
#if _DEBUG
    __debugbreak();
#else
    char buf[8192] = { 0 };
    snprintf(buf, sizeof(buf),
        "\n---[DLB_ASSERT_HANDLER]-----------------\n"
        "Source file: %s:%d\n\n"
        "%s\n"
        "----------------------------------------\n",
        filename, line, expr
    );
    ta_window_msgbox(tg_window, SDL_MESSAGEBOX_ERROR, "ASSERT", buf);
#endif
    exit(-1);
}
dlb_assert_handler_def *dlb_assert_handler = handle_assert;

void debug_tests() {
#if _DEBUG
    parse_tests();
    dlb_hash_test();
    //dlb_bitset_test();
    dlb_index_test();
    ta_math_test();
#endif
}

void ndc_tests() {
    DLB_ASSERT(SCREEN_WRAP_X(0) == 0);
    DLB_ASSERT(SCREEN_WRAP_X(1) == 1);
    DLB_ASSERT(SCREEN_WRAP_X(WINDOW_W) == WINDOW_W);
    DLB_ASSERT(SCREEN_WRAP_X(-1) == WINDOW_W - 1);

    DLB_ASSERT(SCREEN_WRAP_Y(0) == 0);
    DLB_ASSERT(SCREEN_WRAP_Y(1) == 1);
    DLB_ASSERT(SCREEN_WRAP_Y(WINDOW_H) == WINDOW_H);
    DLB_ASSERT(SCREEN_WRAP_Y(-1) == WINDOW_H - 1);
}

// Random thoughts
// https://en.wikipedia.org/wiki/Accumulator_(energy)

static void debug_nametag();

// NOTE: Only works in Subsystem:Console mode?
//#undef main

int main(int argc, char *argv[])
{
    UNUSED(argc);
    UNUSED(argv);
    DLB_ASSERT(SDL_NUM_SCANCODES == TA_SDL_NUM_SCANCODES);

    ta_timer_init();
    ta_log_init_file(&tg_debug_log, "log.txt", false, false, SRC_ALL,
        SRC_EVENT | SRC_SYSTEM);
    srand((u32)ta_timer_only_ms());  // TODO: Better seed if it matters

    ta_json_test();
    ta_gltf_test();

    ta_log_write(&tg_debug_log, SRC_SYSTEM, "Running debug_tests...\n");
    debug_tests();
    ta_log_write(&tg_debug_log, SRC_SYSTEM, "Initializing symbols...\n");
    ta_symbol_init();
    ta_log_write(&tg_debug_log, SRC_SYSTEM, "Registering schema...\n");
    ta_schema_register();
    // TODO: Save size/position to a config file
    ta_log_write(&tg_debug_log, SRC_SYSTEM, "Initializing window...\n");
    ta_window_init(tg_window, 1600, 900, false);
    ta_log_write(&tg_debug_log, SRC_SYSTEM, "Running ndc_tests...\n");
    ndc_tests();
    ta_log_write(&tg_debug_log, SRC_SYSTEM, "Initializing audio...\n");
    ta_audio_listener_init(&tg_audio);
    ta_log_write(&tg_debug_log, SRC_SYSTEM, "Initializing mouse...\n");
    ta_mouse_init();
    ta_log_write(&tg_debug_log, SRC_SYSTEM, "Initializing renderer...\n");
    ta_render_init();
    ta_log_write(&tg_debug_log, SRC_SYSTEM, "Initializing primitives...\n");
    ta_primitive_init();
    ta_log_write(&tg_debug_log, SRC_SYSTEM, "Initializing game...\n");
    ta_game_init();
    ta_log_write(&tg_debug_log, SRC_SYSTEM, "Initializing editor...\n");
    ta_editor_init();

    // TODO: Cleanup
    ta_ui_barchart chart = ta_ui_barchart_init(10, 10, MAX(0, WINDOW_W - 20), 30);
    UNUSED(chart);

    //ta_shader_set_sampler2d(tg_shader_mesh, SYM_U_TEX0, tex_test->gl_id);

    ta_game_loop();

    ta_log_flush(&tg_debug_log);

    // TODO: Free *EVERYTHING* (at least in debug mode.. to check memory leaks)
    ta_window_free(tg_window);
    ta_log_write(&tg_debug_log, SRC_SYSTEM, "Goodbye.\n\n");
    return 0;
}

static void debug_nametag(ta_camera *camera)
{
    ta_font *font = ta_game_by_name(RES_FONT, tg_font);
    static ta_rect_uv *tag_rects = 0;
    ta_rectf tag_rect = ta_font_push_text(&tag_rects, font,
        CSTR("Player 1\nis da best"), true, 0, 0, 0);

    ta_position *player_pos = ta_game_component(RES_COMP_POSITION, tg_e_player_one);

    ta_vec3 tag_pos = vec3_add(player_pos->transform.position,
        (ta_vec3){ 0.0f, 1.2f, 0.0f });
    ta_vec3 tag_to_cam = vec3_sub(camera->position, tag_pos);
    tag_to_cam.z *= -1.0f;
    tag_to_cam.y *= 0.0f;
    float tag_scalef = MAX(vec3_len(tag_to_cam), 4.0f);

    ta_vec3 tag_offset = tag_offset = vec3_scalef(camera->right,
        NDC_W(tag_rect.w) / 2.0f * tag_scalef);
    ta_vec3 tag_pos_off = vec3_sub(tag_pos, tag_offset);

    ta_mat4 tag_rot = mat4_lookat(VEC3_ZERO, tag_to_cam, VEC3_Y);

    ta_mat4 tag_trans_bg = mat4_translate(tag_pos_off);
    ta_mat4 tag_xform_bg = mat4_scalef(tag_scalef);
    tag_xform_bg = mat4_mul(&tag_rot, &tag_xform_bg);
    tag_xform_bg = mat4_mul(&tag_trans_bg, &tag_xform_bg);

    ta_vec3 tag_pos_off_fg = tag_pos_off;
    tag_pos_off_fg.y += NDC_H(tag_rect.h) * tag_scalef;
    ta_mat4 tag_trans_fg = mat4_translate(tag_pos_off_fg);
    ta_mat4 tag_xform_fg = mat4_scalef(tag_scalef);
    //tag_xform_fg = mat4_mul(&tag_xform_fg, &tag_rot_trans);
    tag_xform_fg = mat4_mul(&tag_rot, &tag_xform_fg);
    tag_xform_fg = mat4_mul(&tag_trans_fg, &tag_xform_fg);

    // Name tag background
    ta_shader_set_mat4(tg_shader_quads, SYM_U_PROJ, &camera->projection);
    ta_shader_set_mat4(tg_shader_quads, SYM_U_VIEW, &camera->look_at);
    ta_shader_set_mat4(tg_shader_quads, SYM_U_MODEL, &tag_xform_bg);
    ta_texture *tex_orange = ta_game_by_name(RES_TEXTURE, tg_tex_orange);
    ta_shader_set_sampler2d(tg_shader_quads, SYM_U_TEX, tex_orange->gl_id);
    ta_rect_uv tag_background = { 0 };
    tag_background.rect.x -= NDC_W(5.0f);
    tag_background.rect.w = NDC_W(tag_rect.w) + NDC_W(10.0f);
    tag_background.rect.h = NDC_H(tag_rect.h); //tg_game.font->pixel_height * 1.5f;
    ta_primitive_push_rect_uv(&quads_queue, tag_background, TA_COLOR_GRAY3A,
        UI_LAYER_HUD_BG, false, false);
    ta_primitive_render_quads(quads_queue, tg_shader_quads, true, true);
    ta_shader_set_sampler2d(tg_shader_quads, SYM_U_TEX, 0);

    // Name tag text
    ta_shader *font_shader = ta_game_by_name(RES_SHADER, font->shader);
    ta_shader_set_mat4(font_shader, SYM_U_PROJ, &camera->projection);
    ta_shader_set_mat4(font_shader, SYM_U_VIEW, &camera->look_at);
    ta_shader_set_mat4(font_shader, SYM_U_MODEL, &tag_xform_fg);

    // TODO: Move UI_LAYER_HUD out of push_rect_uv into tag_xform, or
    //       make font_render's xform arguments stack with current value
    //       of SYM_U_MODEL.
    dlb_vec_each(ta_rect_uv *, rect, tag_rects) {
        ta_primitive_push_rect_uv(&quads_queue, *rect, TA_COLOR_WHITE,
            UI_LAYER_HUD, true, true);
    }
    dlb_vec_zero(tag_rects);
    ta_font_render(quads_queue, font, 0, 0, 0, true, true);
}