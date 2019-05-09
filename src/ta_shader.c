#include "ta_shader.h"
#include "ta_log.h"
#include "ta_file.h"
#include "ta_symbol.h"
#include "dlb_memory.h"
#include "dlb_vector.h"
#include "misc/gl3w.h"

ta_shader *tg_shader_lines;
ta_shader *tg_shader_quads;
ta_shader *tg_shader_mesh;

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

static GLuint ta_shader_compile(GLenum type, ta_buffer *buf)
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

static GLuint ta_shader_compile_file(GLenum type, const char *filename)
{
    ta_buffer *buf = ta_file_read_all(filename);

    GLint shader = ta_shader_compile(type, buf);
    if (!shader) {
        ta_log_write(tg_debug_log, "Failed to compile shader '%s'\n", filename);
    }

	ta_buffer_free(buf);
    return shader;
}

static void ta_shader_program_link(GLuint program)
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

static GLint ta_shader_attribute_location(ta_shader *shader, const char *name)
{
    GLint location = glGetAttribLocation(shader->program_id, name);
    if (location < 0)
    {
        ta_log_write(tg_debug_log,
            "[Shader] Failed to locate attribute by '%s' in '%s'. "
            "Possibly optimized out.\n", name, shader->uid);
    }
    return location;
}

static GLint ta_shader_uniform_location(ta_shader *shader, const char *name)
{
    GLint location = glGetUniformLocation(shader->program_id, name);
    if (location < 0)
    {
        ta_log_write(tg_debug_log,
            "[Shader] Failed to locate uniform '%s' in '%s'. "
            "Possibly optimized out.\n", name, shader->uid);
    }
    return location;
}

static ta_shader_attribute *find_attribute_by_name(ta_shader *shader,
    const char *name, const char *type)
{
    ta_shader_attribute *result = 0;
    for (ta_shader_attribute *attr = shader->attributes;
        attr != dlb_vec_end(shader->attributes); attr++)
    {
        if (attr->name == name) {
            result = attr;
            break;
        }
    }
    DLB_ASSERT(!type || !result || result->type == type);
    return result;
}

static ta_shader_uniform *find_uniform_by_name(ta_shader *shader,
    const char *name, const char *type)
{
    ta_shader_uniform *result = 0;
    for (ta_shader_uniform *u = shader->uniforms;
        u != dlb_vec_end(shader->uniforms); u++)
    {
        if (u->name == name) {
            result = u;
            break;
        }
    }
    DLB_ASSERT(result);
    DLB_ASSERT(!type || result->type == type);
    return result;
}

static void delete_shader(GLuint shader)
{
    if (shader) {
        glDeleteShader(shader);
    }
}

void ta_shader_init(ta_shader *shader, const char *path_vert,
    const char *path_frag)
{
    shader->path_vert = path_vert;
    shader->path_frag = path_frag;
}

