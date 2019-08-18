#include "ta_timer.h"
#include "ta_log.h"
#include "ta_window.h"
#include "ta_audio.h"
#include "ta_render.h"
#include "ta_file.h"
#include "ta_scene.h"
#include "ta_shader.h"
#include "ta_ui_scrollview.h"
#include "ta_ui_barchart.h"
#include "ta_texture.h"
#include "ta_mesh.h"
#include "ta_camera.h"
#include "ta_viewport.h"
#include "ta_event.h"
#include "ta_game.h"
#include "ta_keyboard.h"
#include "ta_mouse.h"
#include "ta_node.h"
#include "ta_light.h"
#include "ta_schema.h"
#include "ta_parse.h"
#include "ta_symbol.h"
#include "ta_font.h"
#include "dlb/dlb_types.h"
#define DLB_VECTOR_IMPLEMENTATION
#include "dlb/dlb_vector.h"
#define DLB_HASH_IMPLEMENTATION
#define DLB_HASH_TEST
#include "dlb/dlb_hash.h"
#include "misc/gl3w.h"
#include "SDL/SDL.h"

DLB_ASSERT_HANDLER(handle_assert)
{
    if (tg_debug_log) {
        ta_log_write(tg_debug_log,
            "\n---[DLB_ASSERT_HANDLER]-----------------\n"
            "Source file: %s:%d\n\n"
            "%s\n"
            "----------------------------------------\n",
            filename, line, expr
        );
    }
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
    ta_window_msgbox(SDL_MESSAGEBOX_ERROR, "ASSERT", buf);
#endif
    exit(-1);
}
dlb_assert_handler_def *dlb_assert_handler = handle_assert;

void debug_tests() {
#if _DEBUG
    parse_tests();
    dlb_hash_test();
    ta_math_test();
#endif
}

void ndc_tests() {
    DLB_ASSERT(SCREEN_WRAP_X(0) == 0);
    DLB_ASSERT(SCREEN_WRAP_X(1) == 1);
    DLB_ASSERT(SCREEN_WRAP_X(tg_window.rect.w) == tg_window.rect.w);
    DLB_ASSERT(SCREEN_WRAP_X(-1) == tg_window.rect.w - 1);

    DLB_ASSERT(SCREEN_WRAP_Y(0) == 0);
    DLB_ASSERT(SCREEN_WRAP_Y(1) == 1);
    DLB_ASSERT(SCREEN_WRAP_Y(tg_window.rect.h) == tg_window.rect.h);
    DLB_ASSERT(SCREEN_WRAP_Y(-1) == tg_window.rect.h - 1);
}

// Random thoughts
// https://en.wikipedia.org/wiki/Accumulator_(energy)

