#include "ta_window.h"
#include "ta_log.h"
#include "ta_event.h"
#include "dlb/dlb_types.h"
#include "dlb/dlb_memory.h"
#include "dlb/dlb_vector.h"
#include "GLFW/glfw3.h"
#include "misc/glad.h"
#include <math.h>

typedef struct ta_window {
    int width;
    int height;
    ta_rect restore;
    bool vsync;
    bool fullscreen;
    GLFWwindow *glfw_window;
    GLFWcursor *glfw_active_cursor;
    GLFWcursor *glfw_requested_cursor;
} ta_window;
ta_window window__internal;
ta_window *tg_window = &window__internal;

static GLFWcursor *window_cursor_arrow;    // normal mouse pointer
static GLFWcursor *window_cursor_hresize;  // left/right arrow "<->" cursor
static GLFWcursor *window_cursor_ibeam;    // text edit ibeam "I" cursor

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

static void window_glfw_close(GLFWwindow *glfw_window)
{
    UNUSED(glfw_window);

    //if (unsaved_changes)
    //    glfwSetWindowShouldClose(glfw_window, GLFW_FALSE);

    ta_event event = { 0 };
    event.type = GAME_EVENT_SHUTDOWN;
    ta_event_push(&event);
}
static void window_glfw_framebuffer_resize(GLFWwindow *glfw_window, int w, int h)
{
    ta_window *window = glfwGetWindowUserPointer(glfw_window);
    window->width = w;
    window->height = h;

    ta_event event = { 0 };
    event.type = WINDOW_EVENT_RESIZE;
    event.data.window_resize.width = window->width;
    event.data.window_resize.height = window->height;
    ta_event_push(&event);
}
static void window_glfw_key(GLFWwindow *glfw_window, int key, int scancode, int action, int mods)
{
    UNUSED(glfw_window);
    static int action_to_event[] = {
        [GLFW_RELEASE] = INPUT_EVENT_KEY_RELEASE,
        [GLFW_PRESS  ] = INPUT_EVENT_KEY_PRESS,
        [GLFW_REPEAT ] = INPUT_EVENT_KEY_REPEAT,
    };

    ta_event event = { 0 };
    event.type = action_to_event[action];
    if (event.type) {
        event.data.key.key = key;
        event.data.key.scancode = scancode;
        event.data.key.mods = mods;
        event.data.key.repeat = (action == GLFW_REPEAT);
        ta_key_event(&event);
        ta_event_push(&event);
    }
}
static void window_glfw_codepoint(GLFWwindow *glfw_window, unsigned int codepoint)
{
    UNUSED(glfw_window);
    ta_event event = { 0 };
    event.type = INPUT_EVENT_TEXT_INPUT;
    event.data.text_input.codepoint = codepoint;
    ta_event_push(&event);
}
static void window_glfw_mouse_button(GLFWwindow *glfw_window, int button, int action, int mods)
{
    UNUSED(glfw_window);
    static int action_to_event[] = {
        [GLFW_RELEASE] = INPUT_EVENT_KEY_RELEASE,
        [GLFW_PRESS  ] = INPUT_EVENT_KEY_PRESS,
        [GLFW_REPEAT ] = INPUT_EVENT_KEY_REPEAT,
    };
    static int button_to_key[] = {
        [GLFW_MOUSE_BUTTON_LEFT  ] = GLFW_KEY_MOUSE_LEFT,
        [GLFW_MOUSE_BUTTON_RIGHT ] = GLFW_KEY_MOUSE_RIGHT,
        [GLFW_MOUSE_BUTTON_MIDDLE] = GLFW_KEY_MOUSE_MIDDLE,
    };

    ta_event event = { 0 };
    event.type = action_to_event[action];
    if (event.type) {
        event.data.key.key = button_to_key[button];
        event.data.key.scancode = -1;
        event.data.key.mods = mods;
        event.data.key.repeat = (action == GLFW_REPEAT);
        ta_key_event(&event);
        ta_event_push(&event);
    }
}

