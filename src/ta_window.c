#include "ta_window.h"
#include "ta_log.h"
#include "ta_event.h"
#include "dlb/dlb_types.h"
#include "dlb/dlb_memory.h"
#include "dlb/dlb_vector.h"
#include "misc/glad.h"
#include <math.h>

typedef struct ta_window {
    int width;
    int height;
    ta_rect restore;
    bool vsync;
    bool fullscreen;
    SDL_Window *sdl_window;
    SDL_GLContext *sdl_gl_context;
    SDL_Cursor *sdl_cursor_active;
    SDL_Cursor *sdl_cursor_requested;
} ta_window;
ta_window window__internal;
ta_window *tg_window = &window__internal;

static SDL_Cursor *window_cursor_arrow;    // normal mouse pointer
static SDL_Cursor *window_cursor_hresize;  // left/right arrow "<->" cursor
static SDL_Cursor *window_cursor_ibeam;    // text edit ibeam "I" cursor

//static void APIENTRY ta_window_gl_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
//    const GLchar *message, const void *userParam)
//{
//    UNUSED(length);
//    UNUSED(userParam);
//
//    char *sourceStr, *typeStr, *severityStr;
//
//    switch (source) {
//        case GL_DEBUG_SOURCE_API:
//            sourceStr = "API            ";
//            break;
//        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
//            sourceStr = "WINDOW SYSTEM  ";
//            break;
//        case GL_DEBUG_SOURCE_SHADER_COMPILER:
//            sourceStr = "SHADER COMPILER";
//            break;
//        case GL_DEBUG_SOURCE_THIRD_PARTY:
//            sourceStr = "THIRD PARTY    ";
//            break;
//        case GL_DEBUG_SOURCE_APPLICATION:
//            sourceStr = "APPLICATION    ";
//            break;
//        case GL_DEBUG_SOURCE_OTHER:
//            sourceStr = "OTHER          ";
//            return;
//            break;
//        default:
//            sourceStr = "???????????????";
//            break;
//    }
//
//    switch (type) {
//        case GL_DEBUG_TYPE_ERROR:
//            typeStr = "ERROR";
//            break;
//        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
//            typeStr = "DEPRC";
//            break;
//        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
//            typeStr = "UNDEF";
//            break;
//        case GL_DEBUG_TYPE_PORTABILITY:
//            typeStr = "PORT ";
//            break;
//        case GL_DEBUG_TYPE_PERFORMANCE:
//            typeStr = "PERF ";
//            break;
//        case GL_DEBUG_TYPE_OTHER:
//            typeStr = "OTHER";
//            return;
//            break;
//        default:
//            typeStr = "?????";
//            break;
//    }
//
//    switch (severity) {
//        case GL_DEBUG_SEVERITY_LOW:
//            severityStr = "LOW ";
//            break;
//        case GL_DEBUG_SEVERITY_MEDIUM:
//            severityStr = "MED ";
//            break;
//        case GL_DEBUG_SEVERITY_HIGH:
//            severityStr = "HIGH";
//            break;
//        default:
//            severityStr = "????";
//            break;
//    }
//
//    static u32 gl_errors = 0;
//    if (gl_errors < 10)
//    {
//        ta_log_write(&tg_debug_log, SRC_OPENGL,
//            "[source: %s][type: %s][severity: %s][id: %d] %s\n", sourceStr,
//            typeStr, severityStr, id, message);
//        gl_errors++;
//    }
//}

static void sdl_gl_attrib(SDL_GLattr attr, int value)
{
    int sdl_err = SDL_GL_SetAttribute(attr, value);
    if (sdl_err < 0)
    {
        ta_log_write(&tg_debug_log, SRC_WINDOW, "SDL_GL_SetAttribute %d error: %s\n", attr, SDL_GetError());
        DLB_ASSERT(!"sdl_gl_attrib: error");
    }
}

