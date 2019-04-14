#include "ta_timer.h"
#include "ta_log.h"
#include "ta_window.h"
#include "ta_render.h"
#include "ta_file.h"
#include "ta_shader.h"
#include "ta_shader_quads.h"
#include "ta_shader_mesh.h"
#include "ta_ui_scrollview.h"
#include "ta_barchart.h"
#include "ta_texture.h"
#include "ta_mesh.h"
#include "ta_camera.h"
#include "ta_viewport.h"
#include "dlb_types.h"
#define DLB_VECTOR_IMPLEMENTATION
#include "dlb_vector.h"
#define DLB_HASH_IMPLEMENTATION
#include "dlb_hash.h"
#include "misc/gl3w.h"
#include "SDL/SDL.h"

#define TA_INT_KEY(key) (void *)(key), sizeof(key)
#define TA_IS_KEYDOWN(key) dlb_hash_search(&tg_key_states, TA_INT_KEY(key))
#define TA_KEYDOWN(key) dlb_hash_insert(&tg_key_states, TA_INT_KEY(key), (void *)1)
#define TA_KEYUP(key) dlb_hash_insert(&tg_key_states, TA_INT_KEY(key), (void *)0)
dlb_hash tg_key_states;

#if _DEBUG
DLB_ASSERT_HANDLER(handle_assert)
{
    ta_log_write(tg_debug_log, "ASSERT FAILED:\nfile: %s:%d \nmessage: %s\n",
		filename, line, expr);
	exit(-1);
}
dlb_assert_handler_def *dlb_assert_handler = handle_assert;
#endif

// pitch: -89.0f - 89.0f deg
// yaw: 0.0f - 360.0f deg
ta_vec3 cam_target(ta_vec3 pos, float pitch, float yaw)
{
	DLB_ASSERT(pitch > -90.0f);
	DLB_ASSERT(pitch < 90.0f);
	DLB_ASSERT(yaw >= 0.0f);
	DLB_ASSERT(yaw < 360.0f);
	ta_vec3 result = { 0 };
	ta_vec3 dir = { 0 };
	float pitch_rads = DEG_TO_RADF(pitch);
	float yaw_rads = DEG_TO_RADF(yaw);
	dir.x = cosf(yaw_rads);
	dir.y = sinf(pitch_rads);
	dir.z = -sinf(yaw_rads);
	result = vec3_add(pos, dir);
	return result;
}

