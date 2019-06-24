#include "ta_shader.h"
#include "ta_log.h"
#include "ta_file.h"
#include "ta_symbol.h"
#include "ta_light.h"
#include "dlb_memory.h"
#include "dlb_vector.h"
#include "misc/gl3w.h"

ta_shader *tg_shader_lines;
ta_shader *tg_shader_quads;

const char *ta_glsl_type_str(int type)
{
    switch(type) {
        case TA_GLSL_INT:       return "TA_GLSL_GLINT";
        case TA_GLSL_UINT:      return "TA_GLSL_GLUINT";
        case TA_GLSL_FLOAT:     return "TA_GLSL_FLOAT";
        case TA_GLSL_SAMPLER2D: return "TA_GLSL_SAMPLER2D";
        case TA_GLSL_VEC2:      return "TA_GLSL_VEC2";
        case TA_GLSL_VEC3:      return "TA_GLSL_VEC3";
        case TA_GLSL_VEC4:      return "TA_GLSL_VEC4";
        case TA_GLSL_MAT3:      return "TA_GLSL_MAT3";
        case TA_GLSL_MAT4:      return "TA_GLSL_MAT4";
        case TA_GLSL_STRUCT:    return "TA_GLSL_STRUCT";
        default:
            DLB_ASSERT(!"<UNKNOWN_TA_GLSL_TYPE>");
            return 0;
    }
}

static void show_info_log(GLuint shader, PFNGLGETSHADERIVPROC glGet__iv,
    PFNGLGETSHADERINFOLOGPROC glGet__InfoLog)
{
    ta_buffer buf;

    glGet__iv(shader, GL_INFO_LOG_LENGTH, (GLint *)&buf.length);
    buf.data = dlb_malloc(buf.length);
    glGet__InfoLog(shader, buf.length, NULL, (GLchar *)buf.data);
    ta_log_write(tg_debug_log,
        "\n---[OpenGL Info Log]------------------------------------------------------------\n"
        "%s\n", buf.data);
    dlb_free(buf.data);
    DLB_ASSERT(!"show_info_log: GL error occurred");
};

static GLuint ta_shader_compile(GLenum type, ta_buffer *buf)
{
    GLuint shader = glCreateShader(type);

    // Read shader source
    glShaderSource(shader, 1, &(GLchar *)buf->data, (GLint *)&buf->length);

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
            "Possibly optimized out.\n", name, shader->ref.uid);
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
            "Possibly optimized out.\n", name, shader->ref.uid);
    }
    return location;
}

// Return attribute if found, else null
static ta_shader_attribute *find_attribute_by_name(ta_shader *shader,
    const char *name, ta_glsl_type type)
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
    DLB_ASSERT(!result || result->type == type);
    return result;
}

