#include "ta_timer.h"
#include "ta_log.h"
#include "ta_window.h"
#include "ta_render.h"
#include "ta_file.h"
#include "ta_shader.h"
#include "ta_shader_lines.h"
#include "ta_shader_quads.h"
#include "ta_shader_mesh.h"
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
#include "ta_schema.h"
#include "ta_parse.h"
#include "ta_scene.h"
#include "ta_symbol.h"
#include "dlb_types.h"
#define DLB_VECTOR_IMPLEMENTATION
#include "dlb_vector.h"
#define DLB_HASH_IMPLEMENTATION
#define DLB_HASH_TEST
#include "dlb_hash.h"
#include "misc/gl3w.h"
#include "SDL/SDL.h"

static bool debug_a = false;

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
    ta_entity *e = ta_scene_obj_init(scn, F_TA_ENTITY);
    e->type = ENTITY_DEFAULT;
    e->name = name;
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
    ta_scene *scene = ta_scene_init("test scene");
    entity_create(scene, "Timmy");
    entity_create(scene, "Bobby");

    printf("[WRITE: %s]\n", filename);
    ta_scene_print(scene, stdout);
    printf("\n");

    ta_file *data_file = ta_file_open(filename, FILE_WRITE);
    ta_scene_print(scene, data_file->hnd);
    ta_file_close(data_file);
    ta_scene_free(scene);
}

void read_scene(const char *filename) {
    ta_log_write(tg_debug_log, "[Scene] Loading %s\n", filename);
    ta_file *data_file = ta_file_open(filename, FILE_READ);
    ta_scene *scene = ta_scene_load(data_file);
    ta_file_close(data_file);

    ta_log_write(tg_debug_log, "[Scene] Scene loaded successfully:\n");
    ta_scene_print(scene, tg_debug_log->stream);
    ta_scene_free(scene);
}