int main(int argc, char *argv[])
{
    UNUSED(argc);
    UNUSED(argv);

	ta_timer_init();
	srand((u32)ta_timer_now_ms());
	ta_log debug_log;
	tg_debug_log = &debug_log;
    ta_log_init(tg_debug_log, "log.txt", true);

	// Window setup
    ta_window_init(1600, 900, 65.0f, 0.1f, false);

	// Mouse setup
	SDL_SetRelativeMouseMode(true);
	int mouse_x, mouse_y;
	int mouse_dx, mouse_dy;
	SDL_GetMouseState(&mouse_x, &mouse_y);
	mouse_dx = 0;
	mouse_dy = 0;

	// Keyboard setup
	dlb_hash_init(&tg_key_states, DLB_HASH_INT, "key_states", 16);
	SDL_Keycode key_quit = SDLK_ESCAPE;
	SDL_Keycode key_move_forward = SDLK_w;
	SDL_Keycode key_move_backward = SDLK_s;
	SDL_Keycode key_move_left = SDLK_a;
	SDL_Keycode key_move_right = SDLK_d;
	SDL_Keycode key_move_up = SDLK_SPACE;
	SDL_Keycode key_move_down = SDLK_LSHIFT;

	// OpenGL setup
    ta_render_init();

	// Shader setup
    ta_primitive_init();

	// Mesh setup
	const ta_color mesh_selector_bg = { 0.1f, 0.1f, 0.2f, 1.0f };
	ta_viewport mesh_selector = ta_viewport_init(10, 50, 400, 400, 65.0f, 0.1f,
		mesh_selector_bg);

	ta_texture_2d *tex_test = ta_texture_init(TA_TEXTURE_QUEUE_STATIC,
		"data/texture/wall_512_512.png");
	UNUSED(tex_test);

	//ta_mesh_load_obj_file(TA_MESH_QUEUE_STATIC, "data/mesh/prim_cube.obj");
	ta_mesh_load_obj_file(TA_MESH_QUEUE_STATIC, "data/models/Chamber0001.obj");
	ta_mesh *mesh_cube = dlb_hash_search(&tg_mesh_table, CSTR("chamber0001_base"));
	if (!mesh_cube) {
		DLB_ASSERT(!"Failed to load or find mesh");
	}

	//ta_mat4 project = mat4_perspective(65.0f, aspect, 0.1f, 100.0f);
	//float oo = 0.5f;
	//ta_mat4 project = mat4_ortho(-oo, oo, -oo, oo, 0.1f, 10.0f);

	ta_camera cam = { 0 };
	float c_pitch = 0.0f;
	float c_pitch_speed = 0.1f;
	float c_yaw = 90.0f;
	float c_yaw_speed = 0.1f;
	ta_vec3 c_pos = { 0.0f, 1.7f, 24.0f };
	float c_pos_speed = 0.2f;
	ta_vec3 c_target = cam_target(c_pos, 0.0f, c_yaw);
	ta_mat4 look_at = ta_camera_lookat(&cam, &c_pos, &c_target, &VEC3_UP);

	float model_deg = 0.0f;

	ta_shader_mesh_init();
	ta_shader_mesh_bind();
	ta_shader_mesh_set_view(&look_at);
	ta_shader_mesh_set_texture(0, tex_test->gl_id);
	ta_shader_mesh_unbind();

	// TODO: Remove x,y coords from init() methods and only store size. Pass x,y
	//       at render time (make sure to update viewport correctly).
	ta_ui_image *ui_image = ta_ui_image_init(0, 0, 0, 0, tex_test);
	ta_ui_scrollview *view = ta_ui_scrollview_init(420, 50, 800, 300,
		(ta_ui_base *)ui_image);

	ta_barchart chart = ta_barchart_init(10, 10, tg_window.width - 20, 30);

	SDL_Event event;
    bool quit = false;
    while (!quit) {
		bool camera_dirty = false;

		// Update mouse
		{
			SDL_GetRelativeMouseState(&mouse_dx, &mouse_dy);
			mouse_x += mouse_dx;
			mouse_y += mouse_dy;

			if (mouse_dx) {
				c_yaw -= c_yaw_speed * mouse_dx;
				while (c_yaw < 0.0f) { c_yaw += 360.0f; }
				while (c_yaw >= 360.0f) { c_yaw -= 360.0f; }
				camera_dirty = true;
			}
			if (mouse_dy) {
				c_pitch -= c_pitch_speed * mouse_dy;
				c_pitch = clampf(c_pitch, -89.0f, 89.0f);
				camera_dirty = true;
			}
		}

        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_QUIT:
                quit = true;
                break;
            case SDL_WINDOWEVENT:
				break;
			case SDL_KEYDOWN:
				TA_KEYDOWN(event.key.keysym.sym);
                break;
            case SDL_KEYUP:
				TA_KEYUP(event.key.keysym.sym);
				break;
			case SDL_MOUSEBUTTONDOWN:
				break;
			case SDL_MOUSEBUTTONUP:
				break;
            case SDL_MOUSEWHEEL:
				ta_ui_scrollview_scroll(view, -event.wheel.y);
				break;
            case SDL_MOUSEMOTION:
				break;
			case SDL_TEXTEDITING:
                break;
            default:
                ta_log_write(tg_debug_log, "Unhandled event type: %d\n", event.type);
            }
        }

		if (TA_IS_KEYDOWN(key_quit)) {
			quit = true;
		}

		// TODO: Normalize to prevent fast diagonal movement
		if (TA_IS_KEYDOWN(key_move_forward)) {
			c_pos = vec3_add(c_pos, vec3_scalef(cam.front, c_pos_speed));
			camera_dirty = true;
		}
		if (TA_IS_KEYDOWN(key_move_backward)) {
			c_pos = vec3_sub(c_pos, vec3_scalef(cam.front, c_pos_speed));
			camera_dirty = true;
		}
		if (TA_IS_KEYDOWN(key_move_left)) {
			c_pos = vec3_sub(c_pos, vec3_scalef(cam.right, c_pos_speed));
			camera_dirty = true;
		}
		if (TA_IS_KEYDOWN(key_move_right)) {
			c_pos = vec3_add(c_pos, vec3_scalef(cam.right, c_pos_speed));
			camera_dirty = true;
		}
		if (TA_IS_KEYDOWN(key_move_up)) {
			c_pos = vec3_add(c_pos, vec3_scalef(cam.up, c_pos_speed));
			camera_dirty = true;
		}
		if (TA_IS_KEYDOWN(key_move_down)) {
			c_pos = vec3_sub(c_pos, vec3_scalef(cam.up, c_pos_speed));
			camera_dirty = true;
		}

		// Update camera
		if (camera_dirty) {
			c_target = cam_target(c_pos, c_pitch, c_yaw);
			look_at = ta_camera_lookat(&cam, &c_pos, &c_target, &VEC3_UP);
			ta_shader_mesh_set_view(&look_at);
		}

		glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Draw models
		glDisable(GL_CULL_FACE);
		ta_shader_mesh_bind();
		ta_shader_mesh_set_projection(&tg_window.projection);
		ta_shader_mesh_set_model(&MAT4_IDENT);
		ta_shader_mesh_prerender();
		ta_shader_mesh_render(mesh_cube);
		ta_shader_mesh_unbind();

		ta_viewport_bind(&mesh_selector, true);
		{
			// Update models
			ta_mat4 model = mat4_rotate_y(model_deg);
			model_deg += 1.0f;
			if (model_deg >= 360.0f) {
				model_deg = 0.0f;
			}

			// Draw models
			ta_shader_mesh_bind();
			ta_shader_mesh_set_projection(&mesh_selector.projection);
			ta_shader_mesh_set_model(&model);
			ta_shader_mesh_prerender();
			ta_shader_mesh_render(mesh_cube);
			ta_shader_mesh_unbind();
		}
		ta_viewport_unbind(&mesh_selector);
		glEnable(GL_CULL_FACE);

		// Draw UI
		ta_barchart_draw(0, 0, &chart);

		// Render UI
		//ta_ui_scrollview_draw(0, 0, view);
		ta_primitive_render();
		ta_primitive_clear();
		ta_ui_clear();

        ta_window_swap();
		// TODO: Save ticks immediately after swap for physics/vsync
    }

    ta_window_free();
    ta_log_write(tg_debug_log, "Goodbye.\n\n");
    return 0;
}