void ta_window_init(ta_window *window, int w, int h, bool fullscreen)
{
    // Make sure we don't have just width or just height
    DLB_ASSERT((w && h) || !(w || h));

    static const int gl_major = 3;
    static const int gl_minor = 2;

    ta_log_write(&tg_debug_log, SRC_WINDOW, "Setting SDL GL attributes\n");
    sdl_gl_attrib(SDL_GL_RED_SIZE, 8);
    sdl_gl_attrib(SDL_GL_GREEN_SIZE, 8);
    sdl_gl_attrib(SDL_GL_BLUE_SIZE, 8);
    sdl_gl_attrib(SDL_GL_ALPHA_SIZE, 8);
    sdl_gl_attrib(SDL_GL_DOUBLEBUFFER, 1);
    sdl_gl_attrib(SDL_GL_DEPTH_SIZE, 24);
    sdl_gl_attrib(SDL_GL_STENCIL_SIZE, 8);

    // Anti-aliasing
    sdl_gl_attrib(SDL_GL_MULTISAMPLESAMPLES, 16);
    sdl_gl_attrib(SDL_GL_MULTISAMPLEBUFFERS, 1);
    //sdl_gl_attrib(SDL_GL_ACCELERATED_VISUAL, 1);  // NOTE: Set to 1 to force hardware acceleration

    sdl_gl_attrib(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    sdl_gl_attrib(SDL_GL_CONTEXT_MAJOR_VERSION, gl_major);
    sdl_gl_attrib(SDL_GL_CONTEXT_MINOR_VERSION, gl_minor);
    int context_flags = SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG;
#if _DEBUG
    context_flags |= SDL_GL_CONTEXT_DEBUG_FLAG;
#endif
    sdl_gl_attrib(SDL_GL_CONTEXT_FLAGS, context_flags);


    // TODO: Make fullscreen a borderless window because people say vsync
    //       doesn't work in fullscreen (can we confirm this?)
    // Create window
    ta_log_write(&tg_debug_log, SRC_WINDOW, "SDL_CreateWindow...\n");
    u32 flags = SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI;
    if (fullscreen) {
        SDL_Rect rect = { 0 };
        SDL_GetDisplayBounds(0, &rect);
        window->width = rect.w;
        window->height = rect.h;
#if 0
        flags |= SDL_WINDOW_BORDERLESS;
#else
        flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
#endif
    } else {
        window->width = w;
        window->height = h;
        // Allowing resizable doesn't set the resolution properly
        flags |= SDL_WINDOW_RESIZABLE;
    }
    window->sdl_window = SDL_CreateWindow("Talaria", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, window->width,
        window->height, flags);
    if (window->sdl_window == NULL) {
        ta_log_write(&tg_debug_log, SRC_WINDOW, "SDL_CreateWindow error: %s\n", SDL_GetError());
        DLB_ASSERT(!"ta_init_sdl: SDL_CreateWindow failed");
        // TODO: Return error code, handle in caller
        return;
    }

    // Create GL context
    ta_log_write(&tg_debug_log, SRC_WINDOW, "SDL_GL_CreateContext...\n");
    window->sdl_gl_context = SDL_GL_CreateContext(window->sdl_window);
    if (window->sdl_gl_context == NULL) {
        ta_log_write(&tg_debug_log, SRC_WINDOW, "SDL_GL_CreateContext error: %s\n", SDL_GetError());
        DLB_ASSERT(!"ta_init_sdl: SDL_GL_CreateContext failed");
        // TODO: Return error code, handle in caller
        return;
    }

    ta_log_write(&tg_debug_log, SRC_WINDOW, "gladLoadGL...\n");
    if (!gladLoadGL()) {
        ta_log_write(&tg_debug_log, SRC_WINDOW, "gladLoadGL failed\n");
        DLB_ASSERT(!"ta_window_init: failed to init GLAD");
    }

    // Get actual window size
    ta_log_write(&tg_debug_log, SRC_WINDOW, "Get window size / swap interval\n");
    if (fullscreen) {
        SDL_GetWindowSize(window->sdl_window, &window->width, &window->height);
    }

    // Log default VSync state
    window->vsync = SDL_GL_GetSwapInterval();
    ta_log_write(&tg_debug_log, SRC_WINDOW, "w: %d, h: %d, vsync: %s\n", window->width, window->height,
        window->vsync ? "on" : "off");

    // Draw something as soon as humanly possible (just a dark gray for now to get rid of the default white)
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    ta_window_swap(tg_window);

    ta_log_write(&tg_debug_log, SRC_WINDOW, "Initializing OpenGL state...\n");
#if _DEBUG
    {
        GLint gl_max_vertex_attribs;
        glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &gl_max_vertex_attribs);
        ta_log_write(&tg_debug_log, SRC_WINDOW, "GL_MAX_VERTEX_ATTRIBS = %d\n", gl_max_vertex_attribs);
    }

    // TODO: Use newer version of GL to get debug messages, or find an old way to do something similar
    //if (glDebugMessageCallback != NULL)
    //{th
    //    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    //    glDebugMessageCallback(ta_window_gl_callback, NULL);
    //    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, 0, true);
    //    ta_log_write(&tg_debug_log, SRC_WINDOW, "Registered glDebugMessageCallback\n");
    //}
    //else
    //{
    //    ta_log_write(&tg_debug_log, SRC_WINDOW, "glDebugMessageCallback not available\n");
    //}
#endif

    ta_log_write(&tg_debug_log, SRC_WINDOW, "OpenGL: %s\n", glGetString(GL_VERSION));
    ta_log_write(&tg_debug_log, SRC_WINDOW, "GLSL: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));

    //glClearColor(0.7f, 0.9f, 1.0f, 1.0f);

    // Stencil buffer
    //glStencilMask(0x00);
    //glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
    //glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

    GLint stencilSize = 0;
    glGetFramebufferAttachmentParameteriv(GL_DRAW_FRAMEBUFFER, GL_STENCIL, GL_FRAMEBUFFER_ATTACHMENT_STENCIL_SIZE,
        &stencilSize);
    DLB_ASSERT(stencilSize == 8);

    // Depth buffer
    glDepthFunc(GL_LEQUAL);    // Default GL_LESS (LEQUAL required for skybox)
    glEnable(GL_DEPTH_TEST);   // Default off
    glEnable(GL_CULL_FACE);    // Backface culling
    glCullFace(GL_BACK);

    // Alpha-blending
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);

    // Multi-sampling
    //glEnable(GL_MULTISAMPLE);

    // Gamma correction
    //glEnable(GL_FRAMEBUFFER_SRGB);

    // Seamless filtering across cubemap seams
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    ta_log_write(&tg_debug_log, SRC_WINDOW, "Loading cursors\n");
    window_cursor_arrow = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
    window_cursor_hresize = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEWE);
    window_cursor_ibeam = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_IBEAM);
    window->sdl_cursor_active = window_cursor_arrow;
    window->sdl_cursor_requested = window_cursor_arrow;
}

