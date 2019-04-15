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

typedef enum {
    TA_EVENT_QUEUE_GLOBAL,
    TA_EVENT_QUEUE_CAMERA,
    TA_EVENT_QUEUE_COUNT
} ta_event_queue_type;

#define TA_EVENT_TYPE_BITS 7
#define TA_EVENT_TYPE_QUEUE(type) ((type) >> TA_EVENT_TYPE_BITS)
#define TA_EVENT_TYPE_FIRST(queue) ((queue) << TA_EVENT_TYPE_BITS)

typedef enum {
    // Global events
    TA_EVENT_GLOBAL_QUIT = TA_EVENT_TYPE_FIRST(TA_EVENT_QUEUE_GLOBAL),
    TA_EVENT_GLOBAL_MOUSE_MOVE,

    // Camera events
    TA_EVENT_CAMERA_MOVE_FORWARD = TA_EVENT_TYPE_FIRST(TA_EVENT_QUEUE_CAMERA),
    TA_EVENT_CAMERA_MOVE_BACKWARD,
    TA_EVENT_CAMERA_MOVE_LEFT,
    TA_EVENT_CAMERA_MOVE_RIGHT,
    TA_EVENT_CAMERA_MOVE_UP,
    TA_EVENT_CAMERA_MOVE_DOWN,
    TA_EVENT_CAMERA_ROTATE,
} ta_event_type;

typedef struct {
    ta_event_type type;
    int dx;
    int dy;
} ta_event_mouse_move;

typedef struct {
    ta_event_type type;
    float delta_pitch;
    float delta_yaw;
} ta_event_camera_rotate;

typedef struct {
    ta_event_type type;
    union {
        ta_event_mouse_move mouse_move;
        ta_event_camera_rotate camera_rotate;
    } data;
} ta_event;

typedef struct {
    ta_event_queue_type type;
	u32 head;  // oldest item
	u32 count;
	u32 capacity;
	ta_event *buffer;
} ta_event_queue;
ta_event_queue tg_event_queues[TA_EVENT_QUEUE_COUNT];

void ta_event_push(ta_event *event)
{
    ta_event_queue *queue = &tg_event_queues[TA_EVENT_TYPE_QUEUE(event->type)];
    if (queue->count == queue->capacity) {
        u32 old_size = dlb_vec_size(queue->buffer);
        u32 new_cap = MAX(16, queue->capacity * 2);
        dlb_vec_reserve(queue->buffer, new_cap);
        if (old_size) {
            // Before resize: [D, A, B, C]
            // After resize : [-, A, B, C, D, -, -, -]
            if (queue->head > 0) {
                int bytes = queue->head * sizeof(queue->buffer[0]);
                memcpy(&queue->buffer[queue->head + queue->count],
                    queue->buffer, bytes);
#if _DEBUG
                memset(queue->buffer, 0, bytes);
#endif
            }
        }
        queue->capacity = new_cap;
    }
    int next = (queue->head + queue->count) % queue->capacity;
    queue->buffer[next] = *event;
    queue->count++;
}

bool ta_event_pop(ta_event *event, ta_event_queue_type queue_type)
{
    ta_event_queue *queue = &tg_event_queues[queue_type];
    if (queue->count) {
        *event = queue->buffer[queue->head];
        queue->head = (queue->head + 1) % queue->capacity;
        queue->count--;
        return true;
    } else {
        return false;
    }
}

bool ta_event_peek(ta_event *event, ta_event_queue_type queue_type)
{
    ta_event_queue *queue = &tg_event_queues[queue_type];
    if (queue->count) {
        *event = queue->buffer[queue->head];
        return true;
    } else {
        return false;
    }
}

typedef enum {
    TA_STATE_INIT,
    TA_STATE_PLAY,
    TA_STATE_COUNT
} ta_state;
ta_state tg_state = TA_STATE_INIT;

