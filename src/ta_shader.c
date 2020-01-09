#include "ta_buffer.h"
#include "ta_file.h"
#include "ta_light.h"
#include "ta_log.h"
#include "ta_shader.h"
#include "ta_symbol.h"
#include "dlb/dlb_memory.h"
#include "dlb/dlb_vector.h"
#include "misc/gl3w.h"

ta_shader *tg_shader_lines;
ta_shader *tg_shader_quads;
ta_shader *tg_shader_cubemap;

const char *ta_glsl_type_str(int type)
{
    switch(type) {
        case TA_GLSL_BOOL:         return "TA_GLSL_BOOL";
        case TA_GLSL_INT:          return "TA_GLSL_INT";
        case TA_GLSL_UINT:         return "TA_GLSL_UINT";
        case TA_GLSL_FLOAT:        return "TA_GLSL_FLOAT";
        case TA_GLSL_SAMPLER2D:    return "TA_GLSL_SAMPLER2D";
        case TA_GLSL_VEC2:         return "TA_GLSL_VEC2";
        case TA_GLSL_VEC3:         return "TA_GLSL_VEC3";
        case TA_GLSL_VEC4:         return "TA_GLSL_VEC4";
        case TA_GLSL_MAT3:         return "TA_GLSL_MAT3";
        case TA_GLSL_MAT4:         return "TA_GLSL_MAT4";
        case TA_GLSL_STRUCT:       return "TA_GLSL_STRUCT";
        case TA_GLSL_SAMPLER_CUBE: return "TA_GLSL_SAMPLER_CUBE";
        default:
            DLB_ASSERT(!"<UNKNOWN_TA_GLSL_TYPE>");
            return 0;
    }
}

void ta_shader_init(ta_shader *shader)
{
    ta_shader_load(shader);
}

static void show_info_log(GLuint shader)
{
    ta_log_write(&tg_debug_log, SRC_SHADER, "Trying to show_info_log\n");

    GLint length = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
    if (length) {
        ta_buffer buf = { 0 };
        ta_buffer_init(&buf, length);
        glGetShaderInfoLog(shader, buf.length, NULL, (GLchar *)buf.data);
        ta_log_write(&tg_debug_log, SRC_SHADER,
            "\n---[OpenGL Info Log]------------------------------------------------------------\n"
            "%s\n", buf.data);
        ta_buffer_free(buf);
    } else {
        ta_log_write(&tg_debug_log, SRC_SHADER,
            "\n---[OpenGL Info Log]------------------------------------------------------------\n"
            "No error text: GL_INFO_LOG_LENGTH = 0\n");
    }
    DLB_ASSERT(!"show_info_log: GL error occurred");
};

