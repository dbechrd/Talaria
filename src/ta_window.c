#include "ta_window.h"
#include "ta_log.h"
#include "ta_event.h"
#include "dlb_types.h"
#include "SDL/SDL.h"

static SDL_Window *window;
static SDL_GLContext gl_context;

ta_window tg_window;

static void sdl_gl_attrib(SDL_GLattr attr, int value)
{
    int sdl_err = SDL_GL_SetAttribute(attr, value);
    if (sdl_err < 0)
    {
        ta_log_write(tg_debug_log, "[Window] SDL_GL_SetAttribute %d error: %s\n", attr, SDL_GetError());
        DLB_ASSERT(!"sdl_gl_attrib: error");
    }
}

static void ta_init_sdl(int *w, int *h, bool fullscreen)
{
    ta_log_write(tg_debug_log, "[Window] SDL_Init...\n");
    int sdl_err = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);  // SDL_INIT_TIMER
    if (sdl_err < 0) {
        ta_log_write(tg_debug_log, "[Window] SDL_Init error: %s\n", SDL_GetError());
        DLB_ASSERT(!"ta_init_sdl: SDL_Init failed");
    }
    ta_log_write(tg_debug_log, "[Window] Success\n");

    sdl_gl_attrib(SDL_GL_RED_SIZE, 8);
    sdl_gl_attrib(SDL_GL_GREEN_SIZE, 8);
    sdl_gl_attrib(SDL_GL_BLUE_SIZE, 8);
    sdl_gl_attrib(SDL_GL_ALPHA_SIZE, 8);
    sdl_gl_attrib(SDL_GL_DOUBLEBUFFER, 1);
    sdl_gl_attrib(SDL_GL_DEPTH_SIZE, 24);
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

    // Create window
    ta_log_write(tg_debug_log, "[Window] SDL_CreateWindow...\n");
    u32 flags = SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI;
    if (fullscreen) {
		window = SDL_CreateWindow("Talaria", SDL_WINDOWPOS_CENTERED,
			SDL_WINDOWPOS_CENTERED, 0, 0, flags | SDL_WINDOW_FULLSCREEN_DESKTOP);
	} else if (w && h) {
		window = SDL_CreateWindow("Talaria", SDL_WINDOWPOS_CENTERED,
			SDL_WINDOWPOS_CENTERED, *w, *h, flags | SDL_WINDOW_RESIZABLE | SDL_WINDOW_MAXIMIZED);
	}
    if (window == NULL) {
        ta_log_write(tg_debug_log, "[Window] SDL_CreateWindow error: %s\n", SDL_GetError());
        DLB_ASSERT(!"ta_init_sdl: SDL_CreateWindow failed");
    }
	ta_log_write(tg_debug_log, "[Window] Success\n");

    // Create GL context
    ta_log_write(tg_debug_log, "[Window] SDL_GL_CreateContext...\n");
    gl_context = SDL_GL_CreateContext(window);
    if (gl_context == NULL) {
        ta_log_write(tg_debug_log, "[Window] SDL_GL_CreateContext error: %s\n", SDL_GetError());
        DLB_ASSERT(!"ta_init_sdl: SDL_GL_CreateContext failed");
    }
    ta_log_write(tg_debug_log, "[Window] Success\n");

    // Get actual window size
    if (fullscreen && w && h) {
        SDL_GetWindowSize(window, w, h);
    }

    // Log default VSync state
	//SDL_GL_SetSwapInterval(0);
    int swap = SDL_GL_GetSwapInterval();
    ta_log_write(tg_debug_log, "[Window] vsync: %s\n", (swap) ? "enabled" : "disabled");
}

void ta_window_init(int w, int h, bool fullscreen)
{
    ta_log_write(tg_debug_log, "[Window] Initializing window\n");
    tg_window.rect.w = w;
	tg_window.rect.h = h;
    ta_init_sdl(&tg_window.rect.w, &tg_window.rect.h, fullscreen);
    DLB_ASSERT(fullscreen || (tg_window.rect.w == w && tg_window.rect.h == h));
    tg_window.aspect = (float)tg_window.rect.w / tg_window.rect.h;
    ta_log_write(tg_debug_log, "[Window] Window initialized\n");
}

void ta_window_free()
{
    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void ta_window_swap()
{
    SDL_GL_SwapWindow(window);
}

void ta_window_events()
{
    ta_event event;
    while (ta_event_pop(&event, TA_EVENT_QUEUE_WINDOW)) {
        switch (event.type) {
            case TA_EVENT_WINDOW_RESIZE: {
                tg_window.rect.w = event.data.window_resize.width;
                tg_window.rect.h = event.data.window_resize.height;
                tg_window.aspect = (float)tg_window.rect.w / tg_window.rect.h;
                break;
            } default: {
                DLB_ASSERT(!"Unhandled event type");
            }
        }
    }
}