#if 0
enum {
    TA_SCANCODE_MOUSE_MOVE = SDL_NUM_SCANCODES,
    TA_SCANCODE_COUNT,
};
#endif

typedef struct {
	bool down;
} ta_key_state;
ta_key_state tg_key_states[SDL_NUM_SCANCODES];

typedef SDL_Scancode ta_key;
typedef struct {
    ta_key keys[3];
} ta_keychord;

typedef struct {
    ta_event_type event_type;
    ta_keychord chord;
} ta_keybind;
ta_keybind *tg_keybinds[TA_STATE_COUNT];

void ta_keybind_bind(ta_state state_type, ta_event_type event_type,
    ta_keychord *chord)
{
    ta_keybind *bind = dlb_vec_alloc(tg_keybinds[state_type]);
    bind->event_type = event_type;
    bind->chord.keys[0] = chord->keys[0];
    bind->chord.keys[1] = chord->keys[1];
    bind->chord.keys[2] = chord->keys[2];
}

void ta_keybind_bind1(ta_state state_type, ta_event_type event_type,
    ta_key key1)
{
    ta_keybind *bind = dlb_vec_alloc(tg_keybinds[state_type]);
    bind->event_type = event_type;
    bind->chord.keys[0] = key1;
}

void ta_keybind_bind2(ta_state state_type, ta_event_type event_type,
    ta_key key1, ta_key key2)
{
    ta_keybind *bind = dlb_vec_alloc(tg_keybinds[state_type]);
    bind->event_type = event_type;
    bind->chord.keys[0] = key1;
    bind->chord.keys[1] = key2;
}

void ta_keybind_bind3(ta_state state_type, ta_event_type event_type,
    ta_key key1, ta_key key2, ta_key key3)
{
    ta_keybind *bind = dlb_vec_alloc(tg_keybinds[state_type]);
    bind->event_type = event_type;
    bind->chord.keys[0] = key1;
    bind->chord.keys[1] = key2;
    bind->chord.keys[2] = key3;
}

bool ta_keychord_down(ta_keychord *chord) {
    bool down =
        (!chord->keys[0] || tg_key_states[chord->keys[0]].down) &&
        (!chord->keys[1] || tg_key_states[chord->keys[1]].down) &&
        (!chord->keys[2] || tg_key_states[chord->keys[2]].down);
    return down;
}

// press key -> add event

