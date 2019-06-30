#include "ta_timer.h"
#include "ta_log.h"
#include "ta_window.h"
#include "ta_audio.h"
#include "ta_render.h"
#include "ta_file.h"
#include "ta_scene.h"
#include "ta_shader.h"
//#include "ta_shader_lines.h"
//#include "ta_shader_quads.h"
//#include "ta_shader_mesh.h"
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
#include "ta_entity.h"
#include "ta_light.h"
#include "ta_schema.h"
#include "ta_parse.h"
#include "ta_symbol.h"
#include "dlb_types.h"
#define DLB_VECTOR_IMPLEMENTATION
#include "dlb_vector.h"
#define DLB_HASH_IMPLEMENTATION
#define DLB_HASH_TEST
#include "dlb_hash.h"
#include "misc/gl3w.h"
#include "SDL/SDL.h"

DLB_ASSERT_HANDLER(handle_assert)
{
    ta_log_write(tg_debug_log,
        "\n---[DLB_ASSERT_HANDLER]---------------------------------------------------------\n"
        "Source file: %s:%d\n\n"
        "%s\n"
        "--------------------------------------------------------------------------------\n",
		filename, line, expr);
    UNUSED(getchar());
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

ta_entity *entity_create(ta_scene *scn, const char *name) {
    ta_entity *e = ta_scene_obj_alloc(scn, F_TA_ENTITY, INTERN(name));
    e->type = 0;
    e->transform.position.x = 1.1f;
    e->transform.position.y = 1.2f;
    e->transform.position.z = 1.3f;
    e->transform.orientation.x = 2.1f;
    e->transform.orientation.y = 2.2f;
    e->transform.orientation.z = 2.3f;
    e->transform.orientation.w = 2.4f;
    e->transform.scale.x = 3.1f;
    e->transform.scale.y = 3.2f;
    e->transform.scale.z = 3.3f;
    return e;
}

void write_scene(const char *filename) {
    tg_game.scene = ta_scene_init("test scene");
    entity_create(tg_game.scene, "Timmy");
    entity_create(tg_game.scene, "Bobby");

    printf("[WRITE: %s]\n", filename);
    ta_scene_print(tg_game.scene, stdout);
    printf("\n");

    ta_file *data_file = ta_file_open(filename, FILE_WRITE);
    ta_scene_print(tg_game.scene, data_file->hnd);
    ta_file_close(data_file);
    ta_scene_free(tg_game.scene);
}

void read_scene(const char *filename) {
    ta_log_write(tg_debug_log, "[Scene] Loading %s\n", filename);
    ta_file *data_file = ta_file_open(filename, FILE_READ);
    tg_game.scene = ta_scene_load(data_file);
    ta_file_close(data_file);

    ta_log_write(tg_debug_log, "[Scene] Initializing objects\n", filename);
    ta_scene_initialize_objects(tg_game.scene);

    ta_log_write(tg_debug_log, "[Scene] Loaded successfully\n");
    //ta_scene_print(tg_game.scene, tg_debug_log->stream);
    //ta_scene_free(tg_game.scene);
}

int main(int argc, char *argv[])
{
    UNUSED(argc);
    UNUSED(argv);
    ta_timer_init();
	srand((u32)ta_timer_only_ms());  // TODO: Better seed if it matters

    ta_log debug_log;
	tg_debug_log = &debug_log;
    ta_log_init(tg_debug_log, "log.txt", true);
    debug_tests();

    ta_symbol_init();
    ta_schema_register();

    ta_window_init(1600, 900, false);
    // TODO: Make sure this gets freed or handled better
    tg_game.audio = dlb_calloc(1, sizeof(ta_audio_listener));
    ta_audio_listener_init(tg_game.audio);
    ta_mouse_init();
    ta_keyboard_init();
    ta_render_init();
    ta_primitive_init();
    ta_game_init();

    // Intro scene
    read_scene("data/scene/scene1.dml");
    dlb_vec_push(tg_game.lights, ta_scene_find(tg_game.scene, F_TA_LIGHT,
        INTERN("light_sun")));
    dlb_vec_push(tg_game.lights, ta_scene_find(tg_game.scene, F_TA_LIGHT,
        INTERN("light_point_1")));
    tg_game.camera_player = ta_scene_find(tg_game.scene, F_TA_CAMERA,
        INTERN("camera_player"));
    tg_game.camera_freecam = ta_scene_find(tg_game.scene, F_TA_CAMERA,
        INTERN("camera_freecam"));
    tg_game.player = ta_scene_find(tg_game.scene, F_TA_ENTITY,
        INTERN("entity_player"));
    ta_game_state_set(TA_STATE_FREE_CAM);

    // Ensure we have a valid camera, player and light
    DLB_ASSERT(tg_game.camera);
    DLB_ASSERT(tg_game.player);
    DLB_ASSERT(tg_game.lights && tg_game.lights[0]);

    ta_audio_source *ambient_wakeup = ta_scene_find(tg_game.scene,
        F_TA_AUDIO_SOURCE, INTERN("src_ambient_genesis"));
    DLB_ASSERT(ambient_wakeup);
    ta_audio_source_play_loop(ambient_wakeup);

    ////////////////////////////////////////////////////////////////////////////
    // Shaders
    ////////////////////////////////////////////////////////////////////////////
    tg_shader_lines =
        ta_scene_find(tg_game.scene, F_TA_SHADER, INTERN("shader_lines"));
    DLB_ASSERT(tg_shader_lines);

    tg_shader_quads =
        ta_scene_find(tg_game.scene, F_TA_SHADER, INTERN("shader_quads"));
    DLB_ASSERT(tg_shader_quads);

    tg_shader_shadow =
        ta_scene_find(tg_game.scene, F_TA_SHADER, INTERN("shader_shadow"));
    DLB_ASSERT(tg_shader_shadow);

    ////////////////////////////////////////////////////////////////////////////
    // UI
    ////////////////////////////////////////////////////////////////////////////
    // TODO: Move this to DML (e.g. editor.dml)
    ta_camera minimap_camera = { 0 };
    minimap_camera.mode = TA_CAMERA_ORBIT;
    minimap_camera.fov = 90.0f;
    minimap_camera.up = VEC3_NZ;
    minimap_camera.ortho = true;
    ta_camera_init(&minimap_camera);
	ta_viewport minimap_viewport = ta_viewport_init(10, 50, 200, 200,
        (ta_rgba) { 0.1f, 0.1f, 0.2f, 1.0f }, &minimap_camera);

    ta_texture *tex_test =
        ta_scene_find(tg_game.scene, F_TA_TEXTURE, INTERN("TEXTURE_ALBEDO"));
    DLB_ASSERT(tex_test && tex_test->gl_id && "Could not find TEXTURE_ALBEDO");

	// TODO: Remove x,y coords from init() methods and only store size. Pass x,y
	//       at render time (make sure to update viewport correctly).
	ta_ui_image *ui_image = ta_ui_image_init(0, 0, 0, 0, tex_test);
	ta_ui_scrollview *view = ta_ui_scrollview_init(420, 50, 800, 300,
		(ta_ui_base *)ui_image);

	ta_ui_barchart chart = ta_ui_barchart_init(10, 10, tg_window.width - 20, 30);
    UNUSED(view);
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

    ta_audio_listener_mute(tg_game.audio);

    while (tg_game.state != TA_STATE_QUIT) {
        double ms_frame_start = ta_timer_elapsed_ms();
        double ms_frame_delta = ms_frame_start - ms_frame_prev;
        ms_frame_prev = ms_frame_start;

        ta_event_sdl_poll();
        ta_mouse_update();  // TODO: Rename these "ta_mouse_events" or similar
        ta_keyboard_update();
        ta_event_update();
        ta_game_update();
        ta_camera_events(tg_game.camera);

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
            ta_rigid_body *player_body = ta_entity_rigid_body(tg_game.player);
            ta_camera_set_target_pos_absolute(tg_game.camera_player,
                vec3_add(player_body->position, (ta_vec3) { 0.0f, 2.0f, 0.0f }));
            ta_camera_update(tg_game.camera_player, sim_dt);

            // HACK: Make point light follow player camera
            //tg_game.lights[1]->position = tg_game.camera_player->position;
            // HACK: Make point light follow camera
            //tg_game.lights[1]->position = vec3_add(tg_game.camera_freecam->position, tg_game.camera_freecam->front);

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
            //ta_rigid_body *player_body = ta_entity_rigid_body(tg_game.player);
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

        ta_primitive_render();
        ta_primitive_clear();

        // World axes
        ta_shader_set_mat4(tg_shader_lines, SYM_U_PROJ, &tg_game.camera->projection);
        ta_shader_set_mat4(tg_shader_lines, SYM_U_VIEW, &tg_game.camera->look_at);
        ta_shader_set_mat4(tg_shader_lines, SYM_U_MODEL, &MAT4_IDENT);
        ta_primitive_push_axes(1.0f);
        ta_primitive_render();
        ta_primitive_clear();

        ta_shader_set_mat4(tg_shader_quads, SYM_U_PROJ, &MAT4_IDENT);
        ta_shader_set_mat4(tg_shader_quads, SYM_U_VIEW, &MAT4_IDENT);
        ta_shader_set_mat4(tg_shader_quads, SYM_U_MODEL, &MAT4_IDENT);

        // Minimap
		ta_viewport_bind(&minimap_viewport, true);
		{
#if 0
			// TODO: Mesh selector, highlight and rotate mesh while mouse hover
			//ta_mat4 model = mat4_rotate_y(model_deg);
			//model_deg += 1.0f;
			//if (model_deg >= 360.0f) {
			//	model_deg = 0.0f;
			//}

			// Draw models
            ta_scene_render(tg_game.scene, minimap_viewport.camera, sim_alpha);

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
#elif 0
            ta_shader_set_sampler2d(tg_shader_quads, SYM_U_TEX, tg_game.lights[0]->shadowmap.texture);
            ta_rect parent = { 0 };
            parent.w = tg_game.lights[0]->shadowmap.resolution;
            parent.h = tg_game.lights[0]->shadowmap.resolution;
            ta_rect child = { 0 };
            child.w = tg_game.lights[0]->shadowmap.resolution;
            child.h = tg_game.lights[0]->shadowmap.resolution;

            ta_primitive_push_rect(parent, child, TA_COLOR_INVIS);
            ta_primitive_render();
            ta_shader_set_sampler2d(tg_shader_quads, SYM_U_TEX, 0);
            ta_primitive_clear();
#endif
		}
		ta_viewport_unbind(&minimap_viewport);
		glEnable(GL_CULL_FACE);

#if 0
		// Barchart
        ta_shader_set_mat4(tg_shader_lines, SYM_U_PROJ, &MAT4_IDENT);
        ta_shader_set_mat4(tg_shader_lines, SYM_U_VIEW, &MAT4_IDENT);
        ta_shader_set_mat4(tg_shader_lines, SYM_U_MODEL, &MAT4_IDENT);
        ta_ui_barchart_draw(0, 0, &chart);
        ta_primitive_render();
        ta_primitive_clear();
#endif

#if 0
        // Scroll view
		ta_ui_scrollview_draw(0, 0, view);
        ta_primitive_render();
        ta_primitive_clear();
#endif

        ta_ui_clear();

        // TODO: Print frame time on the screen once we have text rendering
        //double ms_frame_time = ta_timer_elapsed_ms() - ms_frame_start;
        //printf("Frame %5llu took %f ms.\n", frame_num, ms_frame_time);

        ta_window_swap();
        //ta_log_write(tg_debug_log, "Frame %llu started at %f sim time: %f\n", frame_num, ms_frame_start - ms_frame_first, ms_sim_t);
        frame_num++;
    }

    ta_window_free();
    ta_log_write(tg_debug_log, "Goodbye.\n\n");
    return 0;
}

// Random thoughts
// https://en.wikipedia.org/wiki/Accumulator_(energy)