static void window_glfw_mouse_move(GLFWwindow *glfw_window, double xpos, double ypos)
{
    UNUSED(glfw_window);
    ta_event event = { 0 };
    event.type = INPUT_EVENT_MOUSE_MOVE;
    event.data.mouse_move.x = (int)floor(xpos);
    event.data.mouse_move.y = (int)floor(ypos);
    event.data.mouse_move.dx = event.data.mouse_move.x - ta_mouse_x();
    event.data.mouse_move.dy = event.data.mouse_move.y - ta_mouse_y();
    ta_mouse_event(&event);
    ta_event_push(&event);
}
static void window_glfw_scroll(GLFWwindow *glfw_window, double xoffset, double yoffset)
{
    UNUSED(glfw_window);
    ta_event event = { 0 };
    event.type = INPUT_EVENT_MOUSE_SCROLL;
    event.data.mouse_scroll.x = (int)floor(xoffset);
    event.data.mouse_scroll.y = (int)floor(yoffset);
    ta_mouse_event(&event);
    ta_event_push(&event);
}
static void window_glfw_dragndrop(GLFWwindow *glfw_window, int count, const char **paths)
{
    UNUSED(glfw_window);
    UNUSED(count);
    UNUSED(paths);
    //int i;
    //for (i = 0;  i < count;  i++)
    //    handle_dropped_file(paths[i]);
}

void ta_window_init(ta_window *window, int w, int h, bool fullscreen)
{
    // Make sure we don't have just width or just height
    DLB_ASSERT((w && h) || !(w || h));

    static const int gl_major = 3;
    static const int gl_minor = 2;

    // Create window
    GLFWmonitor *monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode *mode = glfwGetVideoMode(monitor);
    glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
    glfwWindowHint(GLFW_RED_BITS, mode->redBits); // 8;
    glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits); // 8;
    glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits); // 8;
    glfwWindowHint(GLFW_ALPHA_BITS, 8);
    glfwWindowHint(GLFW_DEPTH_BITS, 24);
    glfwWindowHint(GLFW_STENCIL_BITS, 8);
    glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);
    // TODO: Disable if doing multi-sampling in a post-processing shader
    glfwWindowHint(GLFW_SAMPLES, 16);
    //glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, gl_major);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, gl_minor);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#if _DEBUG
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#else
    // TODO: Disable GL errors in release mode for performance?
    //glfwWindowHint(GLFW_CONTEXT_NO_ERROR, GLFW_TRUE);
#endif

    window->fullscreen = fullscreen;
    if (!fullscreen) {
        // Delay displaying window so we can center it first
        glfwWindowHint(GLFW_VISIBLE, GL_FALSE);
    }

    ta_log_write(&tg_debug_log, SRC_WINDOW, "glfwCreateWindow...\n");
    window->glfw_window = glfwCreateWindow(w, h, "Talaria", fullscreen ? monitor : 0, 0);
    if (!window->glfw_window) {
        ta_log_write(&tg_debug_log, SRC_WINDOW, "glfwCreateWindow error\n");
        DLB_ASSERT(!"ta_window_init: glfwCreateWindow failed");
    }

    if (!fullscreen) {
        // Center window
        int cx = (mode->width - w) / 2;
        int cy = (mode->height - h) / 2;
        glfwSetWindowPos(window->glfw_window, cx, cy);
        glfwShowWindow(window->glfw_window);
    }

    // Register event callbacks
    glfwSetWindowUserPointer        (window->glfw_window, window);
    glfwSetWindowCloseCallback      (window->glfw_window, window_glfw_close);
    glfwSetFramebufferSizeCallback  (window->glfw_window, window_glfw_framebuffer_resize);
    glfwSetKeyCallback              (window->glfw_window, window_glfw_key);
    glfwSetCharCallback             (window->glfw_window, window_glfw_codepoint);
    glfwSetMouseButtonCallback      (window->glfw_window, window_glfw_mouse_button);
    glfwSetCursorPosCallback        (window->glfw_window, window_glfw_mouse_move);
    glfwSetScrollCallback           (window->glfw_window, window_glfw_scroll);
    glfwSetDropCallback             (window->glfw_window, window_glfw_dragndrop);

    ta_log_write(&tg_debug_log, SRC_WINDOW, "glfwMakeContextCurrent...\n");
    glfwMakeContextCurrent(window->glfw_window);

    ta_log_write(&tg_debug_log, SRC_WINDOW, "gladLoadGL...\n");
    if (!gladLoadGL()) {
        ta_log_write(&tg_debug_log, SRC_WINDOW, "gladLoadGL failed\n");
        DLB_ASSERT(!"ta_window_init: failed to init GLAD");
    }

