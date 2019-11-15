#include "ta_timer.h"
#include "ta_log.h"
#include "ta_window.h"
#include "ta_audio.h"
#include "ta_render.h"
#include "ta_file.h"
#include "ta_scene.h"
#include "ta_shader.h"
#include "ta_text_entry.h"
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
    ta_log_write(&tg_debug_log, "ASSERT",
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

static void render_frame_info(u64 frame_num, double ms_frame_time,
    double ms_frame_delta, u64 sim_step);
static void debug_nametag();

// NOTE: Only works in Subsystem:Console mode?
//#undef main

int main(int argc, char *argv[])
{
    UNUSED(argc);
    UNUSED(argv);
    DLB_ASSERT(SDL_NUM_SCANCODES == TA_SDL_NUM_SCANCODES);

    tg_log_level.event = 0;
    tg_log_level.window = 0;
    tg_log_level.system = 0;

    ta_timer_init();
    ta_log_init_file(&tg_debug_log, "log.txt", false, false);
    srand((u32)ta_timer_only_ms());  // TODO: Better seed if it matters

    ta_log_write(&tg_debug_log, "System", "Running debug_tests...\n");
    debug_tests();
    ta_log_write(&tg_debug_log, "System", "Initializing symbols...\n");
    ta_symbol_init();
    ta_log_write(&tg_debug_log, "System", "Registering schema...\n");
    ta_schema_register();
    // TODO: Save size/position to a config file
    ta_log_write(&tg_debug_log, "System", "Initializing window...\n");
    ta_window_init(tg_window, 1600, 900, false);
    ta_log_write(&tg_debug_log, "System", "Running ndc_tests...\n");
    ndc_tests();
    ta_log_write(&tg_debug_log, "System", "Initializing audio...\n");
    tg_audio.volume = 0.05f;
    ta_audio_listener_init(&tg_audio);
    ta_log_write(&tg_debug_log, "System", "Initializing mouse...\n");
    ta_mouse_init();
    ta_log_write(&tg_debug_log, "System", "Initializing renderer...\n");
    ta_render_init();
    ta_log_write(&tg_debug_log, "System", "Initializing primitives...\n");
    ta_primitive_init();
    ta_log_write(&tg_debug_log, "System", "Initializing editor...\n");
    ta_editor_init();
    ta_log_write(&tg_debug_log, "System", "Initializing game...\n");
    ta_game_init(&tg_game);
    ta_log_write(&tg_debug_log, "System", "Loading first scene...\n");
    ta_scene *scene1 = ta_scene_load_file("data/scene/scene1_gen.dml");
    DLB_ASSERT(scene1 && tg_game.scene == scene1);

    tg_game.simulate = -1;

    // TODO: Find closest 8 lights and store them in tg_game.lights

    ////////////////////////////////////////////////////////////////////////////
    // Player
    ////////////////////////////////////////////////////////////////////////////
    // HACK: Find first entity with a player component, assume it's the player
    tg_game.e_player_one = SYM_ENTITY_PLAYER_ONE;
    DLB_ASSERT(tg_game.e_player_one);

    ////////////////////////////////////////////////////////////////////////////
    // Cameras
    ////////////////////////////////////////////////////////////////////////////
    // TODO: idk how best to find static resources other than by name. Maybe
    // have a lookup table in the scene file whose only purpose is to populate
    // the scene with ids of static objects (root node, free cam, player, etc.)?
    tg_game.e_freecam = SYM_ENTITY_FREECAM;
    DLB_ASSERT(tg_game.e_freecam);

    ////////////////////////////////////////////////////////////////////////////
    // Audio
    ////////////////////////////////////////////////////////////////////////////
    // TODO: Parent this node to the active player
    tg_game.e_background_music = SYM_ENTITY_BACKGROUND_MUSIC;
    DLB_ASSERT(tg_game.e_background_music);

    ta_audio_source *bg_music_src = ta_scene_component(tg_game.scene,
        RES_COMP_AUDIO_SOURCE, tg_game.e_background_music);
    DLB_ASSERT(bg_music_src);
    ta_audio_source_play_loop(bg_music_src);

    ////////////////////////////////////////////////////////////////////////////
    // Textures
    ////////////////////////////////////////////////////////////////////////////
    tg_game.font            = ta_scene_find_by_name(tg_game.scene, RES_FONT, INTERN("consola"));
    tg_game.tex_orange      = ta_scene_find_by_name(tg_game.scene, RES_TEXTURE, INTERN("test_diff"));
    tg_game.tex_red         = ta_scene_find_by_name(tg_game.scene, RES_TEXTURE, INTERN("test_mrao"));
    tg_game.tex_audio_icon  = ta_scene_find_by_name(tg_game.scene, RES_TEXTURE, INTERN("audio_icon"));
    DLB_ASSERT(tg_game.font);
    DLB_ASSERT(tg_game.tex_orange && tg_game.tex_orange->gl_id);
    DLB_ASSERT(tg_game.tex_red && tg_game.tex_red->gl_id);
    DLB_ASSERT(tg_game.tex_audio_icon && tg_game.tex_audio_icon->gl_id);

    ////////////////////////////////////////////////////////////////////////////
    // Shaders
    ////////////////////////////////////////////////////////////////////////////
    tg_shader_lines   = ta_scene_find_by_name(tg_game.scene, RES_SHADER, INTERN("lines"));
    tg_shader_quads   = ta_scene_find_by_name(tg_game.scene, RES_SHADER, INTERN("quads"));
    tg_shader_cubemap = ta_scene_find_by_name(tg_game.scene, RES_SHADER, INTERN("cubemap"));
    tg_shader_shadow  = ta_scene_find_by_name(tg_game.scene, RES_SHADER, INTERN("shadow"));
    DLB_ASSERT(tg_shader_lines);
    DLB_ASSERT(tg_shader_quads);
    DLB_ASSERT(tg_shader_cubemap);
    DLB_ASSERT(tg_shader_shadow);

#if _DEBUG
    ta_game_state_set(&tg_game, TA_GAME_STATE_FREE_CAM);
#else
    ta_game_state_set(&tg_game, TA_GAME_STATE_PLAY);
#endif
    ta_log_write(&tg_debug_log, "System", "Active camera: %s\n", tg_game.e_active_camera);
    DLB_ASSERT(tg_game.e_active_camera);

    ////////////////////////////////////////////////////////////////////////////
    // UI
    ////////////////////////////////////////////////////////////////////////////
    // TODO: Move this to DML (e.g. editor.dml)
    ta_camera minimap_camera = { 0 };
    minimap_camera.fov = 90.0f;
    minimap_camera.up = VEC3_NZ;
    minimap_camera.ortho = true;
    ta_camera_init(&minimap_camera);

    ta_ui_barchart chart = ta_ui_barchart_init(10, 10, WINDOW_W - 20, 30);
    UNUSED(chart);

    //ta_shader_set_sampler2d(tg_shader_mesh, SYM_U_TEX0, tex_test->gl_id);

    ////////////////////////////////////////////////////////////////////////////
    // Main loop
    ////////////////////////////////////////////////////////////////////////////
    u64 frame_num = 0;
    u64 sim_step = 0;

    // Eric Catto - Soft Constraints (GDC 2011)
    // Semi-implicit Euler will eventually blow up if you take big time steps. A
    // general rule is to take at least 4 time steps per period of oscillation.
    // For example, if the oscillation frequency is 60Hz, then you shouldn’t
    // take time steps slower than 15Hz.
    //
    // Randy Gaul
    // https://gamedevelopment.tutsplus.com/series/how-to-create-a-custom-physics-engine--gamedev-12715
    const double ms_sim_dt = 20;             // fixed dt milliseconds
    const double sim_dt = ms_sim_dt / 1000;  // fixed dt seconds
    const double sim_max_steps = 0;          // max simulation steps per frame
    double ms_sim_t = 0;                     // current simulation time
    double ms_frame_accum = 0;

    double ms_frame_prev = 0;   // Last frame started
    double ms_frame_start;      // This frame started
    double ms_frame_delta;      // Total delta time (including v-sync)
    double ms_frame_time;       // Actual frame time before v-sync

    while (tg_game.state != TA_GAME_STATE_SHUTDOWN) {
        ms_frame_start = ta_timer_elapsed_ms();
        ms_frame_delta = ms_frame_start - ms_frame_prev;
        ms_frame_prev = ms_frame_start;

        // Engine events
        if (tg_log_level.system) ta_log_write(&tg_debug_log, "System", " Handling events...\n");
        ta_event_events();

        if (tg_log_level.system) ta_log_write(&tg_debug_log, "System", " Accumulating...\n");
        if (sim_max_steps == 0) {
            // TODO: This *requires* vsync to work correctly!
            // If sim_max_steps == 0, assume we want lockstep physics
            ms_frame_accum = ms_sim_dt;
        } else {
            ms_frame_accum += ms_frame_delta;
            // Prevent spiral of death
            // NOTE: This breaks determinism when simulation is under duress
            if (ms_frame_accum > ms_sim_dt * sim_max_steps) {
                ta_log_write(&tg_debug_log, "System",
                    "WARNING: Physics accumulator spiraling; truncating %f to %f\n",
                    ms_frame_accum, ms_sim_dt * sim_max_steps);
                ms_frame_accum = ms_sim_dt * sim_max_steps;
            }
        }

        if (tg_log_level.system) ta_log_write(&tg_debug_log, "System", " Finding components...\n");
        ta_camera *active_camera = ta_scene_component(tg_game.scene,
            RES_COMP_CAMERA, tg_game.e_active_camera);
        ta_camera *player_cam = 0;
        ta_rigid_body *player_body = 0;

        if (ms_frame_accum >= ms_sim_dt) {
            if (tg_log_level.system) ta_log_write(&tg_debug_log, "System", " Finding sim components...\n");
            player_cam = ta_scene_component(tg_game.scene, RES_COMP_CAMERA,
                tg_game.e_player_one);
            player_body = ta_scene_component(tg_game.scene, RES_COMP_RIGID_BODY,
                tg_game.e_player_one);
        }

        while (ms_frame_accum >= ms_sim_dt) {
            if (tg_log_level.system) ta_log_write(&tg_debug_log, "System", " Sim step...\n");
            // Target player camera
            ta_camera_set_target_pos_absolute(player_cam,
                vec3_add(player_body->position, (ta_vec3) { 0.0f, 2.0f, 0.0f }));

            // Target minimap camera
            ta_vec3 minimap_camera_target_pos = active_camera->position;
            minimap_camera_target_pos.y += 50.0f;
            minimap_camera.focal_point = active_camera->position;
            ta_camera_set_target_pos_absolute(&minimap_camera,
                minimap_camera_target_pos);

            // Update cameras
            dlb_vec_each(ta_camera *, cam, tg_game.scene->resource_data[RES_COMP_CAMERA]) {
                ta_camera_update(cam, sim_dt);
            }

            if (tg_game.simulate) {
                if (tg_game.simulate > 0) {
                    tg_game.simulate--;
                }
#if 0
                ta_mat3 rotate_sun = mat3_rotate_z(1.0f);
                tg_game.sun->data.sun.direction =
                    mat3_mul_vec3(&rotate_sun, tg_game.sun->data.sun.direction);
#endif

#if 1
                // HACK: Make point light rotate in a circle
                static float light_deg = 0.0f;
                light_deg += 0.01f;
                if (light_deg >= 360.0f) light_deg = 0.0f;

                ta_light *lights = tg_game.scene->resource_data[RES_COMP_LIGHT];
                lights[1].position.x = cosf(light_deg) * 16.0f;
                lights[1].position.z = sinf(light_deg) * 16.0f;
#else
                // HACK: Make point light follow player camera
                lights[1]->position = tg_game.camera_player->position;
                // HACK: Make point light follow camera
                lights[1]->position = vec3_add(
                    tg_game.camera_freecam->position,
                    tg_game.camera_freecam->front
                );
#endif

                // Update scene
                ta_scene_update(tg_game.scene, (float)sim_dt);
                sim_step++;
            }

            // TODO: Put this somewhere intelligent
            // Update audio listener position
            ta_vec3 fwd_up[2];
            fwd_up[0] = active_camera->front;
            fwd_up[1] = active_camera->up;
            alListenerfv(AL_ORIENTATION, (float *)fwd_up);
            alListenerfv(AL_POSITION, (float *)&active_camera->position);
            //alListenerfv(AL_VELOCITY, (float *)&tg_game.camera->velocity);

            ms_sim_t += ms_sim_dt;
            ms_frame_accum -= ms_sim_dt;
        }

        float sim_alpha = (float)(ms_frame_accum / ms_sim_dt);

        // Draw models
        if (tg_log_level.system) ta_log_write(&tg_debug_log, "System", " Shadow pass...\n");
        ta_scene_shadow_pass(tg_game.scene, tg_shader_shadow, sim_alpha);
        if (tg_log_level.system) ta_log_write(&tg_debug_log, "System", " Render pass...\n");
        ta_scene_render(tg_game.scene, active_camera, sim_alpha);

        // World axes
        ta_primitive_push_axes(1.0f);
        ta_primitive_render(true, true);

        if (tg_game.state == TA_GAME_STATE_EDITOR) {
            if (tg_log_level.system) ta_log_write(&tg_debug_log, "System", " Editor pass...\n");
            ta_editor_draw(sim_alpha);
        }

        // Cursor
        glClear(GL_DEPTH_BUFFER_BIT);
        ta_primitive_push_crosshair(10, 2);

#if 0
        // Render minimap
        ta_rect map_rect = { 10, 50, 200, 200 };
        ta_viewport_bind(map_rect, TA_COLOR_GRAY7, true);
        ta_scene_render(tg_game.scene, &minimap_camera, sim_alpha);
        ta_viewport_unbind();
        ta_primitive_render(true, true);

        // Red dot on map
        int dot_radius = 2;
        ta_rect dot_rect = { 0 };
        dot_rect.x = map_rect.x + map_rect.w / 2 - dot_radius;
        dot_rect.y = map_rect.y + map_rect.h / 2 - dot_radius;
        dot_rect.w = dot_radius * 2;
        dot_rect.h = dot_radius * 2;
        ta_primitive_push_rect(dot_rect, TA_COLOR_RED, UI_LAYER_HUD);
        ta_primitive_render(true, true);
#endif

#if 0
        // TODO: Mesh selector, highlight and rotate mesh while mouse hover
        //ta_mat4 model = mat4_rotate_y(model_deg);
        //model_deg += 1.0f;
        //if (model_deg >= 360.0f) {
        //    model_deg = 0.0f;
        //}

        // Barchart
        ta_shader_set_mat4(tg_shader_lines, SYM_U_PROJ, &MAT4_IDENT);
        ta_shader_set_mat4(tg_shader_lines, SYM_U_VIEW, &MAT4_IDENT);
        ta_shader_set_mat4(tg_shader_lines, SYM_U_MODEL, &MAT4_IDENT);
        ta_ui_barchart_draw(0, 0, &chart);
        ta_primitive_render();
        ta_primitive_clear();
#endif

        //debug_nametag(active_cam);
        // TODO: Make HUD drawing suck less.. way too many draw calls
        //       Use texture atlas, batch everything into one draw call. Import
        //       textures from Rico; stop using stupid RGB placeholders
        if (tg_log_level.system) ta_log_write(&tg_debug_log, "System", " HUD pass...\n");
        ta_game_hud_draw(&tg_game);

        ms_frame_time = ta_timer_elapsed_ms() - ms_frame_start;
        frame_num++;
        if (tg_log_level.system) ta_log_write(&tg_debug_log, "System", " FPS pass...\n");
        render_frame_info(frame_num, ms_frame_time, ms_frame_delta, sim_step);

        // NOTE: This confirms rendering is being deferred until swap buffers,
        // but it's much slower (~5ms), so don't actually use it.
        //ta_log_write(&tg_debug_log, "System", " glFinish...\n");
        //glFinish();

        if (tg_log_level.system) ta_log_write(&tg_debug_log, "System", " Swap...\n");
        ta_window_swap(tg_window);

        tg_debug_log.flush = true;
        if (tg_log_level.system) ta_log_write(&tg_debug_log, "System",
            "Frame %llu displayed. time: %5.3f delta: %5.3f\n", frame_num,
            ms_frame_time, ms_frame_delta);
        tg_debug_log.flush = false;

        if (ms_frame_time > 16) {
            if (tg_log_level.system) ta_log_write(&tg_debug_log, "System", "!!!!!!!! LONG_FRAME !!!!!!!!\n");
            __debugbreak();
        }
    }

    // TODO: Free *EVERYTHING* (at least in debug mode.. to check memory leaks)
    ta_window_free(tg_window);
    if (tg_log_level.system) ta_log_write(&tg_debug_log, "System", "Goodbye.\n\n");
    return 0;
}

static void render_frame_info(u64 frame_num, double ms_frame_time,
    double ms_frame_delta, u64 sim_step)
{
    // Print frame time on the screen
    char frame_time_buf[256] = { 0 };
    int len = snprintf(CSTR(frame_time_buf),
        "Frame\n"
        "  count: %08llu\n"
        "  time : %5.2f ms\n"
        "  delta: %5.2f ms (v-sync: %s)\n"
        "Game\n"
        "  sim step: %08llu\n"
        "  state   : %s\n"
        "  prev    : %s\n"
        "Audio\n"
        "  master volume: %.2f",
        frame_num,
        ms_frame_time,
        ms_frame_delta,
        tg_game.vsync ? "On" : "Off",
        sim_step,
        game_state_str(tg_game.state),
        game_state_str(tg_game.state_prev),
        ta_audio_listener_get_volume(&tg_audio)
    );

    static ta_rect_uv *frame_time_rects = 0;
    ta_font_push_text(&frame_time_rects, tg_game.font,
        frame_time_buf, len, true, 0, 0, 0, 0);
    dlb_vec_each(ta_rect_uv *, rect, frame_time_rects) {
        ta_primitive_push_rect_uv(&quads_queue, *rect, TA_COLOR_WHITE,
            0, true, false);
    }
    dlb_vec_zero(frame_time_rects);

    ta_shader *font_shader = ta_scene_find_by_name(tg_game.scene, RES_SHADER,
        tg_game.font->shader);
    ta_shader_set_mat4(font_shader, SYM_U_PROJ, &MAT4_IDENT);
    ta_shader_set_mat4(font_shader, SYM_U_VIEW, &MAT4_IDENT);
    ta_shader_set_mat4(font_shader, SYM_U_MODEL, &MAT4_IDENT);
    ta_font_render(quads_queue, tg_game.font, SCREEN_WRAP_X(-300.0f), 0,
        UI_LAYER_HUD, true, true);
}

static void debug_nametag(ta_camera *camera)
{
    static ta_rect_uv *tag_rects = 0;
    ta_rectf tag_rect = ta_font_push_text(&tag_rects, tg_game.font,
        CSTR("Player 1\nis da best"), true, 0, 0, 0, 0);

    ta_position *player_pos = ta_scene_component(tg_game.scene,
        RES_COMP_POSITION, tg_game.e_player_one);

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
    ta_shader_set_sampler2d(tg_shader_quads, SYM_U_TEX, tg_game.tex_orange->gl_id);
    ta_rect_uv tag_background = { 0 };
    tag_background.rect.x -= NDC_W(5.0f);
    tag_background.rect.w = NDC_W(tag_rect.w) + NDC_W(10.0f);
    tag_background.rect.h = NDC_H(tag_rect.h); //tg_game.font->pixel_height * 1.5f;
    ta_primitive_push_rect_uv(&quads_queue, tag_background, TA_COLOR_GRAY3A,
        UI_LAYER_HUD_BG, false, false);
    ta_primitive_render_quads(quads_queue, tg_shader_quads, true, true);
    ta_shader_set_sampler2d(tg_shader_quads, SYM_U_TEX, 0);

    // Name tag text
    ta_shader *font_shader = ta_scene_find_by_name(tg_game.scene, RES_SHADER,
        tg_game.font->shader);
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
    ta_font_render(quads_queue, tg_game.font, 0, 0, 0, true, true);
}