void ta_shader_create(ta_shader *shader)
{
    DLB_ASSERT(shader->path_vert);
    DLB_ASSERT(shader->path_frag);

    // Compile shaders
    GLuint vshader = ta_shader_compile_file(GL_VERTEX_SHADER, shader->path_vert);
    if (!vshader) {
        DLB_ASSERT(!"ta_shader_init: failed to compile vertex shader");
    }

    GLuint fshader = ta_shader_compile_file(GL_FRAGMENT_SHADER, shader->path_frag);
    if (!fshader) {
        delete_shader(vshader);
        DLB_ASSERT(!"ta_shader_init: failed to compile fragment shader");
    }

    // Link program
    shader->program_id = glCreateProgram();
    GLuint program_id = shader->program_id;
    glAttachShader(program_id, vshader);
    glAttachShader(program_id, fshader);
    glBindAttribLocation(program_id, TA_SHADER_ATTR_POSITION, "attr_position");
    glBindAttribLocation(program_id, TA_SHADER_ATTR_COLOR,    "attr_color");
    glBindAttribLocation(program_id, TA_SHADER_ATTR_UV,       "attr_uv");
    glBindAttribLocation(program_id, TA_SHADER_ATTR_NORMAL,   "attr_normal");
    ta_shader_program_link(program_id);

    for (ta_shader_attribute *attr = shader->attributes;
        attr != dlb_vec_end(shader->attributes); attr++)
    {
        attr->location = ta_shader_attribute_location(shader, attr->name);
    }

    // Ensure vertex attributes are at the correct locations
    ta_shader_attribute *attr_pos =
        find_attribute_by_name(shader, SYM_ATTR_POSITION, SYM_VEC3);
    ta_shader_attribute *attr_col =
        find_attribute_by_name(shader, SYM_ATTR_COLOR,    SYM_VEC4);
    ta_shader_attribute *attr_uv =
        find_attribute_by_name(shader, SYM_ATTR_UV,       SYM_VEC2);
    ta_shader_attribute *attr_norm =
        find_attribute_by_name(shader, SYM_ATTR_NORMAL,   SYM_VEC3);

    DLB_ASSERT(!attr_pos  || attr_pos->location  < 0 || attr_pos->location  == TA_SHADER_ATTR_POSITION);
    DLB_ASSERT(!attr_col  || attr_col->location  < 0 || attr_col->location  == TA_SHADER_ATTR_COLOR);
    DLB_ASSERT(!attr_uv   || attr_uv->location   < 0 || attr_uv->location   == TA_SHADER_ATTR_UV);
    DLB_ASSERT(!attr_norm || attr_norm->location < 0 || attr_norm->location == TA_SHADER_ATTR_NORMAL);

    for (ta_shader_uniform *u = shader->uniforms;
        u != dlb_vec_end(shader->uniforms); u++)
    {
        u->location = ta_shader_uniform_location(shader, u->name);
    }

    glDetachShader(program_id, vshader);
    glDetachShader(program_id, fshader);
    delete_shader(vshader);
    delete_shader(fshader);

}

void ta_shader_delete(ta_shader *shader)
{
    if (shader->program_id) {
        glDeleteProgram(shader->program_id);
    }
}

void ta_shader_free(ta_shader *shader)
{
    ta_shader_delete(shader);
    dlb_vec_free(shader->attributes);
    dlb_vec_free(shader->uniforms);
}

void ta_shader_bind(ta_shader *shader)
{
    DLB_ASSERT(shader->program_id);
    glUseProgram(shader->program_id);
}

void ta_shader_unbind(ta_shader *shader)
{
    UNUSED(shader);
    glUseProgram(0);
}

void ta_shader_set_mat4(ta_shader *shader, const char *name,
    const ta_mat4 *matrix)
{
    ta_shader_uniform *u = find_uniform_by_name(shader, name, SYM_MAT4);
    u->value.mat4.matrix = *matrix;
}

void ta_shader_set_sampler2d(ta_shader *shader, const char *name,
    GLuint texture_id)
{
    ta_shader_uniform *u = find_uniform_by_name(shader, name, SYM_SAMPLER2D);
    u->value.sampler2d.texture_id = texture_id;
}

void ta_shader_prerender(ta_shader *shader)
{
    int tex_count = 0;
    for (ta_shader_uniform *u = shader->uniforms;
        u != dlb_vec_end(shader->uniforms); u++)
    {
        if (u->location < 0) {
            continue;
        }

        if (u->type == SYM_MAT4) {
            glUniformMatrix4fv(u->location, 1, GL_TRUE,
                (GLfloat *)&u->value.mat4.matrix);
        } else if (u->type == SYM_SAMPLER2D) {
            GLuint tex_id = u->value.sampler2d.texture_id;
            if (tex_id >= 0) {
                glActiveTexture(GL_TEXTURE0 + tex_count);
                glBindTexture(GL_TEXTURE_2D, tex_id);
                glUniform1i(u->location, 0);
            }
        } else {
            DLB_ASSERT(!"TODO: Handle other uniform types");
        }
    }
}