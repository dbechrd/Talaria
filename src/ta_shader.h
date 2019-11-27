#pragma once
#include "ta_uid.h"
#include "ta_math.h"
#include "misc/gl3w.h"

typedef enum ta_shader_attr {
    TA_SHADER_ATTR_POSITION = 0,
    TA_SHADER_ATTR_COLOR    = 1,
    TA_SHADER_ATTR_UV       = 2,
    TA_SHADER_ATTR_NORMAL   = 3,
    TA_SHADER_ATTR_TANGENT  = 4,
    TA_SHADER_ATTR_COUNT
} ta_shader_attr;

// TODO: Map DML string to enum to avoid confusing int values in DML
typedef enum ta_glsl_type {
    TA_GLSL_INT          = 0,
    TA_GLSL_UINT         = 1,
    TA_GLSL_FLOAT        = 2,
    TA_GLSL_SAMPLER2D    = 3,
    TA_GLSL_VEC2         = 4,
    TA_GLSL_VEC3         = 5,
    TA_GLSL_VEC4         = 6,
    TA_GLSL_MAT3         = 7,
    TA_GLSL_MAT4         = 8,
    TA_GLSL_STRUCT       = 9,
    TA_GLSL_SAMPLER_CUBE = 10,  // TODO: Renumber these? Handle enums by string
    TA_GLSL_BOOL         = 11,
} ta_glsl_type;

typedef struct ta_shader_attribute {
    const char *name;
    ta_glsl_type type;
    GLint location;
} ta_shader_attribute;

typedef struct ta_shader_uniform {
    const char *name;
    ta_glsl_type type;
    union {
        GLboolean glbool;
        GLint glint;
        GLuint gluint;
        GLfloat glfloat;
        ta_vec2 vec2;
        ta_vec3 vec3; ta_rgb rgb;
        ta_vec4 vec4;
        ta_mat3 mat3;
        ta_mat4 mat4;
        GLuint sampler2d;
        GLuint sampler_cube;
        struct ta_shader_uniform *properties;  // for structs
    } value;
    GLint location;
    bool dirty;
} ta_shader_uniform;

typedef struct ta_shader {
    u32 index;
    const char *name;
    const char *path_vert;
    const char *path_frag;
    GLint max_attrib_name_len;
    GLint max_uniform_name_len;
    ta_shader_attribute *attributes;
    ta_shader_uniform *uniforms;
    GLuint program_id;
} ta_shader;

extern ta_shader *tg_shader_lines;
extern ta_shader *tg_shader_quads;
extern ta_shader *tg_shader_cubemap;
extern ta_shader *tg_shader_shadow;

struct ta_light;

const char *ta_glsl_type_str(int type);
void ta_shader_init(ta_shader *shader);
void ta_shader_load(ta_shader *shader);
void ta_shader_delete(ta_shader *shader);
void ta_shader_free(ta_shader *shader);
void ta_shader_set_bool(ta_shader *shader, const char *name, GLboolean value);
void ta_shader_set_int(ta_shader *shader, const char *name, GLint value);
void ta_shader_set_uint(ta_shader *shader, const char *name, GLuint value);
void ta_shader_set_float(ta_shader *shader, const char *name, GLfloat value);
void ta_shader_set_sampler2d(ta_shader *shader, const char *name, GLuint tex_id);
void ta_shader_set_sampler_cube(ta_shader *shader, const char *name, GLuint tex_id);
void ta_shader_set_vec2(ta_shader *shader, const char *name, const ta_vec2 *v);
void ta_shader_set_vec3(ta_shader *shader, const char *name, const ta_vec3 *v);
void ta_shader_set_vec4(ta_shader *shader, const char *name, const ta_vec4 *v);
void ta_shader_set_mat3(ta_shader *shader, const char *name, const ta_mat3 *m);
void ta_shader_set_mat4(ta_shader *shader, const char *name, const ta_mat4 *m);
void ta_shader_set_light(ta_shader *shader, const char *name, int index,
    struct ta_light *light);
ta_shader_uniform *ta_shader_state_save(ta_shader *shader);
void ta_shader_state_load(ta_shader_uniform *uniforms);
void ta_shader_bind(ta_shader *shader);
void ta_shader_unbind();