//SDL_Scancode tg_keybinds[TA_KEYBIND_COUNT];
//ta_event_type tg_key_events[TA_KEYBIND_COUNT];

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

    tg_state = TA_STATE_PLAY;
	ta_log debug_log;
	tg_debug_log = &debug_log;
    ta_log_init(tg_debug_log, "log.txt", true);

    // Keyboard setup
    {
        // TODO: Read keybinds from file
        //dlb_vec_reserve(tg_keybinds, 16);
        ta_keybind_bind1(TA_STATE_PLAY, TA_EVENT_GLOBAL_QUIT,           SDL_SCANCODE_ESCAPE);
        ta_keybind_bind1(TA_STATE_PLAY, TA_EVENT_CAMERA_MOVE_FORWARD,   SDL_SCANCODE_W);
        ta_keybind_bind1(TA_STATE_PLAY, TA_EVENT_CAMERA_MOVE_BACKWARD,  SDL_SCANCODE_S);
        ta_keybind_bind1(TA_STATE_PLAY, TA_EVENT_CAMERA_MOVE_LEFT,      SDL_SCANCODE_A);
        ta_keybind_bind1(TA_STATE_PLAY, TA_EVENT_CAMERA_MOVE_RIGHT,     SDL_SCANCODE_D);
        ta_keybind_bind1(TA_STATE_PLAY, TA_EVENT_CAMERA_MOVE_UP,        SDL_SCANCODE_SPACE);
        ta_keybind_bind1(TA_STATE_PLAY, TA_EVENT_CAMERA_MOVE_DOWN,      SDL_SCANCODE_LSHIFT);
    }

    // Mouse setup
    int mouse_x, mouse_y;

	// Window setup
    ta_window_init(1600, 900, 65.0f, 0.1f, false);
	SDL_SetRelativeMouseMode(true);
	SDL_GetMouseState(&mouse_x, &mouse_y);

	// OpenGL setup
    ta_render_init();

	// Shader setup
    ta_primitive_init();

	// Mesh setup
	const ta_color mesh_selector_bg = { 0.1f, 0.1f, 0.2f, 1.0f };
	ta_viewport mesh_selector = ta_viewport_init(10, 50, 200, 200, 90.0f, 0.1f,
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
	float mouse_sensitivity_y = 0.1f;
	float c_yaw = 90.0f;
	float mouse_sensitivity_x = 0.1f;
	ta_vec3 c_pos = { 0.0f, 1.7f, 24.0f };
	float c_pos_speed = 0.2f;
	ta_vec3 c_target = cam_target(c_pos, 0.0f, c_yaw);
	ta_mat4 look_at = ta_camera_lookat(&cam, c_pos, c_target, VEC3_UP);
	ta_mat4 look_at_map = ta_camera_lookat(
		&cam,
		(ta_vec3) { 0.0f, 10.0f, 30.0f },
		(ta_vec3) { 0.0f, 0.0f, 0.0f },
		VEC3_UP
	);

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

    // TODO: What other startup states would be useful (e.g. LOADING_MESHES)?
    //       Could use this for a progress bar during load and better logging.
    //       Maybe also have JUMPING, CLIMBING, etc.? Could use bit flags to
    //       capture overall state as well (e.g. PLAYING, EDITING, etc.)
    tg_state = TA_STATE_PLAY;

	SDL_Event sdl_event;
    bool quit = false;
    while (!quit) {
		bool camera_dirty = false;

        while (SDL_PollEvent(&sdl_event)) {
            switch (sdl_event.type) {
                case SDL_QUIT: {
                    ta_event event = { 0 };
                    event.type = TA_EVENT_GLOBAL_QUIT;
                    ta_event_push(&event);
                    break;
                } case SDL_WINDOWEVENT: {
                    break;
                } case SDL_KEYDOWN: {
                    tg_key_states[sdl_event.key.keysym.scancode].down = true;
                    break;
                } case SDL_KEYUP: {
                    tg_key_states[sdl_event.key.keysym.scancode].down = false;
                    break;
                } case SDL_MOUSEBUTTONDOWN: {
                    break;
                } case SDL_MOUSEBUTTONUP: {
                    break;
                } case SDL_MOUSEWHEEL: {
                    ta_ui_scrollview_scroll(view, -sdl_event.wheel.y);
                    break;
                } case SDL_MOUSEMOTION: {
                    break;
                } case SDL_TEXTEDITING: {
                    break;
                } default: {
                    ta_log_write(tg_debug_log, "Unhandled event type: %d\n", sdl_event.type);
                }
            }
        }

        // Generate keybind events
        {
            for (ta_keybind *bind = tg_keybinds[tg_state];
                bind != dlb_vec_end(tg_keybinds[tg_state]); bind++) {
                if (ta_keychord_down(&bind->chord)) {
                    ta_event event = { 0 };
                    event.type = bind->event_type;
                    ta_event_push(&event);
                }
            }
        }

		// Generate mouse events
		{
            int mouse_dx, mouse_dy;
			SDL_GetRelativeMouseState(&mouse_dx, &mouse_dy);

            if (mouse_dx || mouse_dy) {
			    mouse_x += mouse_dx;
			    mouse_y += mouse_dy;

                ta_event mouse_move_evt = { 0 };
                mouse_move_evt.type = TA_EVENT_GLOBAL_MOUSE_MOVE;
                mouse_move_evt.data.mouse_move.dx = mouse_dx;
                mouse_move_evt.data.mouse_move.dy = mouse_dy;
                ta_event_push(&mouse_move_evt);
            }
		}

        // Handle events
        {
            ta_event event;

            // Global events
            while (ta_event_pop(&event, TA_EVENT_QUEUE_GLOBAL)) {
                switch (event.type) {
                    case TA_EVENT_GLOBAL_QUIT: {
                        quit = true;
                        break;
                    } case TA_EVENT_GLOBAL_MOUSE_MOVE: {
                        switch (tg_state) {
                            case TA_STATE_PLAY: {
                                ta_event cam_rotate_evt = { 0 };
                                cam_rotate_evt.type = TA_EVENT_CAMERA_ROTATE;
                                if (event.data.mouse_move.dx) {
                                    cam_rotate_evt.data.camera_rotate.delta_yaw =
                                        -mouse_sensitivity_x * event.data.mouse_move.dx;
                                }
                                if (event.data.mouse_move.dy) {
                                    cam_rotate_evt.data.camera_rotate.delta_pitch =
                                        -mouse_sensitivity_y * event.data.mouse_move.dy;
                                }
                                ta_event_push(&cam_rotate_evt);
                                break;
                            } default: {
                                DLB_ASSERT(!"Unhandled state");
                            }
                        }
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
                        ta_vec3 delta = vec3_scalef(cam.front, c_pos_speed);
                        delta.y = 0.0f;
                        c_pos = vec3_add(c_pos, delta);
                        camera_dirty = true;
                        break;
                    } case TA_EVENT_CAMERA_MOVE_BACKWARD: {
                        ta_vec3 delta = vec3_scalef(cam.front, c_pos_speed);
                        delta.y = 0.0f;
                        c_pos = vec3_sub(c_pos, delta);
                        camera_dirty = true;
                        break;
                    } case TA_EVENT_CAMERA_MOVE_LEFT: {
                        c_pos = vec3_sub(c_pos, vec3_scalef(cam.right, c_pos_speed));
                        camera_dirty = true;
                        break;
                    } case TA_EVENT_CAMERA_MOVE_RIGHT: {
                        c_pos = vec3_add(c_pos, vec3_scalef(cam.right, c_pos_speed));
                        camera_dirty = true;
                        break;
                    } case TA_EVENT_CAMERA_MOVE_UP: {
                        c_pos = vec3_add(c_pos, vec3_scalef(cam.up, c_pos_speed));
                        camera_dirty = true;
                        break;
                    } case TA_EVENT_CAMERA_MOVE_DOWN: {
                        c_pos = vec3_sub(c_pos, vec3_scalef(cam.up, c_pos_speed));
                        camera_dirty = true;
                        break;
                    } case TA_EVENT_CAMERA_ROTATE: {
                        if (event.data.camera_rotate.delta_yaw) {
                            c_yaw += event.data.camera_rotate.delta_yaw;
                            while (c_yaw < 0.0f) { c_yaw += 360.0f; }
                            while (c_yaw >= 360.0f) { c_yaw -= 360.0f; }
                            camera_dirty = true;
                        }
                        if (event.data.camera_rotate.delta_pitch) {
                            c_pitch += event.data.camera_rotate.delta_pitch;
                            c_pitch = clampf(c_pitch, -89.0f, 89.0f);
                            camera_dirty = true;
                        }
                        break;
                    } default: {
                        DLB_ASSERT(!"Unhandled event type");
                    }
                }
            }
        }

		// Update camera
		if (camera_dirty) {
			c_target = cam_target(c_pos, c_pitch, c_yaw);
			look_at = ta_camera_lookat(&cam, c_pos, c_target, VEC3_UP);
		}

		glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Draw models
		glDisable(GL_CULL_FACE);
		ta_shader_mesh_bind();
		ta_shader_mesh_set_projection(&tg_window.projection);
		ta_shader_mesh_set_view(&look_at);
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
			ta_shader_mesh_set_view(&look_at_map);
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