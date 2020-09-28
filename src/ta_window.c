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

    // TODO: Find a way to do this without making the font look like shit
    // Anti-aliasing
    //sdl_gl_attrib(SDL_GL_MULTISAMPLESAMPLES, 16);
    //sdl_gl_attrib(SDL_GL_MULTISAMPLEBUFFERS, 1);
    //sdl_gl_attrib(SDL_GL_ACCELERATED_VISUAL, 1);  // NOTE: Set to 1 to force hardware acceleration

    sdl_gl_attrib(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    sdl_gl_attrib(SDL_GL_CONTEXT_MAJOR_VERSION, gl_major);
    sdl_gl_attrib(SDL_GL_CONTEXT_MINOR_VERSION, gl_minor);
//    int context_flags = SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG;
//#if _DEBUG
//    context_flags |= SDL_GL_CONTEXT_DEBUG_FLAG;
//#endif
//    sdl_gl_attrib(SDL_GL_CONTEXT_FLAGS, context_flags);


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
#if !defined(_DEBUG)
    ta_window_set_vsync(window, false);
#endif
    window->vsync = SDL_GL_GetSwapInterval();
    ta_log_write(&tg_debug_log, SRC_WINDOW, "w: %d, h: %d, vsync: %s\n", window->width, window->height,
        window->vsync ? "on" : "off");

    // TODO: Separate render thread and have the data loading thread update a progress that the render thread can draw.
    // Draw something as soon as humanly possible (just a color for now to get rid of the default white)
    //glClearColor(0.1f, 0.15f, 0.3f, 1.0f);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    ta_window_swap(tg_window);

    ta_log_write(&tg_debug_log, SRC_WINDOW, "Initializing OpenGL state...\n");
#if _DEBUG
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

#if 1
    GLint data_int = 0;

#define GLGET_LOG_INT(pname) \
    data_int = 0; \
    glGetIntegerv(pname, &data_int); \
    ta_log_write(&tg_debug_log, SRC_WINDOW, "%s: %d\n", #pname, data_int);

    // Max # of textures in a single texture array (at least 256 in OpenGL 3.0, query if need more dynamically)
    GLGET_LOG_INT(GL_MAX_ARRAY_TEXTURE_LAYERS);

    GLGET_LOG_INT(GL_MAX_FRAGMENT_INPUT_COMPONENTS);

    // The value gives the maximum number of texels allowed in the texel array of a texture buffer object. Value must be at least 65536.
    GLGET_LOG_INT(GL_MAX_TEXTURE_BUFFER_SIZE);

    // The maximum, absolute value of the texture level-of-detail bias. The value must be at least 2.0.
    GLGET_LOG_INT(GL_MAX_TEXTURE_LOD_BIAS);

    // The value gives a rough estimate of the largest texture that the GL can handle. The value must be at least 1024. Use a proxy texture target such as GL_PROXY_TEXTURE_1D or GL_PROXY_TEXTURE_2D to determine if a texture is too large. See glTexImage1D and glTexImage2D.
    GLGET_LOG_INT(GL_MAX_TEXTURE_SIZE);

    // The maximum supported texture image units that can be used to access texture maps from the vertex shader. The value may be at least 16. See glActiveTexture.
    GLGET_LOG_INT(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS);
    GLGET_LOG_INT(GL_MAX_GEOMETRY_TEXTURE_IMAGE_UNITS);
    GLGET_LOG_INT(GL_MAX_TEXTURE_IMAGE_UNITS);           // NOTE: Fragment shader limit
    GLGET_LOG_INT(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS);  // NOTE: All shaders, combined limit

    GLGET_LOG_INT(GL_MAX_VERTEX_UNIFORM_BLOCKS);
    GLGET_LOG_INT(GL_MAX_GEOMETRY_UNIFORM_BLOCKS);
    GLGET_LOG_INT(GL_MAX_FRAGMENT_UNIFORM_BLOCKS);
    GLGET_LOG_INT(GL_MAX_COMBINED_UNIFORM_BLOCKS);

    // The maximum number of uniform buffer binding points on the context, which must be at least 36.
    GLGET_LOG_INT(GL_MAX_UNIFORM_BUFFER_BINDINGS);

    // The maximum size in basic machine units of a uniform block, which must be at least 16384.
    GLGET_LOG_INT(GL_MAX_UNIFORM_BLOCK_SIZE);

    // The maximum number of explicitly assignable uniform locations, which must be at least 1024.
    //GLGET_LOG_INT(GL_MAX_UNIFORM_LOCATIONS);

    // The number components for varying variables, which must be at least 60.
    GLGET_LOG_INT(GL_MAX_VARYING_COMPONENTS);

    // The number 4-vectors for varying variables, which is equal to the value of GL_MAX_VARYING_COMPONENTS and must be at least 15.
    //GLGET_LOG_INT(GL_MAX_VARYING_VECTORS);

    // The maximum number of interpolators available for processing varying variables used by vertex and fragment shaders. This value represents the number of individual floating-point values that can be interpolated; varying variables declared as vectors, matrices, and arrays will all consume multiple interpolators. The value must be at least 32.
    GLGET_LOG_INT(GL_MAX_VARYING_FLOATS);

    // The maximum number of atomic counters available to vertex shaders.
    //GLGET_LOG_INT(GL_MAX_VERTEX_ATOMIC_COUNTERS);

    // The maximum number of 4-component generic vertex attributes accessible to a vertex shader. The value must be at least 16. See glVertexAttrib.
    GLGET_LOG_INT(GL_MAX_VERTEX_ATTRIBS);

    // The maximum number of active shader storage blocks that may be accessed by a vertex shader.
    //GLGET_LOG_INT(GL_MAX_VERTEX_SHADER_STORAGE_BLOCKS);

    // NOTE: I'm not really sure what this means...
    // The number of words for shader uniform variables in all uniform blocks (including default). The value must be at least 1.
    GLGET_LOG_INT(GL_MAX_COMBINED_VERTEX_UNIFORM_COMPONENTS);
    GLGET_LOG_INT(GL_MAX_COMBINED_GEOMETRY_UNIFORM_COMPONENTS);
    GLGET_LOG_INT(GL_MAX_COMBINED_FRAGMENT_UNIFORM_COMPONENTS);

    // The maximum number of individual floating-point, integer, or boolean values that can be held in uniform variable storage for a shader. The value must be at least 1024. See glUniform.
    GLGET_LOG_INT(GL_MAX_VERTEX_UNIFORM_COMPONENTS);
    GLGET_LOG_INT(GL_MAX_GEOMETRY_UNIFORM_COMPONENTS);
    GLGET_LOG_INT(GL_MAX_FRAGMENT_UNIFORM_COMPONENTS);

    GLGET_LOG_INT(GL_MAX_ARRAY_TEXTURE_LAYERS);

    // The maximum number of 4-vectors that may be held in uniform variable storage for the vertex shader. The value of GL_MAX_VERTEX_UNIFORM_VECTORS is equal to the value of GL_MAX_VERTEX_UNIFORM_COMPONENTS and must be at least 256.
    //GLGET_LOG_INT(GL_MAX_VERTEX_UNIFORM_VECTORS);

    // The maximum number of components of output written by a vertex shader, which must be at least 64.
    GLGET_LOG_INT(GL_MAX_VERTEX_OUTPUT_COMPONENTS);

#undef GLGET_LOG_INT
#endif

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