//======================================================================================================================
// Proper OOP programming comment header (see below)
//======================================================================================================================
// Summary         : This function frees the window
// Author          : Dan Bechard (a.k.a. dandymcgee@twitch.tv)
// Date            : 2020-05-30 02:23:07 AM PST
// Purpose         : Frees the window
// Argument 1      : ta_window *window (this is the window to free)
// Return value    : void, this function does not return anything
// Errors          : I was too lazy, so I didn't do error handling
// Exceptions      : I don't use exceptions, this is not C++
// See also        : ta_window_init, ta_window_etc...
// Special thanks  : <professor name>, RatcheT2497, JohnnyGinard, electrondefuser, java_is_best
//======================================================================================================================
void ta_window_free(ta_window *window)
{
    SDL_GL_DeleteContext(window->sdl_gl_context);
    SDL_DestroyWindow(window->sdl_window);
}

int ta_window_get_width(ta_window *window)
{
    return window->width;
}

int ta_window_get_height(ta_window *window)
{
    return window->height;
}

void ta_window_get_size(ta_window *window, int *w, int *h)
{
    *w = window->width;
    *h = window->height;
}
void ta_window_set_size(ta_window *window, int w, int h)
{
    // HACK: Use ta_window_event
    window->width = w;
    window->height = h;
}
void ta_window_get_restore_rect(ta_window *window, ta_rect *restore)
{
    *restore = window->restore;
}

float ta_window_aspect(ta_window *window)
{
    return (float)window->width / window->height;
}