// Return attribute if found, else assert
static ta_shader_uniform *find_uniform_by_name(ta_shader_uniform *uniforms,
    const char *name, ta_glsl_type type)
{
    ta_shader_uniform *result = 0;
    for (ta_shader_uniform *u = uniforms;
        u != dlb_vec_end(uniforms); u++)
    {
        if (u->name == name) {
            result = u;
            break;
        }
    }
    DLB_ASSERT(!result || result->type == type);
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

static void shader_print_uniforms(ta_shader *shader)
{
    GLint count;
    glGetProgramiv(shader->program_id, GL_ACTIVE_UNIFORMS, &count);
    ta_log_write(tg_debug_log, "[Shader]  Active uniforms: %d\n", count);

    GLsizei length;
    GLint size; // size of the variable
    GLenum type; // type of the variable (float, vec3 or mat4, etc)
    GLchar *name = dlb_malloc(shader->max_uniform_name_len);
    for (GLint i = 0; i < count; i++)
    {
        glGetActiveUniform(shader->program_id, (GLuint)i,
            shader->max_uniform_name_len, &length, &size, &type, name);
        ta_log_write(tg_debug_log, "[Shader]    Uniform #%d Name: %s Type: %u\n",
            i, name, type);
    }
    dlb_free(name);
}

static void shader_locate_uniforms(ta_shader *shader, ta_shader_uniform *uniforms)
{
    for (ta_shader_uniform *u = uniforms; u != dlb_vec_end(uniforms); u++)
    {
        u->location = ta_shader_uniform_location(shader, u->name);
        if (u->type == TA_GLSL_STRUCT) {
            shader_locate_uniforms(shader, u->value.properties);
        }
    }
}

void ta_shader_create(ta_shader *shader)
{
    DLB_ASSERT(shader->path_vert);
    DLB_ASSERT(shader->path_frag);

    ta_log_write(tg_debug_log, "[Shader] Creating shader %s\n", shader->ref.uid);

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
        find_attribute_by_name(shader, SYM_ATTR_POSITION, TA_GLSL_VEC3);
    ta_shader_attribute *attr_col =
        find_attribute_by_name(shader, SYM_ATTR_COLOR,    TA_GLSL_VEC4);
    ta_shader_attribute *attr_uv =
        find_attribute_by_name(shader, SYM_ATTR_UV,       TA_GLSL_VEC2);
    ta_shader_attribute *attr_norm =
        find_attribute_by_name(shader, SYM_ATTR_NORMAL,   TA_GLSL_VEC3);

    DLB_ASSERT(!attr_pos  || attr_pos->location  < 0 || attr_pos->location  == TA_SHADER_ATTR_POSITION);
    DLB_ASSERT(!attr_col  || attr_col->location  < 0 || attr_col->location  == TA_SHADER_ATTR_COLOR);
    DLB_ASSERT(!attr_uv   || attr_uv->location   < 0 || attr_uv->location   == TA_SHADER_ATTR_UV);
    DLB_ASSERT(!attr_norm || attr_norm->location < 0 || attr_norm->location == TA_SHADER_ATTR_NORMAL);

    glGetProgramiv(shader->program_id, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH,
        &shader->max_attrib_name_len);
    glGetProgramiv(shader->program_id, GL_ACTIVE_UNIFORM_MAX_LENGTH,
        &shader->max_uniform_name_len);

    shader_print_uniforms(shader);
    shader_locate_uniforms(shader, shader->uniforms);

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

void ta_shader_set_int(ta_shader *shader, const char *name, GLint value)
{
    ta_shader_uniform *u = find_uniform_by_name(shader->uniforms, name, TA_GLSL_INT);
    u->value.glint = value;
}

void ta_shader_set_uint(ta_shader *shader, const char *name, GLuint value)
{
    ta_shader_uniform *u = find_uniform_by_name(shader->uniforms, name, TA_GLSL_UINT);
    u->value.gluint = value;
}

void ta_shader_set_float(ta_shader *shader, const char *name, GLfloat value)
{
    ta_shader_uniform *u = find_uniform_by_name(shader->uniforms, name, TA_GLSL_FLOAT);
    u->value.glfloat = value;
}

void ta_shader_set_sampler2d(ta_shader *shader, const char *name, GLuint tex_id)
{
    ta_shader_uniform *u = find_uniform_by_name(shader->uniforms, name, TA_GLSL_SAMPLER2D);
    u->value.gluint = tex_id;
}

void ta_shader_set_vec2(ta_shader *shader, const char *name, const ta_vec2 *v)
{
    ta_shader_uniform *u = find_uniform_by_name(shader->uniforms, name, TA_GLSL_VEC2);
    u->value.vec2 = *v;
}

void ta_shader_set_vec3(ta_shader *shader, const char *name, const ta_vec3 *v)
{
    ta_shader_uniform *u = find_uniform_by_name(shader->uniforms, name, TA_GLSL_VEC3);
    u->value.vec3 = *v;
}

void ta_shader_set_vec4(ta_shader *shader, const char *name, const ta_vec4 *v)
{
    ta_shader_uniform *u = find_uniform_by_name(shader->uniforms, name, TA_GLSL_VEC4);
    u->value.vec4 = *v;
}

void ta_shader_set_mat3(ta_shader *shader, const char *name, const ta_mat3 *m)
{
    ta_shader_uniform *u = find_uniform_by_name(shader->uniforms, name, TA_GLSL_MAT3);
    u->value.mat3 = *m;
}

void ta_shader_set_mat4(ta_shader *shader, const char *name, const ta_mat4 *m)
{
    ta_shader_uniform *u = find_uniform_by_name(shader->uniforms, name, TA_GLSL_MAT4);
    u->value.mat4 = *m;
}

void ta_shader_set_light(ta_shader *shader, const char *name, int index,
    ta_light *light)
{
    ta_shader_uniform *u = find_uniform_by_name(shader->uniforms, name, TA_GLSL_STRUCT);

    ta_shader_uniform *u_intensity=
        find_uniform_by_name(u->value.properties, SYM_U_LIGHTS_INTENSITY[index],
            TA_GLSL_FLOAT);
    ta_shader_uniform *u_position =
        find_uniform_by_name(u->value.properties, SYM_U_LIGHTS_POSITION[index],
            TA_GLSL_VEC3);
    ta_shader_uniform *u_color =
        find_uniform_by_name(u->value.properties, SYM_U_LIGHTS_COLOR[index],
            TA_GLSL_VEC3);
    ta_shader_uniform *u_type =
        find_uniform_by_name(u->value.properties, SYM_U_LIGHTS_TYPE[index],
            TA_GLSL_INT);
    ta_shader_uniform *u_direction =
        find_uniform_by_name(u->value.properties, SYM_U_LIGHTS_DIRECTION[index],
            TA_GLSL_VEC3);

    u_intensity->value.glfloat = light->intensity;
    u_color->value.rgb         = light->color;
    u_position->value.vec3     = light->position;
    u_type->value.glint        = light->type;
    u_direction->value.vec3    = VEC3_ZERO;

    switch (light->type) {
        case TA_LIGHT_AMBIENT:
            break;
        case TA_LIGHT_DIRECTIONAL:
            u_direction->value.vec3 = light->data.directional.direction;
            break;
        case TA_LIGHT_POINT:
            break;
        case TA_LIGHT_SPOT:
            u_direction->value.vec3 = light->data.directional.direction;
            DLB_ASSERT(!"Don't handle spot lights yet");
            break;
        default:
            DLB_ASSERT(!"Don't know how to initialize this type of light");
    }
}

static void shader_bind_uniforms(ta_shader_uniform *uniforms, int *tex_count)
{
    for (ta_shader_uniform *u = uniforms; u != dlb_vec_end(uniforms); u++)
    {
        if (u->location < 0 && u->type != TA_GLSL_STRUCT) {
            continue;
        }

        switch (u->type) {
            case TA_GLSL_INT: {
                glUniform1i(u->location, u->value.glint);
                break;
            } case TA_GLSL_UINT: {
                glUniform1ui(u->location, u->value.gluint);
                break;
            } case TA_GLSL_FLOAT: {
                glUniform1f(u->location, u->value.glfloat);
                break;
            } case TA_GLSL_SAMPLER2D: {
                GLuint tex_id = u->value.gluint;
                if (tex_id >= 0) {
                    glActiveTexture(GL_TEXTURE0 + *tex_count);
                    glBindTexture(GL_TEXTURE_2D, tex_id);
                    glUniform1i(u->location, *tex_count);
                    (*tex_count)++;
                }
                break;
            } case TA_GLSL_VEC2: {
                glUniform2fv(u->location, 1, (GLfloat *)&u->value.vec2);
                break;
            } case TA_GLSL_VEC3: {
                glUniform3fv(u->location, 1, (GLfloat *)&u->value.vec3);
                break;
            } case TA_GLSL_VEC4: {
                glUniform4fv(u->location, 1, (GLfloat *)&u->value.vec4);
                break;
            } case TA_GLSL_MAT3: {
                glUniformMatrix3fv(u->location, 1, GL_TRUE, (GLfloat *)&u->value.mat3);
                break;
            } case TA_GLSL_MAT4: {
                glUniformMatrix4fv(u->location, 1, GL_TRUE, (GLfloat *)&u->value.mat4);
                break;
            } case TA_GLSL_STRUCT: {
                shader_bind_uniforms(u->value.properties, tex_count);
                break;
            } default: {
                DLB_ASSERT(!"Unexpected GLSL type");
            }
        }
    }
}

void ta_shader_prerender(ta_shader *shader)
{
    int tex_count = 0;
    shader_bind_uniforms(shader->uniforms, &tex_count);
}