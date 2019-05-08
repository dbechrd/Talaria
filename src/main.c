#include "ta_timer.h"
#include "ta_log.h"
#include "ta_window.h"
#include "ta_render.h"
#include "ta_file.h"
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
    e->type = ENTITY_DEFAULT;
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
    tg_game.state = TA_STATE_INIT;
    ta_timer_init();
	srand((u32)ta_timer_only_ms());
    debug_tests();

	ta_log debug_log;
	tg_debug_log = &debug_log;
    ta_log_init(tg_debug_log, "log.txt", true);

    ta_symbol_init();
    ta_schema_register();

    ta_window_init(1600, 900, 80.0f, 0.1f, false);
    ta_render_init();
    ta_mouse_init();
    ta_keyboard_init();

    // Fallback resources
    ta_mesh_load_obj_file(TA_MESH_QUEUE_STATIC, "data/mesh/prim_cube.obj");
    ta_mesh *mesh_cube = dlb_hash_search(&tg_mesh_table, CSTR("prim_cube"));
    UNUSED(mesh_cube);

    // Intro scene
    read_scene("data/scenes/scene1.dml");
    DLB_ASSERT(tg_game.scene->cameras->dirty);  // Ensure we have a valid camera

    ////////////////////////////////////////////////////////////////////////////
    // Shaders
    ////////////////////////////////////////////////////////////////////////////
    tg_shader_lines = ta_scene_find(tg_game.scene, F_TA_SHADER,
        INTERN("shader_lines"));
    DLB_ASSERT(tg_shader_lines && "Failed to load or find shader_lines");

    tg_shader_quads = ta_scene_find(tg_game.scene, F_TA_SHADER,
        INTERN("shader_quads"));
    DLB_ASSERT(tg_shader_quads && "Failed to load or find shader_quads");

    tg_shader_mesh = ta_scene_find(tg_game.scene, F_TA_SHADER,
        INTERN("shader_mesh"));
    DLB_ASSERT(tg_shader_mesh && "Failed to load or find shader_mesh");

    // TODO: Figure out a better way to initialize attribs
    ta_primitive_init();

    ////////////////////////////////////////////////////////////////////////////
    // Textures
    ////////////////////////////////////////////////////////////////////////////
    ta_texture *tex_test = ta_scene_find(tg_game.scene, F_TA_TEXTURE,
        INTERN("texture_1"));
    DLB_ASSERT(tex_test && tex_test->gl_id && "Failed to load or find texture_1");

    ////////////////////////////////////////////////////////////////////////////
    // Meshes
    ////////////////////////////////////////////////////////////////////////////
    // TODO: Move this to DML and use ta_scene_find
    ta_mesh_load_obj_file(TA_MESH_QUEUE_LEVEL, "data/models/Chamber0001.obj");
    ta_mesh *mesh_chamber = dlb_hash_search(&tg_mesh_table, CSTR("chamber0001_base"));
    if (!mesh_chamber) {
        DLB_ASSERT(!"Failed to load or find mesh_chamber");
    }
    ta_mesh_init_vertex_normals(mesh_chamber, 0.5f);
    ta_mesh_init_face_normals(mesh_chamber, 0.5f);

    ////////////////////////////////////////////////////////////////////////////
    // UI
    ////////////////////////////////////////////////////////////////////////////
    // TODO: Move this to DML (e.g. editor.dml)
	const ta_rgba mesh_selector_bg = { 0.1f, 0.1f, 0.2f, 1.0f };
	ta_viewport mesh_selector = ta_viewport_init(10, 50, 200, 200, 90.0f, 0.1f,
		mesh_selector_bg);

	//ta_mat4 project = mat4_perspective(65.0f, aspect, 0.1f, 100.0f);
	//float oo = 0.5f;
	//ta_mat4 project = mat4_ortho(-oo, oo, -oo, oo, 0.1f, 10.0f);

    ta_camera_update(tg_game.scene->cameras);

	ta_mat4 look_at_map = mat4_lookat(
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

	// TODO: Remove x,y coords from init() methods and only store size. Pass x,y
	//       at render time (make sure to update viewport correctly).
	ta_ui_image *ui_image = ta_ui_image_init(0, 0, 0, 0, tex_test);
	ta_ui_scrollview *view = ta_ui_scrollview_init(420, 50, 800, 300,
		(ta_ui_base *)ui_image);

	ta_ui_barchart chart = ta_ui_barchart_init(10, 10, tg_window.width - 20, 30);

    ta_shader_set_sampler2d(tg_shader_mesh, INTERN("u_tex0"), tex_test->gl_id);

    // TODO: What other startup states would be useful (e.g. LOADING_MESHES)?
    //       Could use this for a progress bar during load and better logging.
    //       Maybe also have JUMPING, CLIMBING, etc.? Could use bit flags to
    //       capture overall state as well (e.g. PLAYING, EDITING, etc.)
    tg_game.state = TA_STATE_PLAY;

    ////////////////////////////////////////////////////////////////////////////
    // Main loop
    ////////////////////////////////////////////////////////////////////////////
    u64 frame_num = 0;
    while (tg_game.state != TA_STATE_QUIT) {
        frame_num++;
        ta_event_sdl_poll();
        ta_mouse_update();
        ta_keyboard_update();
        ta_event_update();
        ta_camera_update(tg_game.scene->cameras);

		glClearColor(0.5f, 0.8f, 1.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Draw models
		glDisable(GL_CULL_FACE);
        ta_shader_set_mat4(tg_shader_mesh, INTERN("u_proj"), &tg_window.projection);
		ta_shader_set_mat4(tg_shader_mesh, INTERN("u_view"), &tg_game.scene->cameras->look_at);
        ta_shader_set_mat4(tg_shader_mesh, INTERN("u_model"), &MAT4_IDENT);
        ta_shader_bind(tg_shader_mesh);
        ta_shader_prerender(tg_shader_mesh);
        ta_mesh_render(mesh_chamber);
        //ta_mesh_render(mesh_cube);
		ta_shader_unbind(tg_shader_mesh);

        ta_shader_set_mat4(tg_shader_lines, INTERN("u_proj"), &tg_window.projection);
        ta_shader_set_mat4(tg_shader_lines, INTERN("u_view"), &tg_game.scene->cameras->look_at);
        ta_shader_set_mat4(tg_shader_lines, INTERN("u_model"), &MAT4_IDENT);

        //ta_primitive_push_line_3d(&X_AXIS, &TA_COLOR_RED,   &TA_COLOR_RED);
        //ta_primitive_push_line_3d(&Y_AXIS, &TA_COLOR_GREEN, &TA_COLOR_GREEN);
        //ta_primitive_push_line_3d(&Z_AXIS, &TA_COLOR_BLUE,  &TA_COLOR_BLUE);

        //////////////////////////////////////////
        if (tg_debug_a) {
            ta_mesh_push_normals(mesh_chamber);
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
            ta_shader_set_mat4(tg_shader_mesh, INTERN("u_proj"), &mesh_selector.projection);
            ta_shader_set_mat4(tg_shader_mesh, INTERN("u_view"), &look_at_map);
            ta_shader_set_mat4(tg_shader_mesh, INTERN("u_model"), &model);
            ta_shader_bind(tg_shader_mesh);
            ta_shader_prerender(tg_shader_mesh);
            ta_mesh_render(mesh_chamber);
            ta_shader_unbind(tg_shader_mesh);
		}
		ta_viewport_unbind(&mesh_selector);
		glEnable(GL_CULL_FACE);

		// Draw UI
        ta_shader_set_mat4(tg_shader_lines, INTERN("u_proj"), &MAT4_IDENT);
        ta_shader_set_mat4(tg_shader_lines, INTERN("u_view"), &MAT4_IDENT);
        ta_shader_set_mat4(tg_shader_lines, INTERN("u_model"), &MAT4_IDENT);
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