#if 1
    glfwSwapInterval(1);
    window->vsync = true;
#else
    glfwSwapInterval(0);
#endif
    glfwGetFramebufferSize(window->glfw_window, &window->width, &window->height);

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
    glGetFramebufferAttachmentParameteriv(GL_DRAW_FRAMEBUFFER,
        GL_STENCIL, GL_FRAMEBUFFER_ATTACHMENT_STENCIL_SIZE, &stencilSize);
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

    // Use raw input when cursor disabled (for camera rotation)
    if (glfwRawMouseMotionSupported()) {
        glfwSetInputMode(window->glfw_window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    }

    ta_log_write(&tg_debug_log, SRC_WINDOW, "Loading cursors\n");
    window_cursor_arrow = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
    window_cursor_hresize = glfwCreateStandardCursor(GLFW_HRESIZE_CURSOR);
    window_cursor_ibeam = glfwCreateStandardCursor(GLFW_IBEAM_CURSOR);
    window->glfw_active_cursor = window_cursor_arrow;
    window->glfw_requested_cursor = window_cursor_arrow;
}

void ta_window_free(ta_window *window)
{
    glfwDestroyWindow(window->glfw_window);
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
    //glfwGetFramebufferSize(window->glfw_window, w, h);
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
        glfwSwapInterval(vsync ? 1 : 0);
        window->vsync = vsync;
    }
}

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

    glfwGetWindowPos(window->glfw_window, &wx, &wy);
    glfwGetWindowSize(window->glfw_window, &ww, &wh);
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
void ta_window_get_fullscreen(ta_window *window, bool *fullscreen)
{
    *fullscreen = window->fullscreen;
}
void ta_window_set_fullscreen(ta_window *window, bool fullscreen)
{
    if (fullscreen == window->fullscreen)
        return;

    if (fullscreen) {
        glfwGetWindowPos(window->glfw_window, &window->restore.x, &window->restore.y);
        glfwGetWindowSize(window->glfw_window, &window->restore.w, &window->restore.h);

        GLFWmonitor *monitor = window_current_monitor(window); //glfwGetPrimaryMonitor();
        const GLFWvidmode *mode = glfwGetVideoMode(monitor);
        glfwSetWindowMonitor(window->glfw_window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
        glfwSwapInterval(window->vsync);
        window->fullscreen = true;
    } else {
        glfwSetWindowMonitor(window->glfw_window, 0, window->restore.x, window->restore.y, window->restore.w,
            window->restore.h, 0);
        glfwSwapInterval(window->vsync);
        window->fullscreen = false;
    }
}

void ta_window_request_cursor(ta_window *window, ta_cursor_type cursor_type)
{
    GLFWcursor *cursor = window_cursor_arrow;
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
    window->glfw_requested_cursor = cursor;
}
void ta_window_set_cursor_pos(ta_window *window, int x, int y)
{
    glfwSetCursorPos(window->glfw_window, (double)x, (double)y);
}
void ta_window_get_cursor_pos(ta_window *window, int *x, int *y)
{
    double x_screen, y_screen;
    glfwGetCursorPos(window->glfw_window, &x_screen, &y_screen);
    if (x) *x = (int)floor(x_screen);
    if (y) *y = (int)floor(y_screen);
}
void ta_window_set_cursor_mode(ta_window *window, int glfw_cursor_mode)
{
    glfwSetInputMode(window->glfw_window, GLFW_CURSOR, glfw_cursor_mode);
}

void ta_window_update_cursor(ta_window *window)
{
    if (window->glfw_requested_cursor != window->glfw_active_cursor) {
        glfwSetCursor(window->glfw_window, window->glfw_requested_cursor);
        window->glfw_active_cursor = window->glfw_requested_cursor;
    }
}
void ta_window_swap(ta_window *window)
{
    glfwSwapBuffers(window->glfw_window);
}
