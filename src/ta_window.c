#include "ta_window.h"
#include "ta_log.h"
#include "ta_event.h"
#include "ta_primitive.h"
#include "ta_camera.h"
#include "ta_game.h"
#include "ta_scene.h"
#include "dlb/dlb_types.h"
#include "dlb/dlb_memory.h"
#include "dlb/dlb_vector.h"
#include "SDL/SDL.h"
#include "misc/gl3w.h"

typedef struct ta_window {
    ta_size size;
    float aspect;
    SDL_Window *sdl_window;
    SDL_GLContext gl_context;
} ta_window;
static ta_window internal_window;
ta_window *tg_window = &internal_window;

static void sdl_gl_attrib(SDL_GLattr attr, int value)
{
    int sdl_err = SDL_GL_SetAttribute(attr, value);
    if (sdl_err < 0)
    {
        ta_log_write(&tg_debug_log, "Window", "SDL_GL_SetAttribute %d error: %s\n", attr, SDL_GetError());
        DLB_ASSERT(!"sdl_gl_attrib: error");
    }
}

static void ta_init_sdl(ta_window *window, bool fullscreen)
{
    ta_log_write(&tg_debug_log, "Window", "SDL_Init...\n");
    int sdl_err = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);  // SDL_INIT_TIMER
    if (sdl_err < 0) {
        ta_log_write(&tg_debug_log, "Window", "SDL_Init error: %s\n", SDL_GetError());
        DLB_ASSERT(!"ta_init_sdl: SDL_Init failed");
    }

    ta_log_write(&tg_debug_log, "Window", "Setting SDL GL attributes\n");
    sdl_gl_attrib(SDL_GL_RED_SIZE, 8);
    sdl_gl_attrib(SDL_GL_GREEN_SIZE, 8);
    sdl_gl_attrib(SDL_GL_BLUE_SIZE, 8);
    sdl_gl_attrib(SDL_GL_ALPHA_SIZE, 8);
    sdl_gl_attrib(SDL_GL_DOUBLEBUFFER, 1);
    sdl_gl_attrib(SDL_GL_DEPTH_SIZE, 24);
    sdl_gl_attrib(SDL_GL_STENCIL_SIZE, 8);
    // Anti-aliasing
    sdl_gl_attrib(SDL_GL_MULTISAMPLESAMPLES, 4);
    //sdl_gl_attrib(SDL_GL_MULTISAMPLEBUFFERS, 1);
    //sdl_gl_attrib(SDL_GL_ACCELERATED_VISUAL, 1);
    int context_flags = SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG;
#if _DEBUG
    context_flags |= SDL_GL_CONTEXT_DEBUG_FLAG;
#endif
    sdl_gl_attrib(SDL_GL_CONTEXT_FLAGS, context_flags);
    sdl_gl_attrib(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    sdl_gl_attrib(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    sdl_gl_attrib(SDL_GL_CONTEXT_MINOR_VERSION, 2);

    // TODO: Make fullscreen a borderless window because people say vsync
    //       doesn't work in fullscreen (can we confirm this?)
    // Create window
    ta_log_write(&tg_debug_log, "Window", "SDL_CreateWindow...\n");
    u32 flags = SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI;
    if (fullscreen) {
        window->sdl_window = SDL_CreateWindow("Talaria", SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED, 0, 0, flags | SDL_WINDOW_FULLSCREEN_DESKTOP);
    } else if (window->size.w && window->size.h) {
        window->sdl_window = SDL_CreateWindow("Talaria", SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED, window->size.w, window->size.h,
            flags | SDL_WINDOW_RESIZABLE | SDL_WINDOW_MAXIMIZED);
    }
    if (window->sdl_window == NULL) {
        ta_log_write(&tg_debug_log, "Window", "SDL_CreateWindow error: %s\n", SDL_GetError());
        DLB_ASSERT(!"ta_init_sdl: SDL_CreateWindow failed");
    }

    // Create GL context
    ta_log_write(&tg_debug_log, "Window", "SDL_GL_CreateContext...\n");
    window->gl_context = SDL_GL_CreateContext(window->sdl_window);
    if (window->gl_context == NULL) {
        ta_log_write(&tg_debug_log, "Window", "SDL_GL_CreateContext error: %s\n", SDL_GetError());
        DLB_ASSERT(!"ta_init_sdl: SDL_GL_CreateContext failed");
    }

    // Get actual window size
    ta_log_write(&tg_debug_log, "Window", "Get window size / swap interval\n");
    if (fullscreen) {
        SDL_GetWindowSize(window->sdl_window, &window->size.w, &window->size.h);
    }

    // Log default VSync state
    //SDL_GL_SetSwapInterval(1);
    int swap = SDL_GL_GetSwapInterval();
    ta_log_write(&tg_debug_log, "Window", "w: %d, h: %d, vsync: %s\n",
        window->size.w, window->size.h, swap ? "on" : "off");
}

void ta_window_init(ta_window *window, int w, int h, bool fullscreen)
{
    window->size.w = w;
    window->size.h = h;
    ta_init_sdl(window, fullscreen);
    DLB_ASSERT(fullscreen || (window->size.w == w && window->size.h == h));
    window->aspect = (float)window->size.w / window->size.h;
}

void ta_window_free(ta_window *window)
{
    SDL_GL_DeleteContext(window->gl_context);
    SDL_DestroyWindow(window->sdl_window);
    SDL_Quit();
}

ta_size ta_window_size(ta_window *window)
{
    return window->size;
}

int ta_window_width(ta_window *window)
{
    return window->size.w;
}

int ta_window_height(ta_window *window)
{
    return window->size.h;
}

float ta_window_aspect(ta_window *window)
{
    return (float)window->size.w / window->size.h;
}

SDL_Window *ta_window_sdl(ta_window *window)
{
    return window->sdl_window;
}

void ta_window_swap(ta_window *window)
{
    SDL_GL_SwapWindow(window->sdl_window);
}

void ta_window_event(ta_window *window, ta_event *event)
{
    switch (event->type) {
        case TA_EVENT_WINDOW_RESIZE: {
            window->size.w = event->data.window_resize.width;
            window->size.h = event->data.window_resize.height;
            window->aspect = (float)window->size.w / window->size.h;

            // Update all cameras to new aspect ratio
            dlb_vec_each(ta_camera *, camera,
                tg_game.scene->resource_data[RES_COMP_CAMERA])
            {
                if (!camera->ortho) {
                    ta_camera_recalc_projection(camera);
                }
            }
            break;
        }
    }
}

int ta_window_msgbox(ta_window *window, u32 flags, const char *title,
    const char *message)
{
    return SDL_ShowSimpleMessageBox(flags, title, message,
        window ? window->sdl_window : 0);
}