static GLuint ta_shader_compile(GLenum type, ta_buffer buf)
{
    GLuint shader = glCreateShader(type);

    // Read shader source
    glShaderSource(shader, 1, &(GLchar *)buf.data, (GLint *)&buf.length);

    // Compile shader
    GLint status;
    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (!status) {
        show_info_log(shader);
        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

static GLuint ta_shader_compile_file(GLenum type, const char *filename)
{
    ta_buffer buf = ta_file_read_all(filename);

    GLint shader = ta_shader_compile(type, buf);
    if (!shader) {
        ta_log_write(&tg_debug_log, SRC_SHADER, "Failed to compile shader '%s'\n",
            filename);
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
        show_info_log(program);
        glDeleteProgram(program);
        DLB_ASSERT(!"ta_shader_program_link: failed to link shader program");
    }
}

static GLint ta_shader_attribute_location(ta_shader *shader, const char *name)
{
    GLint location = glGetAttribLocation(shader->program_id, name);
    if (location < 0) {
        // TODO: Log as warning
        //ta_log_write(&tg_debug_log, SRC_SHADER,
        //    "Failed to locate attribute by '%s' in '%s'. "
        //    "Possibly optimized out.\n", name, shader->path_frag);
    }
    return location;
}

static GLint ta_shader_uniform_location(ta_shader *shader, const char *name)
{
    GLint location = glGetUniformLocation(shader->program_id, name);
    if (location < 0) {
        // TODO: Log as warning
        //ta_log_write(&tg_debug_log, SRC_SHADER,
        //    "Failed to locate uniform '%s' in '%s'. "
        //    "Possibly optimized out.\n", name, shader->path_frag);
    }
    return location;
}

static ta_shader_attribute *find_attribute_by_name(ta_shader *shader,
    const char *name, ta_glsl_type type)
{
    ta_shader_attribute *result = 0;
    dlb_vec_each(ta_shader_attribute *, attr, shader->attributes) {
        if (attr->name == name) {
            result = attr;
            break;
        }
    }
    DLB_ASSERT(!result || result->type == type);
    return result;
}

static ta_shader_uniform *find_uniform_by_name(ta_shader_uniform *uniforms,
    const char *name, ta_glsl_type type)
{
    ta_shader_uniform *result = 0;
    dlb_vec_each(ta_shader_uniform *, uniform, uniforms) {
        if (uniform->name == name) {
            result = uniform;
            break;
        }
    }
    DLB_ASSERT(result && result->type == type);
    return result;
}

static void shader_print_uniforms(ta_shader *shader)
{
    GLint count;
    glGetProgramiv(shader->program_id, GL_ACTIVE_UNIFORMS, &count);
    //ta_log_write(&tg_debug_log, SRC_SHADER, " Active uniforms: %d\n", count);

    GLsizei length;
    GLint size; // size of the variable
    GLenum type; // type of the variable (float, vec3 or mat4, etc)
    GLchar *name = dlb_malloc(shader->max_uniform_name_len);
    for (GLint i = 0; i < count; i++) {
        glGetActiveUniform(shader->program_id, (GLuint)i,
            shader->max_uniform_name_len, &length, &size, &type, name);
        //ta_log_write(&tg_debug_log, SRC_SHADER, "   Uniform #%d Name: %s Type: %u\n",
        //    i, name, type);
    }
    dlb_free(name);
}

static void shader_init_uniforms(ta_shader *shader, ta_shader_uniform *uniforms)
{
    dlb_vec_each(ta_shader_uniform *, uniform, uniforms) {
        if (uniform->type == TA_GLSL_STRUCT) {
            shader_init_uniforms(shader, uniform->value.properties);
        } else {
            uniform->location = ta_shader_uniform_location(shader, uniform->name);
        }
        uniform->dirty = true;
    }
}

void ta_shader_load(ta_shader *shader)
{
    DLB_ASSERT(shader->path_vert);
    DLB_ASSERT(shader->path_frag);

    ta_log_write(&tg_debug_log, SRC_SHADER, "Creating shader %s\n", shader->path_frag);

    // Compile shaders
    GLuint vshader = ta_shader_compile_file(GL_VERTEX_SHADER, shader->path_vert);
    if (!vshader) {
        DLB_ASSERT(!"ta_shader_init: failed to compile vertex shader");
    }

    GLuint fshader = ta_shader_compile_file(GL_FRAGMENT_SHADER, shader->path_frag);
    if (!fshader) {
        glDeleteShader(vshader);
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
    glBindAttribLocation(program_id, TA_SHADER_ATTR_TANGENT,  "attr_tangent");
    ta_shader_program_link(program_id);

    glGetProgramiv(shader->program_id, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH,
        &shader->max_attrib_name_len);
    glGetProgramiv(shader->program_id, GL_ACTIVE_UNIFORM_MAX_LENGTH,
        &shader->max_uniform_name_len);

    dlb_vec_each(ta_shader_attribute *, attr, shader->attributes) {
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
    ta_shader_attribute *attr_tang =
        find_attribute_by_name(shader, SYM_ATTR_TANGENT,   TA_GLSL_VEC3);

    DLB_ASSERT(!attr_pos  || attr_pos->location  < 0 || attr_pos->location  == TA_SHADER_ATTR_POSITION);
    DLB_ASSERT(!attr_col  || attr_col->location  < 0 || attr_col->location  == TA_SHADER_ATTR_COLOR);
    DLB_ASSERT(!attr_uv   || attr_uv->location   < 0 || attr_uv->location   == TA_SHADER_ATTR_UV);
    DLB_ASSERT(!attr_norm || attr_norm->location < 0 || attr_norm->location == TA_SHADER_ATTR_NORMAL);
    DLB_ASSERT(!attr_tang || attr_tang->location < 0 || attr_tang->location == TA_SHADER_ATTR_TANGENT);

    shader_init_uniforms(shader, shader->uniforms);

#if _DEBUG
    shader_print_uniforms(shader);
#endif

    glDetachShader(program_id, vshader);
    glDetachShader(program_id, fshader);
    glDeleteShader(vshader);
    glDeleteShader(fshader);
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

void ta_shader_set_bool(ta_shader *shader, const char *name, GLboolean value)
{
    ta_shader_uniform *u = find_uniform_by_name(shader->uniforms, name, TA_GLSL_BOOL);
    if (u->value.glbool != value) {
        u->value.glbool = value;
        u->dirty = true;
    }
}

void ta_shader_set_int(ta_shader *shader, const char *name, GLint value)
{
    ta_shader_uniform *u = find_uniform_by_name(shader->uniforms, name, TA_GLSL_INT);
    if (u->value.glint != value) {
        u->value.glint = value;
        u->dirty = true;
    }
}

void ta_shader_set_uint(ta_shader *shader, const char *name, GLuint value)
{
    ta_shader_uniform *u = find_uniform_by_name(shader->uniforms, name, TA_GLSL_UINT);
    if (u->value.gluint != value) {
        u->value.gluint = value;
        u->dirty = true;
    }
}

void ta_shader_set_float(ta_shader *shader, const char *name, GLfloat value)
{
    ta_shader_uniform *u = find_uniform_by_name(shader->uniforms, name, TA_GLSL_FLOAT);
    if (u->value.glfloat != value) {
        u->value.glfloat = value;
        u->dirty = true;
    }
}

void ta_shader_set_sampler2d(ta_shader *shader, const char *name, GLuint tex_id)
{
    ta_shader_uniform *u = find_uniform_by_name(shader->uniforms, name, TA_GLSL_SAMPLER2D);
#if 0
    // NOTE: This doesn't work for some reason, probably related to tex_id == 0
    if (u->value.sampler2d != tex_id) {
        u->value.sampler2d = tex_id;
        u->dirty = true;
    }
#else
    u->value.sampler2d = tex_id;
    u->dirty = true;
#endif
}

void ta_shader_set_sampler_cube(ta_shader *shader, const char *name, GLuint tex_id)
{
    ta_shader_uniform *u = find_uniform_by_name(shader->uniforms, name, TA_GLSL_SAMPLER_CUBE);
#if 0
    // NOTE: This doesn't work for some reason, probably related to tex_id == 0
    if (u->value.sampler_cube != tex_id) {
        u->value.sampler_cube = tex_id;
        u->dirty = true;
    }
#else
    u->value.sampler_cube = tex_id;
    u->dirty = true;
#endif
}

void ta_shader_set_vec2(ta_shader *shader, const char *name, const ta_vec2 *v)
{
    ta_shader_uniform *u = find_uniform_by_name(shader->uniforms, name, TA_GLSL_VEC2);
    if (!vec2_equal(u->value.vec2, *v)) {
        u->value.vec2 = *v;
        u->dirty = true;
    }
}

void ta_shader_set_vec3(ta_shader *shader, const char *name, const ta_vec3 *v)
{
    ta_shader_uniform *u = find_uniform_by_name(shader->uniforms, name, TA_GLSL_VEC3);
    if (!vec3_equal(u->value.vec3, *v)) {
        u->value.vec3 = *v;
        u->dirty = true;
    }
}

void ta_shader_set_vec4(ta_shader *shader, const char *name, const ta_vec4 *v)
{
    ta_shader_uniform *u = find_uniform_by_name(shader->uniforms, name, TA_GLSL_VEC4);
    if (!vec4_equal(u->value.vec4, *v)) {
        u->value.vec4 = *v;
        u->dirty = true;
    }
}

void ta_shader_set_mat3(ta_shader *shader, const char *name, const ta_mat3 *m)
{
    ta_shader_uniform *u = find_uniform_by_name(shader->uniforms, name, TA_GLSL_MAT3);
    if (!mat3_equal(&u->value.mat3, m)) {
        u->value.mat3 = *m;
        u->dirty = true;
    }
}

void ta_shader_set_mat4(ta_shader *shader, const char *name, const ta_mat4 *m)
{
    ta_shader_uniform *u = find_uniform_by_name(shader->uniforms, name, TA_GLSL_MAT4);
    //u->value.mat4 = *m;
    //u->dirty = true;
    if (!mat4_equal(&u->value.mat4, m)) {
        u->value.mat4 = *m;
        u->dirty = true;
    }
}

void ta_shader_set_light(ta_shader *shader, const char *name, int index,
    ta_light *light)
{
    // TODO: Use the other set calls above to eliminate duplicate sets once
    // that's implemented for the basic types.
    ta_shader_uniform *u = find_uniform_by_name(shader->uniforms, name, TA_GLSL_STRUCT);

    ta_shader_uniform *u_intensity =
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
    ta_shader_uniform *u_cast_shadows =
        find_uniform_by_name(u->value.properties, SYM_U_LIGHTS_CAST_SHADOWS[index],
            TA_GLSL_BOOL);
    ta_shader_uniform *u_shadowmap2d =
        find_uniform_by_name(u->value.properties, SYM_U_LIGHTS_SHADOWMAP2D[index],
            TA_GLSL_SAMPLER2D);
    ta_shader_uniform *u_shadowmap3d =
        find_uniform_by_name(u->value.properties, SYM_U_LIGHTS_SHADOWMAP3D[index],
            TA_GLSL_SAMPLER_CUBE);
    ta_shader_uniform *u_shadowmap_zfar =
        find_uniform_by_name(u->value.properties, SYM_U_LIGHTS_SHADOWMAP_ZFAR[index],
            TA_GLSL_FLOAT);
    ta_shader_uniform *u_light_pv =
        find_uniform_by_name(u->value.properties, SYM_U_LIGHTS_LIGHT_PV[index],
            TA_GLSL_MAT4);

    u_intensity->value.glfloat        = light->data.common.intensity;
    u_color->value.rgb                = light->data.common.color;
    u_position->value.vec3            = ta_light_position(light);
    u_type->value.glint               = light->type;
    u_direction->value.vec3           = VEC3_ZERO;
    u_cast_shadows->value.glbool      = (GLboolean)light->cast_shadows;
    u_shadowmap2d->value.sampler2d    = 0;
    u_shadowmap3d->value.sampler_cube = 0;
    u_shadowmap_zfar->value.glfloat   = 0;
    u_light_pv->value.mat4            = MAT4_IDENT;

    switch (light->type) {
        case TA_LIGHT_AMBIENT:
            break;
        case TA_LIGHT_DIRECTIONAL:
            u_direction->value.vec3 = ta_light_direction(light);
            u_shadowmap2d->value.sampler2d = light->shadowmap.texture.gl_id;
            u_light_pv->value.mat4 = ta_light_pv(light);
            break;
        case TA_LIGHT_POINT:
            u_shadowmap3d->value.sampler_cube = light->shadowmap.texture.gl_id;
            u_shadowmap_zfar->value.glfloat = light->shadowmap.zfar;
            break;
        case TA_LIGHT_SPOT:
            u_direction->value.vec3 = ta_light_direction(light);
            u_shadowmap2d->value.sampler2d = light->shadowmap.texture.gl_id;
            DLB_ASSERT(!"Don't handle spot lights yet");
            break;
        default:
            DLB_ASSERT(!"Don't know how to initialize this type of light");
    }

    u->dirty = true;
    u_intensity->dirty = true;
    u_color->dirty = true;
    u_position->dirty = true;
    u_type->dirty = true;
    u_direction->dirty = true;
    u_cast_shadows->dirty = true;
    u_shadowmap2d->dirty = true;
    u_shadowmap3d->dirty = true;
    u_shadowmap_zfar->dirty = true;
    u_light_pv->dirty = true;
}

static void shader_store_uniforms(ta_shader_uniform *store,
    ta_shader_uniform *uniforms)
{
    dlb_vec_each(ta_shader_uniform *, u, uniforms) {
        if (u->location < 0 && u->type != TA_GLSL_STRUCT) {
            continue;
        }

        if (u->type == TA_GLSL_STRUCT) {
            shader_store_uniforms(store, u->value.properties);
        } else {
            dlb_vec_push(store, *u);
        }
    }
}

static void shader_bind_uniforms(ta_shader_uniform *uniforms, int *tex_count)
{
    dlb_vec_each(ta_shader_uniform *, u, uniforms) {
        if (!u->dirty || (u->location < 0 && u->type != TA_GLSL_STRUCT)) {
            continue;
        }

        switch (u->type) {
            case TA_GLSL_BOOL: {
                glUniform1i(u->location, u->value.glbool);
                break;
            } case TA_GLSL_INT: {
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
            } case TA_GLSL_SAMPLER_CUBE: {
                GLuint tex_id = u->value.gluint;
                if (tex_id >= 0) {
                    glActiveTexture(GL_TEXTURE0 + *tex_count);
                    glBindTexture(GL_TEXTURE_CUBE_MAP, tex_id);
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

        u->dirty = false;
    }
}

ta_shader_uniform *ta_shader_state_save(ta_shader *shader)
{
    ta_shader_uniform *store = 0;
    shader_store_uniforms(store, shader->uniforms);
    return store;
}

void ta_shader_state_load(ta_shader_uniform *uniforms)
{
    int tex_count = 0;
    shader_bind_uniforms(uniforms, &tex_count);
}

static GLuint bound_program_id = 0;
void ta_shader_bind(ta_shader *shader)
{
    DLB_ASSERT(shader->program_id);
    if (bound_program_id != shader->program_id) {
        glUseProgram(shader->program_id);
        bound_program_id = shader->program_id;
    }
    ta_shader_state_load(shader->uniforms);
}

void ta_shader_unbind()
{
#if 0  // TODO: Test turning this off
    glUseProgram(0);
    bound_program_id = 0;
#endif
}