int main(int argc, char *argv[])
{
    UNUSED(argc);
    UNUSED(argv);
    tg_game.state = TA_STATE_INIT;
    ta_timer_init();
	srand((u32)ta_timer_only_ms());
    debug_tests();

	ta_log debug_log;
	tg_debug_log = &debug_log;
    ta_log_init(tg_debug_log, "log.txt", true);

    ta_symbol_init();
    ta_schema_register();

	// Window setup
    ta_window_init(1600, 900, 80.0f, 0.1f, false);

    // OpenGL setup
    ta_render_init();

    ta_mouse_init();
    ta_keyboard_init();

    read_scene("data/scenes/scene1.dml");

	// Shader setup
    ta_primitive_init();

	// Mesh setup
	const ta_rgba mesh_selector_bg = { 0.1f, 0.1f, 0.2f, 1.0f };
	ta_viewport mesh_selector = ta_viewport_init(10, 50, 200, 200, 90.0f, 0.1f,
		mesh_selector_bg);

	ta_texture_2d *tex_test = ta_texture_init(TA_TEXTURE_QUEUE_STATIC,
		"data/texture/genesis_1024_1024.png");
	UNUSED(tex_test);

#if 0
	ta_mesh_load_obj_file(TA_MESH_QUEUE_STATIC, "data/mesh/prim_cube.obj");
    ta_mesh *mesh_cube = dlb_hash_search(&tg_mesh_table, CSTR("prim_cube"));
#else
	ta_mesh_load_obj_file(TA_MESH_QUEUE_STATIC, "data/models/Chamber0001.obj");
	ta_mesh *mesh_cube = dlb_hash_search(&tg_mesh_table, CSTR("chamber0001_base"));
#endif
	if (!mesh_cube) {
		DLB_ASSERT(!"Failed to load or find mesh");
	}
    ta_mesh_init_vertex_normals(mesh_cube, 0.5f);
    ta_mesh_init_face_normals(mesh_cube, 0.5f);

	//ta_mat4 project = mat4_perspective(65.0f, aspect, 0.1f, 100.0f);
	//float oo = 0.5f;
	//ta_mat4 project = mat4_ortho(-oo, oo, -oo, oo, 0.1f, 10.0f);

    tg_camera.position = (ta_vec3) { 0.0f, 1.7f, 24.0f };
    tg_camera.velocity = 0.1f;
    tg_camera.accel_yaw = 0.1f;
    tg_camera.accel_pitch = 0.1f;
    tg_camera.yaw = 90.0f;
    ta_camera_update(&tg_camera);
	ta_mat4 look_at_map = ta_camera_lookat(
		(ta_vec3) { 0.0f, 10.0f, 30.0f },
		(ta_vec3) { 0.0f, 0.0f, 0.0f },
		VEC3_Y
	);

    ta_line_3d X_AXIS = { 0 };
    ta_line_3d Y_AXIS = { 0 };
    ta_line_3d Z_AXIS = { 0 };
    X_AXIS.p1 = vec3_scalef(VEC3_X, 5.0f);
    Y_AXIS.p1 = vec3_scalef(VEC3_Y, 5.0f);
    Z_AXIS.p1 = vec3_scalef(VEC3_Z, 5.0f);

	float model_deg = 0.0f;

	ta_shader_mesh_init();
	ta_shader_mesh_bind();
	ta_shader_mesh_set_view(&tg_camera.look_at);
	ta_shader_mesh_set_texture(0, tex_test->gl_id);
	ta_shader_mesh_unbind();

	// TODO: Remove x,y coords from init() methods and only store size. Pass x,y
	//       at render time (make sure to update viewport correctly).
	ta_ui_image *ui_image = ta_ui_image_init(0, 0, 0, 0, tex_test);
	ta_ui_scrollview *view = ta_ui_scrollview_init(420, 50, 800, 300,
		(ta_ui_base *)ui_image);

	ta_ui_barchart chart = ta_ui_barchart_init(10, 10, tg_window.width - 20, 30);

    // TODO: What other startup states would be useful (e.g. LOADING_MESHES)?
    //       Could use this for a progress bar during load and better logging.
    //       Maybe also have JUMPING, CLIMBING, etc.? Could use bit flags to
    //       capture overall state as well (e.g. PLAYING, EDITING, etc.)
    tg_game.state = TA_STATE_PLAY;
    u64 frame_num = 0;
    while (tg_game.state != TA_STATE_QUIT) {
        frame_num++;
        ta_event_update();
		ta_mouse_update();
        ta_keyboard_update();

        // Handle events
        {
            ta_event event;

            // Global events
            while (ta_event_pop(&event, TA_EVENT_QUEUE_GLOBAL)) {
                switch (event.type) {
                    case TA_EVENT_GLOBAL_QUIT: {
                        tg_game.state = TA_STATE_QUIT;
                        break;
                    } case TA_EVENT_GLOBAL_MOUSE_MOVE: {
                        if (!tg_mouse.captured) break;

                        switch (tg_game.state) {
                            case TA_STATE_PLAY: {
                                ta_event cam_rotate_evt = { 0 };
                                cam_rotate_evt.type = TA_EVENT_CAMERA_ROTATE;
                                if (event.data.mouse_move.dx) {
                                    cam_rotate_evt.data.camera_rotate.delta_yaw =
                                        -event.data.mouse_move.dx * tg_camera.accel_yaw;
                                }
                                if (event.data.mouse_move.dy) {
                                    cam_rotate_evt.data.camera_rotate.delta_pitch =
                                        -event.data.mouse_move.dy * tg_camera.accel_pitch;
                                }
                                ta_event_push(&cam_rotate_evt);
                                break;
                            } default: {
                                DLB_ASSERT(!"Unhandled state");
                            }
                        }
                        break;
                    } case TA_EVENT_GLOBAL_MOUSE_CLICK: {
                        break;
                    } case TA_EVENT_GLOBAL_MOUSE_SCROLL: {
                        ta_ui_scrollview_scroll(view, event.data.mouse_scroll.y *
                            -event.data.mouse_scroll.flipped);
                        break;
                    } case TA_EVENT_GLOBAL_TOGGLE_MOUSE_LOCK: {
                        ta_mouse_toggle_capture();
                        break;
                    } case TA_EVENT_GLOBAL_TOGGLE_WIREFRAME: {
                        tg_camera.wireframe = !tg_camera.wireframe;
                        glPolygonMode(GL_FRONT_AND_BACK,
                            tg_camera.wireframe ? GL_LINE : GL_FILL);
                        break;
                    } case TA_EVENT_GLOBAL_TOGGLE_DEBUG_A: {
                        debug_a = !debug_a;
                        break;
                    } default: {
                        DLB_ASSERT(!"Unhandled event type");
                    }
                }
            }

            // Camera events
            // TODO: Normalize to prevent fast diagonal movement
            while (ta_event_pop(&event, TA_EVENT_QUEUE_CAMERA)) {
                switch (event.type) {
                    case TA_EVENT_CAMERA_MOVE_FORWARD: {
                        ta_camera_move(&tg_camera, TA_CAMERA_FORWARD);
                        break;
                    } case TA_EVENT_CAMERA_MOVE_BACKWARD: {
                        ta_camera_move(&tg_camera, TA_CAMERA_BACKWARD);
                        break;
                    } case TA_EVENT_CAMERA_MOVE_RIGHT: {
                        ta_camera_move(&tg_camera, TA_CAMERA_RIGHT);
                        break;
                    } case TA_EVENT_CAMERA_MOVE_LEFT: {
                        ta_camera_move(&tg_camera, TA_CAMERA_LEFT);
                        break;
                    } case TA_EVENT_CAMERA_MOVE_UP: {
                        ta_camera_move(&tg_camera, TA_CAMERA_UP);
                        break;
                    } case TA_EVENT_CAMERA_MOVE_DOWN: {
                        ta_camera_move(&tg_camera, TA_CAMERA_DOWN);
                        break;
                    } case TA_EVENT_CAMERA_ROTATE: {
                        if (event.data.camera_rotate.delta_yaw) {
                            tg_camera.yaw += event.data.camera_rotate.delta_yaw;
                            while (tg_camera.yaw < 0.0f) { tg_camera.yaw += 360.0f; }
                            while (tg_camera.yaw >= 360.0f) { tg_camera.yaw -= 360.0f; }
                            tg_camera.dirty = true;
                        }
                        if (event.data.camera_rotate.delta_pitch) {
                            tg_camera.pitch += event.data.camera_rotate.delta_pitch;
                            tg_camera.pitch = clampf(tg_camera.pitch, -75.0f, 75.0f);
                            tg_camera.dirty = true;
                        }
                        break;
                    } default: {
                        DLB_ASSERT(!"Unhandled event type");
                    }
                }
            }
        }

		// Update camera
		if (tg_camera.dirty) {
            ta_camera_update(&tg_camera);
		}

		glClearColor(0.5f, 0.8f, 1.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Draw models
		glDisable(GL_CULL_FACE);
		ta_shader_mesh_set_projection(&tg_window.projection);
		ta_shader_mesh_set_view(&tg_camera.look_at);
		ta_shader_mesh_set_model(&MAT4_IDENT);
        ta_shader_mesh_bind();
        ta_shader_mesh_prerender();
		ta_shader_mesh_render(mesh_cube);
		ta_shader_mesh_unbind();

        ta_shader_lines_set_projection(&tg_window.projection);
        ta_shader_lines_set_view(&tg_camera.look_at);
        ta_shader_lines_set_model(&MAT4_IDENT);

        //ta_primitive_push_line_3d(&X_AXIS, &TA_COLOR_RED,   &TA_COLOR_RED);
        //ta_primitive_push_line_3d(&Y_AXIS, &TA_COLOR_GREEN, &TA_COLOR_GREEN);
        //ta_primitive_push_line_3d(&Z_AXIS, &TA_COLOR_BLUE,  &TA_COLOR_BLUE);

        //////////////////////////////////////////
        if (debug_a) {
            ta_mesh_push_normals(mesh_cube);
            ta_primitive_render();
            ta_primitive_clear();
        }
        //////////////////////////////////////////

		ta_viewport_bind(&mesh_selector, true);
		{
			// Update models
			ta_mat4 model = mat4_rotate_y(model_deg);
			model_deg += 1.0f;
			if (model_deg >= 360.0f) {
				model_deg = 0.0f;
			}

			// Draw models
			ta_shader_mesh_set_projection(&mesh_selector.projection);
			ta_shader_mesh_set_view(&look_at_map);
			ta_shader_mesh_set_model(&model);
            ta_shader_mesh_bind();
            ta_shader_mesh_prerender();
			ta_shader_mesh_render(mesh_cube);
			ta_shader_mesh_unbind();
		}
		ta_viewport_unbind(&mesh_selector);
		glEnable(GL_CULL_FACE);

		// Draw UI
        ta_shader_lines_set_projection(&MAT4_IDENT);
        ta_shader_lines_set_view(&MAT4_IDENT);
        ta_shader_lines_set_model(&MAT4_IDENT);
        ta_ui_barchart_draw(0, 0, &chart);
        ta_primitive_render();
        ta_primitive_clear();

#if 0
		ta_ui_scrollview_draw(0, 0, view);
        ta_primitive_render();
        ta_primitive_clear();
#endif

        ta_ui_clear();

        ta_window_swap();
		// TODO: Save ticks immediately after swap for physics/vsync
    }

    ta_window_free();
    ta_log_write(tg_debug_log, "Goodbye.\n\n");
    return 0;
}