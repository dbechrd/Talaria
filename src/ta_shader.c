#include "ta_shader.h"
#include "ta_log.h"
#include "ta_file.h"
#include "dlb_memory.h"
#include "misc/gl3w.h"

static void show_info_log(GLuint shader, PFNGLGETSHADERIVPROC glGet__iv,
    PFNGLGETSHADERINFOLOGPROC glGet__InfoLog)
{
    ta_buffer buf;

    glGet__iv(shader, GL_INFO_LOG_LENGTH, (GLint *)&buf.length);
    buf.data = dlb_malloc(buf.length);
    glGet__InfoLog(shader, buf.length, NULL, buf.data);
    ta_log_write(tg_debug_log, "OpenGL info log:\n%s\n", buf.data);
    dlb_free(buf.data);
    DLB_ASSERT(!"show_info_log: GL error occurred");
};

GLuint ta_shader_compile(GLenum type, ta_buffer *buf)
{
    GLuint shader = glCreateShader(type);

    // Read shader source
    glShaderSource(shader, 1, &buf->data, (GLint *)&buf->length);

    // Compile shader
    GLint status;
    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (!status)
    {
        show_info_log(shader, glGetShaderiv, glGetShaderInfoLog);
        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

GLuint ta_shader_compile_file(GLenum type, const char *filename)
{
    ta_buffer *buf = ta_file_read_all(filename);

    GLint shader = ta_shader_compile(type, buf);
    if (!shader) {
        ta_log_write(tg_debug_log, "Failed to compile shader '%s'\n", filename);
    }

	ta_buffer_free(buf);
    return shader;
}

GLuint ta_shader_program_init() {
    GLuint program = glCreateProgram();
    return program;
}

void ta_shader_program_link(GLuint program)
{
    glLinkProgram(program);

    GLint status;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (status == GL_FALSE)
    {
        show_info_log(program, glGetProgramiv, glGetProgramInfoLog);
        glDeleteProgram(program);
        DLB_ASSERT(!"ta_shader_program_link: failed to link shader program");
    }
}

GLint ta_shader_attribute_location(GLuint program, const char *name)
{
    GLint location = glGetAttribLocation(program, name);
    if (location < 0)
    {
        ta_log_write(tg_debug_log,
            "[Program %d] Failed to locate attribute by name '%s'. "
            "Possibly optimized out.\n", program, name);
    }
    return location;
}

GLint ta_shader_uniform_location(GLuint program, const char *name)
{
    GLint location = glGetUniformLocation(program, name);
    if (location < 0)
    {
        ta_log_write(tg_debug_log,
            "[Program %d] Failed to locate uniform by name '%s'. "
            "Possibly optimized out.\n", program, name);
    }
    return location;
}

void ta_shader_free(GLuint shader)
{
    if (shader) {
        glDeleteShader(shader);
    }
}

void ta_shader_program_free(GLuint program)
{
    if (program) {
        glDeleteProgram(program);
    }
}