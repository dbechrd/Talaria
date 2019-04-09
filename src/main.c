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

#if _DEBUG
DLB_ASSERT_HANDLER(handle_assert)
{
    ta_log_write(tg_debug_log, "ASSERT FAILED:\nfile: %s:%d \nmessage: %s\n",
		filename, line, expr);
	exit(-1);
}
dlb_assert_handler_def *dlb_assert_handler = handle_assert;
#endif

int main(int argc, char *argv[])
{
    UNUSED(argc);
    UNUSED(argv);

	ta_timer_init();
	srand((u32)ta_timer_now_ms());
	ta_log debug_log;
	tg_debug_log = &debug_log;
    ta_log_init(tg_debug_log, "log.txt", true);
    ta_window_init(1600, 900, false);
    ta_render_init();
    ta_primitive_init();

	const ta_color mesh_selector_bg = { 0.1f, 0.1f, 0.2f, 1.0f };
	ta_viewport mesh_selector = ta_viewport_init(10, 50, 400, 400, mesh_selector_bg);

	float aspect = (float)mesh_selector.rect.w / mesh_selector.rect.h;
	//ta_mat4 project = mat4_perspective(65.0f, aspect, 0.1f, 1000.0f);
	ta_mat4 project = mat4_perspective_inf(65.0f, aspect, 0.1f);
	//float oo = 30.0f;
	//ta_mat4 project = mat4_ortho(-oo, oo, -oo, oo, -oo, oo);
	//ta_mat4 project = mat4_ortho(0.0f, (float)tg_window.width, (float)tg_window.height, 0.0f, 0.1f, 10.0f);
	//ta_mat4 project = mat4_ortho(-0.5f, 0.5f, 0.5f, -0.5f, 0.1f, 10.0f);

	ta_vec3 c_pos = { 0.0f, 1.8f, 3.0f };
	ta_vec3 c_target = { 0.0f, 0.0f, 0.0f };
	ta_camera cam = { 0 };
	ta_mat4 look_at = ta_camera_lookat(&cam, &c_pos, &c_target, &VEC3_UP);

	float model_deg = 0.0f;

	ta_texture_2d *tex_test = ta_texture_init(TA_TEXTURE_QUEUE_STATIC,
		"data/texture/wall_512_512.png");
	UNUSED(tex_test);

	ta_mesh_load_obj_file(TA_MESH_QUEUE_STATIC, "data/mesh/prim_cube.obj");
	ta_mesh *mesh_cube = dlb_hash_search(&tg_mesh_table, CSTR("prim_cube"));
	UNUSED(mesh_cube);

	ta_shader_mesh_init();
	ta_shader_mesh_bind();
	ta_shader_mesh_set_projection(&project);
	ta_shader_mesh_set_view(&look_at);
	ta_shader_mesh_set_texture(0, tex_test->gl_id);
	ta_shader_mesh_unbind();

	// TODO: Remove x,y coords from init() methods and only store size. Pass x,y
	//       at render time (make sure to update viewport correctly).
	ta_ui_image *ui_image = ta_ui_image_init(0, 0, 0, 0, tex_test);
	ta_ui_scrollview *view = ta_ui_scrollview_init(420, 50, 800, 300,
		(ta_ui_base *)ui_image);

	ta_barchart chart = ta_barchart_init(10, 10, tg_window.width - 20, 30, 525);

    SDL_Event event;
    bool quit = false;
    while (!quit) {
        int mouse_x, mouse_y;
        SDL_GetMouseState(&mouse_x, &mouse_y);

        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_QUIT:
                quit = true;
                break;
            case SDL_WINDOWEVENT:
				break;
			case SDL_KEYDOWN:
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    quit = true;
                }
                break;
            case SDL_KEYUP:
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

        /*{
            ta_vert_line line = { 0 };
            line.verts[0].color.r = 1.0f;
            line.verts[1].color.g = 1.0f;
            line.verts[1].pos.x = 2.0f * (float)mouse_x / ta_g_window.width - 1.0f;
			line.verts[1].pos.y = 2.0f * -(float)mouse_y / ta_g_window.height + 1.0f;
			line.verts[0].pos.z = -0.1f;
			line.verts[1].pos.z = -0.1f;
            ta_debug_push_line(&line);
        }

		{
			ta_vert_quad quad = { 0 };
			ta_bbox_2d bbox = { 0 };
			bbox.center = (ta_vec2) { 0.0f, 0.0f };
			bbox.half_axes = (ta_vec2) { 0.1f, 0.1f };
			ta_color4 color = (ta_color4) { 1.0f, 1.0f, 0.0f, 1.0f };
			ta_bbox_to_quad(&quad, &bbox, &color);
			ta_debug_push_quad(&quad);
		}*/

		glClearColor(0.9f, 0.9f, 0.9f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		ta_viewport_bind(&mesh_selector, true);
		{
			// Update models
			//ta_mat4 rotate = mat4_rotate_y(model_deg);
			//ta_vec3 trans = { 0.0f, 0.0f, 0.0f };
			//ta_mat4 translate = mat4_translate(&trans);
			//ta_mat4 model = mat4_mul(translate, rotate);
			ta_mat4 model = mat4_rotate_y(model_deg);
			model_deg += 1.0f;
			if (model_deg >= 360.0f) model_deg = 0.0f;
		
			// Draw models
			ta_shader_mesh_bind();
			ta_shader_mesh_set_model(&model);
			ta_shader_mesh_prerender();
			ta_shader_mesh_render(mesh_cube);
			ta_shader_mesh_unbind();
		}
		ta_viewport_unbind(&mesh_selector);

		// Draw UI
		ta_ui_scrollview_draw(0, 0, view);
		ta_barchart_draw(0, 0, &chart);

		// Render UI
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