int main(int argc, char *argv[])
{
    UNUSED(argc);
    UNUSED(argv);
    ta_timer_init();
    srand((u32)ta_timer_only_ms());  // TODO: Better seed if it matters

    ta_log debug_log = { 0 };
    ta_log_init(&debug_log, "log.txt", true);
    tg_debug_log = &debug_log;
    debug_tests();

    ta_symbol_init();
    ta_schema_register();

    // TODO: Save size/position to a config file
    ta_window_init(1600, 900, false);
    ndc_tests();
    // TODO: Make sure this gets freed or handled better
    tg_game.audio = dlb_calloc(1, sizeof(ta_audio_listener));
    ta_audio_listener_init(tg_game.audio);
    //ta_audio_listener_mute(tg_game.audio);
    ta_audio_listener_set_volume(tg_game.audio, 0.5f);
    ta_mouse_init();
    ta_keyboard_init();
    ta_render_init();
    ta_primitive_init();
    ta_game_init();

    // Intro scene
    tg_game.scene = ta_scene_load_file("data/scene/scene1.dml");
    // TODO: Find closest 8 lights and store them in tg_game.lights
    tg_game.lights = tg_game.scene->pools[TYP_LIGHT];
    tg_game.camera_player =
        ta_scene_find(tg_game.scene, TYP_CAMERA, INTERN("cam_player"));
    tg_game.camera_freecam =
        ta_scene_find(tg_game.scene, TYP_CAMERA, INTERN("cam_freecam"));
    tg_game.player =
        ta_scene_find(tg_game.scene, TYP_NODE, INTERN("node_player"));
    tg_game.player_ammo_max = 20;
    tg_game.player_ammo = tg_game.player_ammo_max;
    tg_game.player_clip_max = 8;
    tg_game.player_clip = MIN(tg_game.player_clip_max, tg_game.player_ammo);
    ta_game_state_set(TA_GAME_STATE_FREE_CAM);

    // Ensure we have a valid camera, player and light
    DLB_ASSERT(tg_game.camera);
    DLB_ASSERT(tg_game.player);
    DLB_ASSERT(tg_game.lights);

    ta_audio_source *bg_music = ta_scene_find(tg_game.scene, TYP_AUDIO_SOURCE,
        INTERN("src_background_music"));
    DLB_ASSERT(bg_music);
    tg_game.background_music = bg_music;
    //ta_audio_source_play_loop(tg_game.background_music);

    tg_game.font = ta_scene_find(tg_game.scene, TYP_FONT, INTERN("font_default"));
    DLB_ASSERT(tg_game.font);
    ta_shader *font_shader = ta_font_shader(tg_game.font);
    ta_shader_set_mat4(font_shader, SYM_U_PROJ, &MAT4_IDENT);
    ta_shader_set_mat4(font_shader, SYM_U_VIEW, &MAT4_IDENT);
    ta_shader_set_mat4(font_shader, SYM_U_MODEL, &MAT4_IDENT);

    tg_game.tex_orange = ta_scene_find(tg_game.scene, TYP_TEXTURE, INTERN("tex_test_diff"));
    DLB_ASSERT(tg_game.tex_orange && tg_game.tex_orange->gl_id);
    tg_game.tex_red = ta_scene_find(tg_game.scene, TYP_TEXTURE, INTERN("tex_test_mrao"));
    DLB_ASSERT(tg_game.tex_red && tg_game.tex_red->gl_id);
    tg_game.tex_audio_icon = ta_scene_find(tg_game.scene, TYP_TEXTURE, INTERN("tex_audio_icon"));
    DLB_ASSERT(tg_game.tex_audio_icon && tg_game.tex_audio_icon->gl_id);

    ////////////////////////////////////////////////////////////////////////////
    // Shaders
    ////////////////////////////////////////////////////////////////////////////
    tg_shader_lines =
        ta_scene_find(tg_game.scene, TYP_SHADER, INTERN("shader_lines"));
    DLB_ASSERT(tg_shader_lines);

    tg_shader_quads =
        ta_scene_find(tg_game.scene, TYP_SHADER, INTERN("shader_quads"));
    DLB_ASSERT(tg_shader_quads);

    tg_shader_cubemap =
        ta_scene_find(tg_game.scene, TYP_SHADER, INTERN("shader_cubemap"));
    DLB_ASSERT(tg_shader_cubemap);

    tg_shader_shadow =
        ta_scene_find(tg_game.scene, TYP_SHADER, INTERN("shader_shadow"));
    DLB_ASSERT(tg_shader_shadow);

    ////////////////////////////////////////////////////////////////////////////
    // UI
    ////////////////////////////////////////////////////////////////////////////
    // TODO: Move this to DML (e.g. editor.dml)
    ta_camera minimap_camera = { 0 };
    minimap_camera.fov = 90.0f;
    minimap_camera.up = VEC3_NZ;
    minimap_camera.ortho = true;
    ta_camera_init(&minimap_camera);

    ta_ui_barchart chart = ta_ui_barchart_init(10, 10, tg_window.rect.w - 20, 30);
    UNUSED(chart);

    //ta_shader_set_sampler2d(tg_shader_mesh, SYM_U_TEX0, tex_test->gl_id);

    ////////////////////////////////////////////////////////////////////////////
    // Main loop
    ////////////////////////////////////////////////////////////////////////////
    u64 frame_num = 0;

    // Eric Catto - Soft Constraints (GDC 2011)
    // Semi-implicit Euler will eventually blow up if you take big time steps. A
    // general rule is to take at least 4 time steps per period of oscillation.
    // For example, if the oscillation frequency is 60Hz, then you shouldn’t
    // take time steps slower than 15Hz.
    //
    // Randy Gaul
    // https://gamedevelopment.tutsplus.com/series/how-to-create-a-custom-physics-engine--gamedev-12715
    const double ms_sim_dt = 10;             // fixed dt milliseconds
    const double sim_dt = ms_sim_dt / 1000;  // fixed dt seconds
    const double sim_max_steps = 10;         // max simulation steps per frame
    double ms_sim_t = 0;                     // current simulation time

    double ms_frame_first = ta_timer_elapsed_ms();
    double ms_frame_prev = ms_frame_first;
    double ms_frame_accum = 0;

    float light_deg = 0;

    while (tg_game.state != TA_GAME_STATE_QUIT) {
        double ms_frame_start = ta_timer_elapsed_ms();
        double ms_frame_delta = ms_frame_start - ms_frame_prev;
        ms_frame_prev = ms_frame_start;

        // Engine events
        ta_mouse_events();
        ta_keyboard_events();
        ta_event_events();
        ta_window_events();
        // Game events
        ta_game_events();
        ta_camera_events();

        ms_frame_accum += ms_frame_delta;

        // Prevent spiral of death
        // NOTE: This breaks determinism when simulation is under duress
        if (ms_frame_accum > ms_sim_dt * sim_max_steps) {
            ta_log_write(tg_debug_log,
                "[Sim] WARNING: Physics accumulator spiraling; truncating %f to %f\n",
                ms_frame_accum, ms_sim_dt * sim_max_steps);
            ms_frame_accum = ms_sim_dt * sim_max_steps;
        }

        while (ms_frame_accum >= ms_sim_dt) {
            // Update player camera
            // TODO: Set target entity and follow distance vector in DML
            ta_rigid_body *player_body = ta_node_rigid_body(tg_game.player);
            ta_camera_set_target_pos_absolute(tg_game.camera_player,
                vec3_add(player_body->position, (ta_vec3) { 0.0f, 2.0f, 0.0f }));
            ta_camera_update(tg_game.camera_player, sim_dt);

            light_deg += 0.005f;
            if (light_deg >= 360.0f) light_deg = 0.0f;

            tg_game.lights[1].position.x = cosf(light_deg) * 4.0f;
            tg_game.lights[1].position.z = sinf(light_deg) * 4.0f;

            // HACK: Make point light follow player camera
            //tg_game.lights[1]->position = tg_game.camera_player->position;
            // HACK: Make point light follow camera
            //tg_game.lights[1]->position = vec3_add(
            //    tg_game.camera_freecam->position,
            //    tg_game.camera_freecam->front
            //);

            // Update main camera
            ta_camera_update(tg_game.camera_freecam, sim_dt);

            // Update minimap camera
            ta_vec3 minimap_camera_target_pos = tg_game.camera->position;
            minimap_camera_target_pos.y += 50.0f;
            minimap_camera.focal_point = tg_game.camera->position;
            ta_camera_set_target_pos_absolute(&minimap_camera,
                minimap_camera_target_pos);
            ta_camera_update(&minimap_camera, sim_dt);

            // Update player
            //ta_rigid_body *player_body = ta_node_rigid_body(tg_game.player);
            //player_body->transform.position = tg_game.camera->position;

            // Update scene
            ta_scene_update(tg_game.scene, (float)sim_dt);

            // TODO: Put this somewhere intelligent
            // Update audio listener position
            ta_vec3 fwd_up[2];
            fwd_up[0] = tg_game.camera->front;
            fwd_up[1] = tg_game.camera->up;
            alListenerfv(AL_ORIENTATION, (float *)fwd_up);
            alListenerfv(AL_POSITION, (float *)&tg_game.camera->position);
            //alListenerfv(AL_VELOCITY, (float *)&tg_game.camera->velocity);

            ms_sim_t += ms_sim_dt;
            ms_frame_accum -= ms_sim_dt;
        }

        float sim_alpha = (float)(ms_frame_accum / ms_sim_dt);

        //ta_mat3 rotate_sun = mat3_rotate_z(1.0f);
        //tg_game.sun->data.sun.direction =
        //    mat3_mul_vec3(&rotate_sun, tg_game.sun->data.sun.direction);


        // Draw models
        ta_scene_shadow_pass(tg_game.scene, tg_shader_shadow, sim_alpha);
        ta_scene_render(tg_game.scene, tg_game.camera, sim_alpha);

        // World axes
        ta_primitive_push_axes(1.0f);
        ta_primitive_render(true, true);
        glClear(GL_DEPTH_BUFFER_BIT);

        ta_rect test1 = { 0 };
        test1.x = 5;
        test1.y = 5;
        test1.w = 10;
        test1.h = 10;
        ta_primitive_push_rect(test1, TA_COLOR_RED, UI_LAYER_HUD);

        // Cursor
        ta_primitive_push_crosshair(10, 2);

#if 0
        ta_light_render_shadowmap_debug(&tg_game.lights[1]);
#endif
#if 0
        // Minimap
        ta_viewport minimap_viewport = ta_viewport_init(TA_SIZE(200, 200),
            (ta_rgba) { 0.1f, 0.1f, 0.2f, 1.0f });
        ta_viewport_bind(&minimap_viewport, TA_POSITION(10, 50), true);
        {
            // TODO: Mesh selector, highlight and rotate mesh while mouse hover
            //ta_mat4 model = mat4_rotate_y(model_deg);
            //model_deg += 1.0f;
            //if (model_deg >= 360.0f) {
            //    model_deg = 0.0f;
            //}

            // Draw models
            ta_scene_render(tg_game.scene, &minimap_camera, sim_alpha);

            ta_shader_set_mat4(tg_shader_quads, SYM_U_PROJ, &MAT4_IDENT);
            ta_shader_set_mat4(tg_shader_quads, SYM_U_VIEW, &MAT4_IDENT);
            ta_shader_set_mat4(tg_shader_quads, SYM_U_MODEL, &MAT4_IDENT);

            // Red dot on map
            ta_rect parent = minimap_viewport.rect;
            parent.x = minimap_viewport.rect.w / 2 - 2;
            parent.y = minimap_viewport.rect.h / 2 - 2;
            ta_primitive_push_rect(parent, (ta_rect) { 0, 0, 4, 4 },
                TA_COLOR_RED);

            ta_primitive_render();
            ta_primitive_clear();
#elif 0
            ta_shader_set_sampler2d(tg_shader_quads, SYM_U_TEX, tex_test->gl_id);
            ta_rect parent = { 0 };
            parent.w = tex_test->width;
            parent.h = tex_test->height;
            ta_rect child = { 0 };
            child.w = tex_test->width;
            child.h = tex_test->height;

            ta_primitive_push_rect(parent, child, TA_COLOR_INVIS);
            ta_primitive_render();
            ta_shader_set_sampler2d(tg_shader_quads, SYM_U_TEX, 0);
            ta_primitive_clear();
        }
        ta_viewport_unbind(&minimap_viewport);
#endif

#if 0
        // Barchart
        ta_shader_set_mat4(tg_shader_lines, SYM_U_PROJ, &MAT4_IDENT);
        ta_shader_set_mat4(tg_shader_lines, SYM_U_VIEW, &MAT4_IDENT);
        ta_shader_set_mat4(tg_shader_lines, SYM_U_MODEL, &MAT4_IDENT);
        ta_ui_barchart_draw(0, 0, &chart);
        ta_primitive_render();
        ta_primitive_clear();
#endif

#if 1
        if (!tg_mouse.captured) {
            ta_ui_test();
        }
        if (tg_mouse.captured) {
            ta_ui_hud();
        }
#endif

#if 1
        // TODO: Move "name tag" logic somewhere else
        {
            ta_vec3 tag_pos = vec3_add(tg_game.player->transform.position, (ta_vec3){ 0.0f, 1.2f, 0.0f });
            ta_vec3 tag_to_cam = vec3_sub(tg_game.camera->position, tag_pos);
            tag_to_cam.z *= -1.0f;
            tag_to_cam.y *= 0.0f;
            float tag_scalef = MAX(vec3_len(tag_to_cam) * NDC_W(1.2), NDC_W(8));

            ta_rectf tag_rect = ta_font_push_text(&tooltip_fg_queue, tg_game.font, 0.0f, 0.0f, UI_LAYER_HUD, CSTR("Player 1"), false, 0, 0);
            ta_vec3 tag_offset = tg_game.camera->right;
            // HACK: Why 4.0? Dunno proper way to center this.
            tag_offset = vec3_scalef(tag_offset, NDC_W(tag_rect.w) * 4.0f);
            tag_pos = vec3_sub(tag_pos, tag_offset);

            ta_mat4 tag_scale = mat4_scalef(tag_scalef);
            ta_mat4 tag_rot = mat4_lookat(VEC3_ZERO, tag_to_cam, VEC3_Y);
            ta_mat4 tag_trans = mat4_translate(tag_pos);
            ta_mat4 tag_xform = tag_scale;
            tag_xform = mat4_mul(&tag_rot, &tag_xform);
            tag_xform = mat4_mul(&tag_trans, &tag_xform);

            // Name tag background
            ta_shader_set_mat4(tg_shader_quads, SYM_U_PROJ, &tg_game.camera->projection);
            ta_shader_set_mat4(tg_shader_quads, SYM_U_VIEW, &tg_game.camera->look_at);
            ta_shader_set_mat4(tg_shader_quads, SYM_U_MODEL, &tag_xform);
            ta_shader_set_sampler2d(tg_shader_quads, SYM_U_TEX, tg_game.tex_orange->gl_id);
            ta_rect_uv tag_background = { 0 };
            tag_background.rect.x = -4.0f;
            tag_background.rect.w = tag_rect.w + 8.0f;
            tag_background.rect.h = tag_rect.h; //tg_game.font->pixel_height * 1.5f;
            ta_primitive_push_rect_uv(&quads_queue, tag_background, TA_COLOR_GRAY3A, UI_LAYER_HUD_BG, false);
            ta_primitive_render_quads(quads_queue, tg_shader_quads, true, true);
            ta_shader_set_sampler2d(tg_shader_quads, SYM_U_TEX, 0);

            // Name tag text
            ta_shader_set_mat4(font_shader, SYM_U_PROJ, &tg_game.camera->projection);
            ta_shader_set_mat4(font_shader, SYM_U_VIEW, &tg_game.camera->look_at);
            ta_shader_set_mat4(font_shader, SYM_U_MODEL, &tag_xform);
            ta_font_render(tooltip_fg_queue, tg_game.font, 0, true, true);
        }

        // Print frame time on the screen
        double ms_frame_time = ta_timer_elapsed_ms() - ms_frame_start;
        char frame_time_buf[40] = { 0 };
        int len = snprintf(CSTR(frame_time_buf), "Frame %8llu\n%.2f ms", frame_num, ms_frame_time);
        ta_font_push_text(&tooltip_fg_queue, tg_game.font, -130.0f, 0.0f, UI_LAYER_HUD, CSTR(frame_time_buf), true, 0, 0);
        ta_font_render(tooltip_fg_queue, tg_game.font, 0, true, true);
#endif

        ta_window_swap();
        //ta_log_write(tg_debug_log, "Frame %llu started at %f sim time: %f\n", frame_num, ms_frame_start - ms_frame_first, ms_sim_t);
        frame_num++;
    }

    ta_window_free();
    ta_log_write(tg_debug_log, "Goodbye.\n\n");
    return 0;
}