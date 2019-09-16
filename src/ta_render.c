#include "ta_render.h"
#include "ta_log.h"
//#include "ta_shader_lines.h"
#include "misc/gl3w.h"

static void ta_render_init_gl3w(int major, int minor)
{
    int init = gl3wInit();
    if (init) {
        ta_log_write(&tg_debug_log, "[Render] gl3wInit failed with code %d", init);
        DLB_ASSERT(!"init_gl3w: failed to init gl3w");
    }

    ta_log_write(&tg_debug_log, "[Render] OpenGL: %s\n", glGetString(GL_VERSION));
    ta_log_write(&tg_debug_log, "[Render] GLSL: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));

    if (!gl3wIsSupported(major, minor)) {
        ta_log_write(&tg_debug_log, "[Render] OpenGL %d.%d not supported", major, minor);
    }
}

static void APIENTRY ta_render_gl_callback(GLenum source, GLenum type,
    GLuint id, GLenum severity, GLsizei length, const GLchar *message,
    const void *userParam)
{
    UNUSED(length);
    UNUSED(userParam);

    char *sourceStr, *typeStr, *severityStr;

    switch (source) {
    case GL_DEBUG_SOURCE_API:
        sourceStr = "API            ";
        break;
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
        sourceStr = "WINDOW SYSTEM  ";
        break;
    case GL_DEBUG_SOURCE_SHADER_COMPILER:
        sourceStr = "SHADER COMPILER";
        break;
    case GL_DEBUG_SOURCE_THIRD_PARTY:
        sourceStr = "THIRD PARTY    ";
        break;
    case GL_DEBUG_SOURCE_APPLICATION:
        sourceStr = "APPLICATION    ";
        break;
    case GL_DEBUG_SOURCE_OTHER:
        sourceStr = "OTHER          ";
        return;
        break;
    default:
        sourceStr = "???????????????";
        break;
    }

    switch (type) {
    case GL_DEBUG_TYPE_ERROR:
        typeStr = "ERROR";
        break;
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
        typeStr = "DEPRC";
        break;
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
        typeStr = "UNDEF";
        break;
    case GL_DEBUG_TYPE_PORTABILITY:
        typeStr = "PORT ";
        break;
    case GL_DEBUG_TYPE_PERFORMANCE:
        typeStr = "PERF ";
        break;
    case GL_DEBUG_TYPE_OTHER:
        typeStr = "OTHER";
        return;
        break;
    default:
        typeStr = "?????";
        break;
    }

    switch (severity) {
    case GL_DEBUG_SEVERITY_LOW:
        severityStr = "LOW ";
        break;
    case GL_DEBUG_SEVERITY_MEDIUM:
        severityStr = "MED ";
        break;
    case GL_DEBUG_SEVERITY_HIGH:
        severityStr = "HIGH";
        break;
    default:
        severityStr = "????";
        break;
    }

    static u32 gl_errors = 0;
    if (gl_errors < 10)
    {
        ta_log_write(&tg_debug_log, "[source: %s][type: %s][severity: %s][id: %d] %s\n", sourceStr, typeStr,
            severityStr, id, message);
        gl_errors++;
    }
}

static void ta_render_init_gl()
{
#if _DEBUG
    {
        GLint gl_max_vertex_attribs;
        glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &gl_max_vertex_attribs);
        ta_log_write(&tg_debug_log, "[Render] GL_MAX_VERTEX_ATTRIBS = %d\n", gl_max_vertex_attribs);
    }

    if (glDebugMessageCallback != NULL)
    {
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(ta_render_gl_callback, NULL);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, 0, true);
        ta_log_write(&tg_debug_log, "[Render] Registered glDebugMessageCallback\n");
    }
    else
    {
        ta_log_write(&tg_debug_log, "[Render] glDebugMessageCallback not available\n");
    }
#endif

    glClearColor(0.4f, 0.7f, 1.0f, 1.0f);

    // Stencil buffer
    //glStencilMask(0x00);
    //glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
    //glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

    GLint stencilSize = 0;
    glGetFramebufferAttachmentParameteriv(GL_DRAW_FRAMEBUFFER,
        GL_STENCIL, GL_FRAMEBUFFER_ATTACHMENT_STENCIL_SIZE, &stencilSize);
    DLB_ASSERT(stencilSize == 8);

    // Depth buffer
    glDepthFunc(GL_LEQUAL);    // Default GL_LESS
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
}

void ta_render_init()
{
    ta_render_init_gl3w(3, 2);
    ta_render_init_gl();
}