void ta_window_get_vsync(ta_window *window, bool *vsync)
{
    *vsync = window->vsync;
}
void ta_window_set_vsync(ta_window *window, bool vsync)
{
    if (window->vsync != vsync) {
        SDL_GL_SetSwapInterval(vsync ? 1 : 0);
        window->vsync = vsync;
    }
}

#if 0
// Shmo @ StackOverflow
// https://stackoverflow.com/a/31526753/770230
static GLFWmonitor* window_current_monitor(ta_window *window)
{
    int nmonitors, i;
    int wx, wy, ww, wh;
    int mx, my, mw, mh;
    int overlap, bestoverlap;
    GLFWmonitor *bestmonitor;
    GLFWmonitor **monitors;
    const GLFWvidmode *mode;

    bestoverlap = 0;
    bestmonitor = NULL;

    glfwGetWindowPos(window->sdl_window, &wx, &wy);
    glfwGetWindowSize(window->sdl_window, &ww, &wh);
    monitors = glfwGetMonitors(&nmonitors);

    for (i = 0; i < nmonitors; i++) {
        mode = glfwGetVideoMode(monitors[i]);
        glfwGetMonitorPos(monitors[i], &mx, &my);
        mw = mode->width;
        mh = mode->height;

        overlap =
            MAX(0, MIN(wx + ww, mx + mw) - MAX(wx, mx)) *
            MAX(0, MIN(wy + wh, my + mh) - MAX(wy, my));

        if (bestoverlap < overlap) {
            bestoverlap = overlap;
            bestmonitor = monitors[i];
        }
    }

    return bestmonitor;
}
#endif
void ta_window_get_fullscreen(ta_window *window, bool *fullscreen)
{
    *fullscreen = window->fullscreen;
}
void ta_window_set_fullscreen(ta_window *window, bool fullscreen)
{
    if (fullscreen == window->fullscreen)
        return;

    if (fullscreen) {
        // TODO: I don't know which one of these is better..
        SDL_SetWindowFullscreen(window->sdl_window, SDL_WINDOW_FULLSCREEN);
        //SDL_SetWindowFullscreen(window->sdl_window, SDL_WINDOW_FULLSCREEN_DESKTOP);
        window->fullscreen = true;
    } else {
        SDL_SetWindowFullscreen(window->sdl_window, 0);
        // TODO: I don't know if setting vsync after changing fullscreen mode is necessary in SDL like it is with GLFW
        SDL_GL_SetSwapInterval(window->vsync);
        window->fullscreen = false;
    }
}

void ta_window_request_cursor(ta_window *window, ta_cursor_type cursor_type)
{
    SDL_Cursor *cursor = window_cursor_arrow;
    switch (cursor_type) {
        case TA_CURSOR_ARROW: {
            cursor = window_cursor_arrow;
            break;
        } case TA_CURSOR_HRESIZE: {
            cursor = window_cursor_hresize;
            break;
        } case TA_CURSOR_IBEAM: {
            cursor = window_cursor_ibeam;
            break;
        } default: {
            DLB_ASSERT(!"Need to handle this cursor type");
        }
    };
    window->sdl_cursor_requested = cursor;
}
void ta_window_set_cursor_pos(ta_window *window, int x, int y)
{
    SDL_WarpMouseInWindow(window->sdl_window, x, y);
}
void ta_window_get_cursor_pos(ta_window *window, int *x, int *y)
{
    UNUSED(window);
    SDL_GetMouseState(x, y);
}
void ta_window_set_mouse_captured(ta_window *window, bool captured)
{
    UNUSED(window);
    SDL_SetRelativeMouseMode(captured);
}

void ta_window_update_cursor(ta_window *window)
{
    if (window->sdl_cursor_requested != window->sdl_cursor_active) {
        SDL_SetCursor(window->sdl_cursor_requested);
        window->sdl_cursor_active = window->sdl_cursor_requested;
    }
}
void ta_window_swap(ta_window *window)
{
    SDL_GL_SwapWindow(window->sdl_window);
}
int ta_window_msgbox(ta_window *window, u32 flags, const char *title, const char *message)
{
    return SDL_ShowSimpleMessageBox(flags, title, message, window ? window->sdl_window : 0);
}
