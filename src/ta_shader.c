#include "ta_file.h"
#include "ta_light.h"
#include "ta_log.h"
#include "ta_material.h"
#include "ta_shader.h"
#include "ta_symbol.h"
#include "dlb/dlb_memory.h"
#include "dlb/dlb_vector.h"
#include "misc/glad.h"

ta_shader *tg_shader_lines;
ta_shader *tg_shader_quads;
ta_shader *tg_shader_cubemap;

const char *ta_glsl_type_str(int type)
{
    switch(type) {
        case TA_GLSL_INT:            return "TA_GLSL_INT";
        case TA_GLSL_UINT:           return "TA_GLSL_UINT";
        case TA_GLSL_FLOAT:          return "TA_GLSL_FLOAT";
        case TA_GLSL_SAMPLER2D:      return "TA_GLSL_SAMPLER2D";
        case TA_GLSL_VEC2:           return "TA_GLSL_VEC2";
        case TA_GLSL_VEC3:           return "TA_GLSL_VEC3";
        case TA_GLSL_VEC4:           return "TA_GLSL_VEC4";
        case TA_GLSL_MAT3:           return "TA_GLSL_MAT3";
        case TA_GLSL_MAT4:           return "TA_GLSL_MAT4";
        case TA_GLSL_STRUCT:         return "TA_GLSL_STRUCT";
        case TA_GLSL_SAMPLER_CUBE:   return "TA_GLSL_SAMPLER_CUBE";
        case TA_GLSL_BOOL:           return "TA_GLSL_BOOL";
        case TA_GLSL_SAMPLER2DARRAY: return "TA_GLSL_SAMPLER2DARRAY";
        case TA_GLSL_UINT_ARRAY:     return "TA_GLSL_UINT_ARRAY";
        default: DLB_ASSERT(0);      return "TA_GLSL_???";
    }
}

void ta_shader_init(ta_shader *shader)
{
    // TODO: Idk where to put this sanity check, so just let it run per shader cus who cares for now
    // NOTE: Docs say GL_MAX_UNIFORM_BUFFER_BINDINGS must be at least 36
    DLB_ASSERT(TA_GLSL_UBO_COUNT <= 36);

    TracyCZone(ctxMethod, true);
    ta_shader_load(shader);
    TracyCZoneEnd(ctxMethod);
}
void ta_shader_init_void(void *shader)
{
    ta_shader_init(shader);
}

