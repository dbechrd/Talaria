#include "ta_timer.h"
#include "ta_log.h"
#include "ta_window.h"
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

#if _DEBUG
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
#endif

void debug_tests() {
#if _DEBUG
    parse_tests();
    dlb_hash_test();
#endif
}

ta_entity *entity_create(ta_scene *scn, const char *name) {
    ta_entity *e = ta_scene_obj_alloc(scn, F_TA_ENTITY);
    e->type = 0;
    e->uid = name;
    e->transform.position.x = 1.1f;
    e->transform.position.y = 1.2f;
    e->transform.position.z = 1.3f;
    e->transform.rotation.x = 2.1f;
    e->transform.rotation.y = 2.2f;
    e->transform.rotation.z = 2.3f;
    e->transform.rotation.w = 2.4f;
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
    ta_scene_obj_init(tg_game.scene);

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
    debug_tests();

    ta_log debug_log;
	tg_debug_log = &debug_log;
    ta_log_init(tg_debug_log, "log.txt", true);

    ta_symbol_init();
    ta_schema_register();

    ta_window_init(1600, 900, false);
    ta_mouse_init();
    ta_keyboard_init();
    ta_render_init();
    ta_primitive_init();

    tg_game.state = TA_STATE_INIT;

    // Intro scene
    read_scene("data/scene/scene1.dml");
    tg_game.sun = ta_scene_find(tg_game.scene, F_TA_LIGHT, INTERN("light_sun"));
    tg_game.camera = ta_scene_find(tg_game.scene, F_TA_CAMERA, INTERN("camera_player"));
    tg_game.player = ta_scene_find(tg_game.scene, F_TA_ENTITY, INTERN("entity_player"));
    DLB_ASSERT(tg_game.sun);     // Ensure we have a valid sun light
    DLB_ASSERT(tg_game.camera);  // Ensure we have a valid camera
    DLB_ASSERT(tg_game.player);  // Ensure we have a valid player

    /*
    tg_window.fov = fov;
    tg_window.nearz = nearz;
    tg_window.projection = mat4_perspective_inf(
        tg_window.fov,
        (float)tg_window.width / tg_window.height,
        tg_window.nearz
    );

    float oo = 0.5f;
    view.projection = mat4_ortho(-oo, oo, -oo, oo, 0.1f, 10.0f);

    //view.projection = mat4_ortho(
    //    (float)view.rect.x,
    //    (float)view.rect.x + view.rect.w,
    //    (float)view.rect.y + view.rect.h,
    //    (float)view.rect.y,
    //    nearz,
    //    100.0f
    //);

    //view.projection = mat4_perspective_inf(
    //    fov,
    //    (float)view.rect.w / view.rect.h,
    //    nearz
    //);
    */

    ////////////////////////////////////////////////////////////////////////////
    // Shaders
    ////////////////////////////////////////////////////////////////////////////
    tg_shader_lines = ta_scene_find(tg_game.scene, F_TA_SHADER,
        INTERN("shader_lines"));
    DLB_ASSERT(tg_shader_lines && "Could not find shader_lines");

    tg_shader_quads = ta_scene_find(tg_game.scene, F_TA_SHADER,
        INTERN("shader_quads"));
    DLB_ASSERT(tg_shader_quads && "Could not find shader_quads");

    ////////////////////////////////////////////////////////////////////////////
    // UI
    ////////////////////////////////////////////////////////////////////////////
    // TODO: Move this to DML (e.g. editor.dml)
    ta_camera minimap_camera = { 0 };
    minimap_camera.mode = TA_CAMERA_ORBIT;
    minimap_camera.fov = 45.0f;
    minimap_camera.up = VEC3_NZ;
    ta_camera_init(&minimap_camera);
	ta_viewport minimap_viewport = ta_viewport_init(10, 50, 200, 200,
        (ta_rgba) { 0.1f, 0.1f, 0.2f, 1.0f }, &minimap_camera);

	//ta_mat4 project = mat4_perspective(65.0f, aspect, 0.1f, 100.0f);
	//float oo = 0.5f;
	//ta_mat4 project = mat4_ortho(-oo, oo, -oo, oo, 0.1f, 10.0f);

	ta_mat4 look_at_map = mat4_lookat(
		(ta_vec3) { 0.0f, 40.0f, 50.0f },
		(ta_vec3) { 0.0f, 0.0f, 0.0f },
		VEC3_Y
	);

    ta_texture *tex_test = ta_scene_find(tg_game.scene, F_TA_TEXTURE,
        INTERN("texture_1"));
    DLB_ASSERT(tex_test && tex_test->gl_id && "Could not find texture_1");

	// TODO: Remove x,y coords from init() methods and only store size. Pass x,y
	//       at render time (make sure to update viewport correctly).
	ta_ui_image *ui_image = ta_ui_image_init(0, 0, 0, 0, tex_test);
	ta_ui_scrollview *view = ta_ui_scrollview_init(420, 50, 800, 300,
		(ta_ui_base *)ui_image);

	ta_ui_barchart chart = ta_ui_barchart_init(10, 10, tg_window.width - 20, 30);

    //ta_shader_set_sampler2d(tg_shader_mesh, SYM_U_TEX0, tex_test->gl_id);

    // TODO: What other startup states would be useful (e.g. LOADING_MESHES)?
    //       Could use this for a progress bar during load and better logging.
    //       Maybe also have JUMPING, CLIMBING, etc.? Could use bit flags to
    //       capture overall state as well (e.g. PLAYING, EDITING, etc.)
    tg_game.state = TA_STATE_FREE_CAM;

    ////////////////////////////////////////////////////////////////////////////
    // Main loop
    ////////////////////////////////////////////////////////////////////////////
    u64 frame_num = 0;

    double ms_sim_t = 0;                     // current simulation time
    const double ms_sim_dt = 10;             // fixed dt milliseconds
    const double sim_dt = ms_sim_dt / 1000;  // fixed dt seconds

    double ms_frame_prev = ta_timer_elapsed_ms();
    double ms_frame_accum = 0;

    while (tg_game.state != TA_STATE_QUIT) {
        double ms_frame_start = ta_timer_elapsed_ms();
        double ms_frame_delta = ms_frame_start - ms_frame_prev;
        ms_frame_prev = ms_frame_start;

        ta_event_sdl_poll();
        ta_mouse_update();  // TODO: Rename these "ta_mouse_events" or similar
        ta_keyboard_update();
        ta_event_update();
        ta_game_update();

        ms_frame_accum += ms_frame_delta;
        while (ms_frame_accum >= ms_sim_dt) {
            // Update main camera
            ta_camera_update(tg_game.camera, sim_dt);

            // Update minimap camera
            ta_vec3 minimap_camera_target_pos = tg_game.camera->position;
            minimap_camera_target_pos.y += 20.0f;
            minimap_camera.focal_point = tg_game.camera->position;
            ta_camera_set_target_pos_absolute(&minimap_camera,
                minimap_camera_target_pos);
            ta_camera_update(&minimap_camera, sim_dt);

            // Update scene
            ta_scene_update(tg_game.scene, sim_dt);

            ms_sim_t += ms_sim_dt;
            ms_frame_accum -= ms_sim_dt;
        }

        if (tg_debug_follow_pinky) {
            ta_vec3 position_target = vec3_add(tg_game.player->transform.position,
                (ta_vec3) { 0.0, 1.0f, 3.0f });
            ta_camera_set_target_pos_absolute(tg_game.camera, position_target);
        }

        const double sim_alpha = (double)ms_frame_accum / ms_sim_dt;
        // TODO: state = state_current * sim_alpha + state_prev (1.0 - sim_alpha);
        // TODO: render(sim_alpha)

        ta_mat3 rotate_sun = mat3_rotate_z(1.0f);
        tg_game.sun->data.sun.direction =
            mat3_mul_vec3(rotate_sun, tg_game.sun->data.sun.direction);

		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Draw models
		//glDisable(GL_CULL_FACE);
        ta_scene_render(tg_game.scene, tg_game.camera);

        ta_shader_set_mat4(tg_shader_lines, SYM_U_PROJ, &tg_game.camera->projection);
        ta_shader_set_mat4(tg_shader_lines, SYM_U_VIEW, &tg_game.camera->look_at);

        // World axes
        ta_shader_set_mat4(tg_shader_lines, SYM_U_MODEL, &MAT4_IDENT);
        ta_primitive_push_axes(2.0f);
        ta_primitive_render();
        ta_primitive_clear();

        if (tg_debug_render_normals || tg_debug_render_bounding_boxes) {
            // TODO: This should take entity transform into account
            dlb_vec_each(ta_entity *, entity, tg_game.scene->entities) {
                if (tg_debug_render_normals) {
                    ta_entity_push_normals(entity);
                }
                if (tg_debug_render_bounding_boxes) {
                    ta_entity_push_aabb(entity, TA_COLOR_RED);
                }
                ta_shader_set_mat4(tg_shader_lines, SYM_U_MODEL, &entity->model);
                ta_primitive_render();
                ta_primitive_clear();
            }
        }

        // Minimap
		ta_viewport_bind(&minimap_viewport, true);
		{
			// TODO: Mesh selector, highlight and rotate mesh while mouse hover
			//ta_mat4 model = mat4_rotate_y(model_deg);
			//model_deg += 1.0f;
			//if (model_deg >= 360.0f) {
			//	model_deg = 0.0f;
			//}

			// Draw models
            ta_scene_render(tg_game.scene, minimap_viewport.camera);

            // Red dot on map
            ta_primitive_push_rect(
                minimap_viewport.rect.x + minimap_viewport.rect.w / 2 - 2,
                minimap_viewport.rect.y + minimap_viewport.rect.h / 2 - 2,
                (ta_rect) { 0, 0, 4, 4 },
                TA_COLOR_RED
            );
		}
		ta_viewport_unbind(&minimap_viewport);
		glEnable(GL_CULL_FACE);

		// Barchart
        ta_shader_set_mat4(tg_shader_lines, SYM_U_PROJ, &MAT4_IDENT);
        ta_shader_set_mat4(tg_shader_lines, SYM_U_VIEW, &MAT4_IDENT);
        ta_shader_set_mat4(tg_shader_lines, SYM_U_MODEL, &MAT4_IDENT);
        ta_ui_barchart_draw(0, 0, &chart);
        ta_primitive_render();
        ta_primitive_clear();

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
        frame_num++;
    }

    ta_window_free();
    ta_log_write(tg_debug_log, "Goodbye.\n\n");
    return 0;
}