static GLuint ta_shader_compile(GLenum type, char *buf)
{
    TracyCZone(ctxMethod, true);
    GLuint shader = glCreateShader(type);

    // Read shader source
    GLint len = (GLint)dlb_vec_len(buf);
    // TODO(cleanup): void * (idk wtf OpenGL wants.. const bullshit)
    glShaderSource(shader, 1, (const GLchar *const *)&buf, &len);

    // Compile shader
    GLint status = 0;
    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (!status) {
        tg_debug_log.echo_stdout = true;
        ta_log_write(&tg_debug_log, SRC_SHADER, "Trying to show_info_log\n");

        GLint length = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
        // TODO: Turn this into a string
        GLenum gl_err = glGetError();
        if (length) {
            char *log = 0;
            dlb_vec_reserve(log, (size_t)length);
            dlb_vec_hdr(log)->len = length;
            glGetShaderInfoLog(shader, (GLsizei)dlb_vec_len(log), NULL, log);
            ta_log_write(&tg_debug_log, SRC_SHADER,
                "\n---[OpenGL Shader Info Log - %d]------------------------------------------------------------\n"
                "%s\n", gl_err, log);
            dlb_vec_free(log);
        } else {
            ta_log_write(&tg_debug_log, SRC_SHADER,
                "\n---[OpenGL Shader Info Log - %d]------------------------------------------------------------\n"
                "No error text: GL_INFO_LOG_LENGTH = 0\n", gl_err);
        }
        DLB_ASSERT(!"show_info_log: GL error occurred");

        glDeleteShader(shader);
        TracyCZoneEnd(ctxMethod);
        return 0;
    }

    TracyCZoneEnd(ctxMethod);
    return shader;
}
static GLuint ta_shader_compile_file(GLenum type, const char *filename)
{
    TracyCZone(ctxMethod, true);
    char *buf = ta_file_read_all(filename);
    GLint shader = ta_shader_compile(type, buf);
    if (!shader) {
        ta_log_write(&tg_debug_log, SRC_SHADER, "Failed to compile shader '%s'\n", filename);
    }
    dlb_vec_free(buf);
    TracyCZoneEnd(ctxMethod);
    return shader;
}
static void ta_shader_program_link(GLuint program)
{
    TracyCZone(ctxMethod, true);
    glLinkProgram(program);

    GLint status = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (!status)
    {
        tg_debug_log.echo_stdout = true;
        ta_log_write(&tg_debug_log, SRC_SHADER, "Trying to show_info_log\n");

        GLint length = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
        // TODO: Turn this into a string
        GLenum gl_err = glGetError();
        if (length) {
            char *log = 0;
            dlb_vec_reserve(log, (size_t)length);
            dlb_vec_hdr(log)->len = length;
            glGetProgramInfoLog(program, (GLsizei)dlb_vec_len(log), NULL, log);
            ta_log_write(&tg_debug_log, SRC_SHADER,
                "\n---[OpenGL Program Info Log - %d]------------------------------------------------------------\n"
                "%s\n", gl_err, log);
        } else {
            ta_log_write(&tg_debug_log, SRC_SHADER,
                "\n---[OpenGL Program Info Log - %d]------------------------------------------------------------\n"
                "No error text: GL_INFO_LOG_LENGTH = 0\n", gl_err);
        }
        DLB_ASSERT(!"show_info_log: GL error occurred");

        glDeleteProgram(program);
        DLB_ASSERT(!"ta_shader_program_link: failed to link shader program");
    }
    TracyCZoneEnd(ctxMethod);
}
static GLint ta_shader_attribute_location(ta_shader *shader, const char *name)
{
    TracyCZone(ctxMethod, true);
    GLint location = glGetAttribLocation(shader->program_id, name);
    if (location < 0) {
        // TODO: Log as warning
        ta_log_write(&tg_debug_log, SRC_SHADER,
            "Failed to locate attribute by '%s' in '%s'. "
            "Possibly optimized out.\n", name, shader->path_frag);
    }
    TracyCZoneEnd(ctxMethod);
    return location;
}
static GLint ta_shader_uniform_location(ta_shader *shader, const char *name)
{
    TracyCZone(ctxMethod, true);
    GLint location = glGetUniformLocation(shader->program_id, name);
    if (location < 0) {
        // TODO: Log as warning
        ta_log_write(&tg_debug_log, SRC_SHADER,
            "Failed to locate uniform '%s' in '%s'. "
            "Possibly optimized out.\n", name, shader->path_frag);
    }
    TracyCZoneEnd(ctxMethod);
    return location;
}
ta_shader_attribute *find_attribute_by_name(ta_shader *shader, const char *name, ta_glsl_type type)
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
ta_shader_uniform *find_uniform_by_name_try(ta_shader_uniform *uniforms, const char *name, ta_glsl_type type)
{
    ta_shader_uniform *result = 0;
    dlb_vec_each(ta_shader_uniform *, uniform, uniforms) {
        if (uniform->name == name) {
            result = uniform;
            DLB_ASSERT(result && result->type == type);
            break;
        }
    }
    return result;
}
ta_shader_uniform *find_uniform_by_name(ta_shader_uniform *uniforms, const char *name, ta_glsl_type type)
{
    ta_shader_uniform *result = find_uniform_by_name_try(uniforms, name, type);
    DLB_ASSERT(result && result->type == type);
    return result;
}
static void shader_print_uniforms(ta_shader *shader)
{
    TracyCZone(ctxMethod, true);
    GLint count = 0;
    glGetProgramiv(shader->program_id, GL_ACTIVE_UNIFORMS, &count);
    //ta_log_write(&tg_debug_log, SRC_SHADER, " Active uniforms: %d\n", count);

    GLsizei length = 0;
    GLint size = 0; // size of the variable
    GLenum type = 0; // type of the variable (float, vec3 or mat4, etc)
    GLchar *name = dlb_malloc(shader->max_uniform_name_len);
    for (GLint i = 0; i < count; i++) {
        glGetActiveUniform(shader->program_id, (GLuint)i, shader->max_uniform_name_len, &length, &size, &type, name);
        //ta_log_write(&tg_debug_log, SRC_SHADER, "   Uniform #%d Name: %s Type: %u_light\n", i, name, type);
    }
    dlb_free(name);
    TracyCZoneEnd(ctxMethod);
}
static void shader_init_uniforms(ta_shader *shader, ta_shader_uniform *uniforms)
{
    TracyCZone(ctxMethod, true);
    dlb_vec_each(ta_shader_uniform *, uniform, uniforms) {
        if (uniform->type == TA_GLSL_STRUCT) {
            shader_init_uniforms(shader, uniform->value.properties);
        } else {
            uniform->location = ta_shader_uniform_location(shader, uniform->name);
        }
        uniform->dirty = true;
    }
    TracyCZoneEnd(ctxMethod);
}
void ta_shader_load(ta_shader *shader)
{
    TracyCZone(ctxMethod, true);
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
    glBindAttribLocation(program_id, TA_VERTEX_ATTR_COLOR,           "attr_color");
    glBindAttribLocation(program_id, TA_VERTEX_ATTR_UV,              "attr_uv");
    glBindAttribLocation(program_id, TA_VERTEX_ATTR_POSITION,        "attr_position");
    glBindAttribLocation(program_id, TA_VERTEX_ATTR_NORMAL,          "attr_normal");
    glBindAttribLocation(program_id, TA_VERTEX_ATTR_TANGENT,         "attr_tangent");
    glBindAttribLocation(program_id, TA_VERTEX_ATTR_MORPH1_POSITION, "attr_morph1_position");
    glBindAttribLocation(program_id, TA_VERTEX_ATTR_MORPH1_NORMAL,   "attr_morph1_normal");
    glBindAttribLocation(program_id, TA_VERTEX_ATTR_MORPH1_TANGENT,  "attr_morph1_tangent");
    glBindAttribLocation(program_id, TA_VERTEX_ATTR_BONE_INDICES,    "attr_bones");
    glBindAttribLocation(program_id, TA_VERTEX_ATTR_BONE_WEIGHTS,    "attr_weights");
    ta_shader_program_link(program_id);

    glGetProgramiv(program_id, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &shader->max_attrib_name_len);
    glGetProgramiv(program_id, GL_ACTIVE_UNIFORM_MAX_LENGTH, &shader->max_uniform_name_len);

    dlb_vec_each(ta_shader_attribute *, attr, shader->attributes) {
        attr->location = ta_shader_attribute_location(shader, attr->name);
    }

    // Ensure vertex attributes are at the correct locations
    ta_shader_attribute *attr_col         = find_attribute_by_name(shader, SYM_ATTR_COLOR,           TA_GLSL_VEC4);
    ta_shader_attribute *attr_uv          = find_attribute_by_name(shader, SYM_ATTR_UV,              TA_GLSL_VEC2);
    ta_shader_attribute *attr_pos         = find_attribute_by_name(shader, SYM_ATTR_POSITION,        TA_GLSL_VEC3);
    ta_shader_attribute *attr_norm        = find_attribute_by_name(shader, SYM_ATTR_NORMAL,          TA_GLSL_VEC3);
    ta_shader_attribute *attr_tang        = find_attribute_by_name(shader, SYM_ATTR_TANGENT,         TA_GLSL_VEC3);
    ta_shader_attribute *attr_morph1_pos  = find_attribute_by_name(shader, SYM_ATTR_MORPH1_POSITION, TA_GLSL_VEC3);
    ta_shader_attribute *attr_morph1_norm = find_attribute_by_name(shader, SYM_ATTR_MORPH1_NORMAL,   TA_GLSL_VEC3);
    ta_shader_attribute *attr_morph1_tang = find_attribute_by_name(shader, SYM_ATTR_MORPH1_TANGENT,  TA_GLSL_VEC3);
    ta_shader_attribute *attr_bones       = find_attribute_by_name(shader, SYM_ATTR_BONES,          TA_GLSL_VEC4);
    ta_shader_attribute *attr_weights     = find_attribute_by_name(shader, SYM_ATTR_WEIGHTS,         TA_GLSL_VEC4);

    DLB_ASSERT(!attr_col         || attr_col->location         < 0 || attr_col->location         == TA_VERTEX_ATTR_COLOR);
    DLB_ASSERT(!attr_uv          || attr_uv->location          < 0 || attr_uv->location          == TA_VERTEX_ATTR_UV);
    DLB_ASSERT(!attr_pos         || attr_pos->location         < 0 || attr_pos->location         == TA_VERTEX_ATTR_POSITION);
    DLB_ASSERT(!attr_norm        || attr_norm->location        < 0 || attr_norm->location        == TA_VERTEX_ATTR_NORMAL);
    DLB_ASSERT(!attr_tang        || attr_tang->location        < 0 || attr_tang->location        == TA_VERTEX_ATTR_TANGENT);
    DLB_ASSERT(!attr_morph1_pos  || attr_morph1_pos->location  < 0 || attr_morph1_pos->location  == TA_VERTEX_ATTR_MORPH1_POSITION);
    DLB_ASSERT(!attr_morph1_norm || attr_morph1_norm->location < 0 || attr_morph1_norm->location == TA_VERTEX_ATTR_MORPH1_NORMAL);
    DLB_ASSERT(!attr_morph1_tang || attr_morph1_tang->location < 0 || attr_morph1_tang->location == TA_VERTEX_ATTR_MORPH1_TANGENT);
    DLB_ASSERT(!attr_bones       || attr_bones->location       < 0 || attr_bones->location       == TA_VERTEX_ATTR_BONE_INDICES);
    DLB_ASSERT(!attr_weights     || attr_weights->location     < 0 || attr_weights->location     == TA_VERTEX_ATTR_BONE_WEIGHTS);

    shader_init_uniforms(shader, shader->uniforms);

    // HACK: Test every shader for Lights UBO, and bind if present
    // TODO: This is bad if it fails for a shader that actually needs the Lights UBO :/
    GLuint ubo_lights_index = glGetUniformBlockIndex(shader->program_id, "ubo_lights");
    if (ubo_lights_index != GL_INVALID_INDEX) {
        glUniformBlockBinding(shader->program_id, ubo_lights_index, TA_GLSL_UBO_LIGHTS);
    }

    GLuint ubo_materials_index = glGetUniformBlockIndex(shader->program_id, "ubo_materials");
    if (ubo_materials_index != GL_INVALID_INDEX) {
        glUniformBlockBinding(shader->program_id, ubo_materials_index, TA_GLSL_UBO_MATERIALS);
    }

    GLuint ubo_bone_xforms_index = glGetUniformBlockIndex(shader->program_id, "ubo_bone_xforms");
    if (ubo_bone_xforms_index != GL_INVALID_INDEX) {
        glUniformBlockBinding(shader->program_id, ubo_bone_xforms_index, TA_GLSL_UBO_BONE_XFORMS);
    }

    GLuint ubo_bone_normal_xforms_index = glGetUniformBlockIndex(shader->program_id, "ubo_bone_normal_xforms");
    if (ubo_bone_normal_xforms_index != GL_INVALID_INDEX) {
        glUniformBlockBinding(shader->program_id, ubo_bone_normal_xforms_index, TA_GLSL_UBO_BONE_NORMAL_XFORMS);
    }

    if (find_uniform_by_name_try(shader->uniforms, SYM_U_TEXTURES[0], TA_GLSL_SAMPLER2DARRAY)) {
        size_t texture_pool_count = dlb_vec_len(tg_game.texturing.texture_pools);
        for (size_t i = 0; i < texture_pool_count; ++i) {
            ta_shader_set_sampler_2d_array(shader, SYM_U_TEXTURES[i], tg_game.texturing.texture_pools[i].gl_id);
        }
    }

#if _DEBUG
    shader_print_uniforms(shader);
#endif

    glDetachShader(program_id, vshader);
    glDetachShader(program_id, fshader);
    glDeleteShader(vshader);
    glDeleteShader(fshader);
    TracyCZoneEnd(ctxMethod);
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
    dlb_vec_each(ta_shader_uniform *, uniform, shader->uniforms) {
        dlb_vec_free(uniform->value.properties);
    }
    dlb_vec_free(shader->uniforms);
}
void ta_shader_free_void(void *shader)
{
    ta_shader_free(shader);
}
void ta_shader_set_bool(ta_shader *shader, const char *name, GLboolean value)
{
    ta_shader_uniform *u = find_uniform_by_name(shader->uniforms, name, TA_GLSL_BOOL);
    u->value.glbool = value;
    u->dirty = true;
}
void ta_shader_set_int(ta_shader *shader, const char *name, GLint value)
{
    ta_shader_uniform *u = find_uniform_by_name(shader->uniforms, name, TA_GLSL_INT);
    u->value.glint = value;
    u->dirty = true;
}
void ta_shader_set_int_try(ta_shader *shader, const char *name, GLint value)
{
    ta_shader_uniform *u = find_uniform_by_name_try(shader->uniforms, name, TA_GLSL_INT);
    if (u) {
        u->value.glint = value;
        u->dirty = true;
    }
}
void ta_shader_set_uint(ta_shader *shader, const char *name, GLuint value)
{
    ta_shader_uniform *u = find_uniform_by_name(shader->uniforms, name, TA_GLSL_UINT);
    u->value.gluint = value;
    u->dirty = true;
}
void ta_shader_set_uint_array(ta_shader *shader, const char *name, GLuint *values)
{
    ta_shader_uniform *u = find_uniform_by_name(shader->uniforms, name, TA_GLSL_UINT_ARRAY);
    dlb_vec_free(u->value.gluint_array);
    u->value.gluint_array = values;
    u->dirty = true;
}
void ta_shader_set_float(ta_shader *shader, const char *name, GLfloat value)
{
    ta_shader_uniform *u = find_uniform_by_name(shader->uniforms, name, TA_GLSL_FLOAT);
    u->value.glfloat = value;
    u->dirty = true;
}
void ta_shader_set_sampler_2d(ta_shader *shader, const char *name, GLuint tex_id)
{
    ta_shader_uniform *u = find_uniform_by_name(shader->uniforms, name, TA_GLSL_SAMPLER2D);
    u->value.sampler_2d = tex_id;
    u->dirty = true;
}
void ta_shader_set_sampler_2d_array(ta_shader *shader, const char *name, GLuint tex_id)
{
    ta_shader_uniform *u = find_uniform_by_name(shader->uniforms, name, TA_GLSL_SAMPLER2DARRAY);
    u->value.sampler_2darray = tex_id;
    u->dirty = true;
}
void ta_shader_set_sampler_cube(ta_shader *shader, const char *name, GLuint tex_id)
{
    ta_shader_uniform *u = find_uniform_by_name(shader->uniforms, name, TA_GLSL_SAMPLER_CUBE);
    u->value.sampler_cube = tex_id;
    u->dirty = true;
}
void ta_shader_set_vec2(ta_shader *shader, const char *name, const ta_vec2 *v)
{
    ta_shader_uniform *u = find_uniform_by_name(shader->uniforms, name, TA_GLSL_VEC2);
    u->value.vec2 = *v;
    u->dirty = true;
}
void ta_shader_set_vec3(ta_shader *shader, const char *name, const ta_vec3 *v)
{
    ta_shader_uniform *u = find_uniform_by_name(shader->uniforms, name, TA_GLSL_VEC3);
    u->value.vec3 = *v;
    u->dirty = true;
}
void ta_shader_set_vec3_try(ta_shader *shader, const char *name, const ta_vec3 *v)
{
    ta_shader_uniform *u = find_uniform_by_name_try(shader->uniforms, name, TA_GLSL_VEC3);
    if (u) {
        u->value.vec3 = *v;
        u->dirty = true;
    }
}
void ta_shader_set_vec4(ta_shader *shader, const char *name, const ta_vec4 *v)
{
    ta_shader_uniform *u = find_uniform_by_name(shader->uniforms, name, TA_GLSL_VEC4);
    u->value.vec4 = *v;
    u->dirty = true;
}
void ta_shader_set_mat3(ta_shader *shader, const char *name, const ta_mat3 *m)
{
    ta_shader_uniform *u = find_uniform_by_name(shader->uniforms, name, TA_GLSL_MAT3);
    u->value.mat3 = *m;
    u->dirty = true;
}
void ta_shader_set_mat4(ta_shader *shader, const char *name, const ta_mat4 *m)
{
    ta_shader_uniform *u = find_uniform_by_name(shader->uniforms, name, TA_GLSL_MAT4);
    u->value.mat4 = *m;
    u->dirty = true;
}
void ta_shader_set_mat4_try(ta_shader *shader, const char *name, const ta_mat4 *m)
{
    ta_shader_uniform *u = find_uniform_by_name_try(shader->uniforms, name, TA_GLSL_MAT4);
    if (u) {
        u->value.mat4 = *m;
        u->dirty = true;
    }
}
void ta_shader_set_light(ta_shader *shader, const char *name, int index, ta_light *light)
{
    TracyCZone(ctxMethod, true);
    // TODO: Use the other set calls above to eliminate duplicate sets once that's implemented for the basic types.
    ta_shader_uniform *u_light          = find_uniform_by_name(shader->uniforms, name, TA_GLSL_STRUCT);
    ta_shader_uniform *u_intensity      = find_uniform_by_name(u_light->value.properties, SYM_U_LIGHTS_INTENSITY[index],      TA_GLSL_FLOAT);
    ta_shader_uniform *u_position       = find_uniform_by_name(u_light->value.properties, SYM_U_LIGHTS_POSITION[index],       TA_GLSL_VEC3);
    ta_shader_uniform *u_color          = find_uniform_by_name(u_light->value.properties, SYM_U_LIGHTS_COLOR[index],          TA_GLSL_VEC3);
    ta_shader_uniform *u_type           = find_uniform_by_name(u_light->value.properties, SYM_U_LIGHTS_TYPE[index],           TA_GLSL_INT);
    ta_shader_uniform *u_direction      = find_uniform_by_name(u_light->value.properties, SYM_U_LIGHTS_DIRECTION[index],      TA_GLSL_VEC3);
    ta_shader_uniform *u_cast_shadows   = find_uniform_by_name(u_light->value.properties, SYM_U_LIGHTS_CAST_SHADOWS[index],   TA_GLSL_BOOL);
    ta_shader_uniform *u_light_pv       = find_uniform_by_name(u_light->value.properties, SYM_U_LIGHTS_LIGHT_PV[index],       TA_GLSL_MAT4);
    ta_shader_uniform *u_shadowmap_zfar = find_uniform_by_name(u_light->value.properties, SYM_U_LIGHTS_SHADOWMAP_ZFAR[index], TA_GLSL_FLOAT);
    ta_shader_uniform *u_shadowmap_texture_pool_index   = find_uniform_by_name(u_light->value.properties, SYM_U_LIGHTS_SHADOWMAP_TEXTURE_POOL_INDEX[index],   TA_GLSL_UINT);
    ta_shader_uniform *u_shadowmap_texture_array_layers = find_uniform_by_name(u_light->value.properties, SYM_U_LIGHTS_SHADOWMAP_TEXTURE_ARRAY_LAYERS[index], TA_GLSL_UINT_ARRAY);

    // Set default values (some are overridden for specific light types below)
    u_intensity->value.glfloat        = light->intensity;
    u_color->value.rgb                = light->color;
    u_position->value.vec3            = ta_light_position(light);
    u_type->value.glint               = light->type;
    u_direction->value.vec3           = VEC3_ZERO;
    u_cast_shadows->value.glbool      = false;
    u_light_pv->value.mat4            = MAT4_IDENT;
    u_shadowmap_zfar->value.glfloat   = 0;
    u_shadowmap_texture_pool_index->value.gluint         = 0;
    u_shadowmap_texture_array_layers->value.gluint_array = 0;

    // Light type-dependent properties
    switch (light->type) {
        case TA_LIGHT_AMBIENT: {
            break;
        } case TA_LIGHT_DIRECTIONAL: {
            u_direction->value.vec3         = ta_light_direction(light);
            u_cast_shadows->value.glbool    = (GLboolean)!light->data.directional.no_shadow_cast;
            u_light_pv->value.mat4          = ta_light_pv(light);

            ta_texture *tex = (ta_texture *)ta_game_by_sym(RES_TEXTURE, light->data.directional.shadow_map);
            u_shadowmap_texture_pool_index->value.gluint = tex->gl_texture_pool_index;
            dlb_vec_push(u_shadowmap_texture_array_layers->value.gluint_array, tex->gl_texture_pool_layer);
            break;
        } case TA_LIGHT_POINT: {
            // NOTE: Use sampler_2d and assume that all 6 cubemap face textures are contiguous in the texture pool
            // TODO: We probably need to store pool index and layer rather than just gl_id to figure out "contiguous"
            u_cast_shadows->value.glbool    = (GLboolean)!light->data.point.no_shadow_cast;
            u_shadowmap_zfar->value.glfloat = light->data.point.shadow_properties.zfar;

            // NOTE: Assume all textures are in the same pool (asserts)
            ta_texture *first_tex = (ta_texture *)ta_game_by_sym(RES_TEXTURE, light->data.point.shadow_map.textures[0]);
            u_shadowmap_texture_pool_index->value.gluint = first_tex->gl_texture_pool_index;
            for (int i = 0; i < 6; i++) {
                ta_texture *tex = (ta_texture *)ta_game_by_sym(RES_TEXTURE, light->data.point.shadow_map.textures[i]);
                dlb_vec_push(u_shadowmap_texture_array_layers->value.gluint_array, tex->gl_texture_pool_layer);
                DLB_ASSERT(tex->gl_texture_pool_index == u_shadowmap_texture_pool_index->value.gluint);
            }
            break;
        } case TA_LIGHT_SPOT: {
            u_direction->value.vec3         = ta_light_direction(light);
            u_cast_shadows->value.glbool    = (GLboolean)!light->data.spot.no_shadow_cast;

            ta_texture *tex = (ta_texture *)ta_game_by_sym(RES_TEXTURE, light->data.spot.shadow_map);
            u_shadowmap_texture_pool_index->value.gluint = tex->gl_texture_pool_index;
            dlb_vec_push(u_shadowmap_texture_array_layers->value.gluint_array, tex->gl_texture_pool_layer);
            DLB_ASSERT(!"Don't handle spot lights yet");
            break;
        } default: {
            DLB_ASSERT(!"Don't know how to initialize this type of light");
        }
    }

    // Mark all light uniforms as dirty
    u_light->dirty = true;
    u_intensity->dirty = true;
    u_color->dirty = true;
    u_position->dirty = true;
    u_type->dirty = true;
    u_direction->dirty = true;
    u_cast_shadows->dirty = true;
    u_light_pv->dirty = true;
    u_shadowmap_zfar->dirty = true;
    u_shadowmap_texture_pool_index->dirty = true;
    u_shadowmap_texture_array_layers->dirty = true;
    TracyCZoneEnd(ctxMethod);
}
void ta_shader_set_material(ta_shader *shader, const char *name, ta_material *material)
{
    TracyCZone(ctxMethod, true);
    ta_texture *albedo_texture    = (ta_texture *)ta_game_by_sym_try(RES_TEXTURE, material->albedo_texture);
    ta_texture *emission_texture  = (ta_texture *)ta_game_by_sym_try(RES_TEXTURE, material->emission_texture);
    ta_texture *height_texture    = (ta_texture *)ta_game_by_sym_try(RES_TEXTURE, material->height_texture);
    ta_texture *metallic_texture  = (ta_texture *)ta_game_by_sym_try(RES_TEXTURE, material->metallic_texture);
    ta_texture *normal_texture    = (ta_texture *)ta_game_by_sym_try(RES_TEXTURE, material->normal_texture);
    ta_texture *occlusion_texture = (ta_texture *)ta_game_by_sym_try(RES_TEXTURE, material->occlusion_texture);
    ta_texture *roughness_texture = (ta_texture *)ta_game_by_sym_try(RES_TEXTURE, material->roughness_texture);

    // NOTE: Seems dumb to bind a texture only for the multiplication factor to be 0.0, right?
    DLB_ASSERT(material->albedo_factor.a);
    if (emission_texture)  { DLB_ASSERT(material->emission_factor.r || material->emission_factor.g || material->emission_factor.b); }
    if (height_texture)    { DLB_ASSERT(material->height_factor); }
    if (metallic_texture)  { DLB_ASSERT(material->metallic_factor); }
    if (roughness_texture) { DLB_ASSERT(material->roughness_factor); }

    if (!albedo_texture   ) { albedo_texture    = ta_game_by_sym_try(RES_TEXTURE, tg_tex_default_albedo); }
    if (!emission_texture ) { emission_texture  = ta_game_by_sym_try(RES_TEXTURE, tg_tex_default_emission); }
    if (!height_texture   ) { height_texture    = ta_game_by_sym_try(RES_TEXTURE, tg_tex_default_height); }
    if (!metallic_texture ) { metallic_texture  = ta_game_by_sym_try(RES_TEXTURE, tg_tex_default_metallic); }
    if (!normal_texture   ) { normal_texture    = ta_game_by_sym_try(RES_TEXTURE, tg_tex_default_normal); }
    if (!occlusion_texture) { occlusion_texture = ta_game_by_sym_try(RES_TEXTURE, tg_tex_default_occlusion); }
    if (!roughness_texture) { roughness_texture = ta_game_by_sym_try(RES_TEXTURE, tg_tex_default_roughness); }

    DLB_ASSERT(albedo_texture    && albedo_texture   ->type == TA_TEXTURE_2D_ARRAY);
    DLB_ASSERT(emission_texture  && emission_texture ->type == TA_TEXTURE_2D_ARRAY);
    DLB_ASSERT(height_texture    && height_texture   ->type == TA_TEXTURE_2D_ARRAY);
    DLB_ASSERT(metallic_texture  && metallic_texture ->type == TA_TEXTURE_2D_ARRAY);
    DLB_ASSERT(normal_texture    && normal_texture   ->type == TA_TEXTURE_2D_ARRAY);
    DLB_ASSERT(occlusion_texture && occlusion_texture->type == TA_TEXTURE_2D_ARRAY);
    DLB_ASSERT(roughness_texture && roughness_texture->type == TA_TEXTURE_2D_ARRAY);

    // TODO: Use material UBO
    ta_shader_uniform *u_material                              = find_uniform_by_name(shader->uniforms, name, TA_GLSL_STRUCT);
    ta_shader_uniform *u_material_albedo_texture_pool_index    = find_uniform_by_name(u_material->value.properties, SYM_U_MATERIAL_ALBEDO_TEXTURE_POOL_INDEX,    TA_GLSL_UINT);
    ta_shader_uniform *u_material_albedo_texture_pool_layer    = find_uniform_by_name(u_material->value.properties, SYM_U_MATERIAL_ALBEDO_TEXTURE_POOL_LAYER,    TA_GLSL_UINT);
    ta_shader_uniform *u_material_albedo_factor                = find_uniform_by_name(u_material->value.properties, SYM_U_MATERIAL_ALBEDO_FACTOR,                TA_GLSL_VEC4);
    ta_shader_uniform *u_material_emission_texture_pool_index  = find_uniform_by_name(u_material->value.properties, SYM_U_MATERIAL_EMISSION_TEXTURE_POOL_INDEX,  TA_GLSL_UINT);
    ta_shader_uniform *u_material_emission_texture_pool_layer  = find_uniform_by_name(u_material->value.properties, SYM_U_MATERIAL_EMISSION_TEXTURE_POOL_LAYER,  TA_GLSL_UINT);
    ta_shader_uniform *u_material_emission_factor              = find_uniform_by_name(u_material->value.properties, SYM_U_MATERIAL_EMISSION_FACTOR,              TA_GLSL_VEC3);
    ta_shader_uniform *u_material_height_texture_pool_index    = find_uniform_by_name(u_material->value.properties, SYM_U_MATERIAL_HEIGHT_TEXTURE_POOL_INDEX,    TA_GLSL_UINT);
    ta_shader_uniform *u_material_height_texture_pool_layer    = find_uniform_by_name(u_material->value.properties, SYM_U_MATERIAL_HEIGHT_TEXTURE_POOL_LAYER,    TA_GLSL_UINT);
    ta_shader_uniform *u_material_height_factor                = find_uniform_by_name(u_material->value.properties, SYM_U_MATERIAL_HEIGHT_FACTOR,                TA_GLSL_FLOAT);
    ta_shader_uniform *u_material_metallic_texture_pool_index  = find_uniform_by_name(u_material->value.properties, SYM_U_MATERIAL_METALLIC_TEXTURE_POOL_INDEX,  TA_GLSL_UINT);
    ta_shader_uniform *u_material_metallic_texture_pool_layer  = find_uniform_by_name(u_material->value.properties, SYM_U_MATERIAL_METALLIC_TEXTURE_POOL_LAYER,  TA_GLSL_UINT);
    ta_shader_uniform *u_material_metallic_factor              = find_uniform_by_name(u_material->value.properties, SYM_U_MATERIAL_METALLIC_FACTOR,              TA_GLSL_FLOAT);
    ta_shader_uniform *u_material_normal_texture_pool_index    = find_uniform_by_name(u_material->value.properties, SYM_U_MATERIAL_NORMAL_TEXTURE_POOL_INDEX,    TA_GLSL_UINT);
    ta_shader_uniform *u_material_normal_texture_pool_layer    = find_uniform_by_name(u_material->value.properties, SYM_U_MATERIAL_NORMAL_TEXTURE_POOL_LAYER,    TA_GLSL_UINT);
    ta_shader_uniform *u_material_occlusion_texture_pool_index = find_uniform_by_name(u_material->value.properties, SYM_U_MATERIAL_OCCLUSION_TEXTURE_POOL_INDEX, TA_GLSL_UINT);
    ta_shader_uniform *u_material_occlusion_texture_pool_layer = find_uniform_by_name(u_material->value.properties, SYM_U_MATERIAL_OCCLUSION_TEXTURE_POOL_LAYER, TA_GLSL_UINT);
    ta_shader_uniform *u_material_roughness_texture_pool_index = find_uniform_by_name(u_material->value.properties, SYM_U_MATERIAL_ROUGHNESS_TEXTURE_POOL_INDEX, TA_GLSL_UINT);
    ta_shader_uniform *u_material_roughness_texture_pool_layer = find_uniform_by_name(u_material->value.properties, SYM_U_MATERIAL_ROUGHNESS_TEXTURE_POOL_LAYER, TA_GLSL_UINT);
    ta_shader_uniform *u_material_roughness_factor             = find_uniform_by_name(u_material->value.properties, SYM_U_MATERIAL_ROUGHNESS_FACTOR,             TA_GLSL_FLOAT);

    // Set uniform values
    u_material_albedo_texture_pool_index    ->value.gluint  = albedo_texture->gl_texture_pool_index;
    u_material_albedo_texture_pool_layer    ->value.gluint  = albedo_texture->gl_texture_pool_layer;
    u_material_albedo_factor                ->value.vec4    = *(ta_vec4 *)&material->albedo_factor;
    u_material_emission_texture_pool_index  ->value.gluint  = emission_texture->gl_texture_pool_index;
    u_material_emission_texture_pool_layer  ->value.gluint  = emission_texture->gl_texture_pool_layer;
    u_material_emission_factor              ->value.vec3    = *(ta_vec3 *)&material->emission_factor;
    u_material_height_texture_pool_index    ->value.gluint  = height_texture->gl_texture_pool_index;
    u_material_height_texture_pool_layer    ->value.gluint  = height_texture->gl_texture_pool_layer;
    u_material_height_factor                ->value.glfloat = material->height_factor;
    u_material_metallic_texture_pool_index  ->value.gluint  = metallic_texture->gl_texture_pool_index;
    u_material_metallic_texture_pool_layer  ->value.gluint  = metallic_texture->gl_texture_pool_layer;
    u_material_metallic_factor              ->value.glfloat = material->metallic_factor;
    u_material_normal_texture_pool_index    ->value.gluint  = normal_texture->gl_texture_pool_index;
    u_material_normal_texture_pool_layer    ->value.gluint  = normal_texture->gl_texture_pool_layer;
    u_material_occlusion_texture_pool_index ->value.gluint  = occlusion_texture->gl_texture_pool_index;
    u_material_occlusion_texture_pool_layer ->value.gluint  = occlusion_texture->gl_texture_pool_layer;
    u_material_roughness_texture_pool_index ->value.gluint  = roughness_texture->gl_texture_pool_index;
    u_material_roughness_texture_pool_layer ->value.gluint  = roughness_texture->gl_texture_pool_layer;
    u_material_roughness_factor             ->value.glfloat = material->roughness_factor;

    // Mark all material uniforms as dirty
    u_material                             ->dirty = true;
    u_material_albedo_texture_pool_index   ->dirty = true;
    u_material_albedo_texture_pool_layer   ->dirty = true;
    u_material_albedo_factor               ->dirty = true;
    u_material_emission_texture_pool_index ->dirty = true;
    u_material_emission_texture_pool_layer ->dirty = true;
    u_material_emission_factor             ->dirty = true;
    u_material_height_texture_pool_index   ->dirty = true;
    u_material_height_texture_pool_layer   ->dirty = true;
    u_material_height_factor               ->dirty = true;
    u_material_metallic_texture_pool_index ->dirty = true;
    u_material_metallic_texture_pool_layer ->dirty = true;
    u_material_metallic_factor             ->dirty = true;
    u_material_normal_texture_pool_index   ->dirty = true;
    u_material_normal_texture_pool_layer   ->dirty = true;
    u_material_occlusion_texture_pool_index->dirty = true;
    u_material_occlusion_texture_pool_layer->dirty = true;
    u_material_roughness_texture_pool_index->dirty = true;
    u_material_roughness_texture_pool_layer->dirty = true;
    u_material_roughness_factor            ->dirty = true;
    TracyCZoneEnd(ctxMethod);
}
void ta_shader_reset_pvm(ta_shader *shader)
{
    ta_shader_set_mat4_try(shader, SYM_U_PROJ, &MAT4_IDENT);
    ta_shader_set_mat4_try(shader, SYM_U_VIEW, &MAT4_IDENT);
    ta_shader_set_mat4_try(shader, SYM_U_MODEL, &MAT4_IDENT);
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
        if ((u->location < 0 && u->type != TA_GLSL_STRUCT)) {
            continue;
        }
        if (!u->dirty) {
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
            } case TA_GLSL_UINT_ARRAY: {
                glUniform1uiv(u->location, (GLsizei)dlb_vec_len(u->value.gluint_array), u->value.gluint_array);
                break;
            } case TA_GLSL_FLOAT: {
                glUniform1f(u->location, u->value.glfloat);
                break;
            } case TA_GLSL_SAMPLER2D: {
                GLuint tex_id = u->value.sampler_2d;
                if (tex_id >= 0) {
                    glActiveTexture(GL_TEXTURE0 + *tex_count);
                    glBindTexture(GL_TEXTURE_2D, tex_id);
                    glUniform1i(u->location, *tex_count);
                    (*tex_count)++;
                }
                break;
            } case TA_GLSL_SAMPLER2DARRAY: {
                GLuint tex_id = u->value.sampler_2darray;
                if (tex_id >= 0) {
                    glActiveTexture(GL_TEXTURE0 + *tex_count);
                    glBindTexture(GL_TEXTURE_2D_ARRAY, tex_id);
                    glUniform1i(u->location, *tex_count);
                    (*tex_count)++;
                }
                break;
            } case TA_GLSL_SAMPLER_CUBE: {
                GLuint tex_id = u->value.sampler_cube;
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

void ta_shader_state_save(ta_shader *shader, ta_shader_uniform *store)
{
    TracyCZone(ctxMethod, true);
    shader_store_uniforms(store, shader->uniforms);
    TracyCZoneEnd(ctxMethod);
}
void ta_shader_state_load(ta_shader_uniform *uniforms)
{
    TracyCZone(ctxMethod, true);
    int tex_count = 0;
    shader_bind_uniforms(uniforms, &tex_count);
    TracyCZoneEnd(ctxMethod);
}

static GLuint shader_bound_program_id = 0;
void ta_shader_bind(ta_shader *shader)
{
    TracyCZone(ctxMethod, true);
    DLB_ASSERT(shader->program_id);
    if (shader_bound_program_id != shader->program_id) {
        TracyCZoneN(ctxUseProgram, "glUseProgram", true);
        glUseProgram(shader->program_id);
        shader_bound_program_id = shader->program_id;
        TracyCZoneEnd(ctxUseProgram);
    }
    ta_shader_state_load(shader->uniforms);
    TracyCZoneEnd(ctxMethod);
}
void ta_shader_unbind()
{
#if 0  // TODO: Test turning this off
    glUseProgram(0);
    bound_program_id = 0;
#endif
}