#pragma once
#include "ta_scene.h"
#include "ta_file.h"
#include "ta_math.h"
#include "misc/gl3w.h"

typedef enum {
	TA_SHADER_ATTR_POSITION = 0,
	TA_SHADER_ATTR_COLOR    = 1,
	TA_SHADER_ATTR_UV       = 2,
	TA_SHADER_ATTR_NORMAL   = 3,
	TA_SHADER_ATTR_COUNT
} ta_shader_attr;

// TODO: Map DML string to enum to avoid confusing int values in DML
typedef enum {
    TA_GLSL_GLINT     = 0,
    TA_GLSL_GLUINT    = 1,
    TA_GLSL_SAMPLER2D = 2,
    TA_GLSL_VEC2      = 3,
    TA_GLSL_VEC3      = 4,
    TA_GLSL_VEC4      = 5,
    TA_GLSL_MAT3      = 6,
    TA_GLSL_MAT4      = 7,
    TA_GLSL_STRUCT    = 8,
} ta_glsl_type;

typedef struct ta_shader_attribute_s {
    const char *name;
    ta_glsl_type type;
    GLint location;
} ta_shader_attribute;

typedef struct ta_shader_uniform_s ta_shader_uniform;
typedef struct ta_shader_uniform_s {
    const char *name;
    ta_glsl_type type;
    union {
        GLint glint;
        GLuint gluint;
        ta_vec2 vec2;
        ta_vec3 vec3;
        ta_vec4 vec4;
        ta_mat3 mat3;
        ta_mat4 mat4;
        GLuint sampler2d;
        ta_shader_uniform *properties;  // for structs
    } value;
    GLint location;
} ta_shader_uniform;

typedef struct ta_shader_s {
    ta_scene *scene;
    const char *uid;
    const char *path_vert;
    const char *path_frag;
    GLint max_attrib_name_len;
    GLint max_uniform_name_len;
    ta_shader_attribute *attributes;
    ta_shader_uniform *uniforms;
    GLuint program_id;
} ta_shader;

////////////////////////////////////////////////////////////////////////////////
// TODO: Cleanup
////////////////////////////////////////////////////////////////////////////////
typedef struct {
    ta_vec3 position;
    ta_rgba color;
} ta_shader_lines_vertex;

typedef struct {
    ta_shader_lines_vertex verts[2];
} ta_vert_line;
////////////////////////////////////////////////////////////////////////////////
typedef struct {
    ta_vec3 position;
    ta_rgba color;
    ta_uv uv;
} ta_shader_quads_vertex;

typedef struct {
    ta_shader_quads_vertex verts[6];
} ta_vert_quad;
////////////////////////////////////////////////////////////////////////////////

extern ta_shader *tg_shader_lines;
extern ta_shader *tg_shader_quads;

const char *ta_glsl_type_str(int type);
void ta_shader_init(ta_shader *shader, const char *path_vert,
    const char *path_frag);
void ta_shader_create(ta_shader *shader);
void ta_shader_delete(ta_shader *shader);
void ta_shader_free(ta_shader *shader);
void ta_shader_bind(ta_shader *shader);
void ta_shader_unbind(ta_shader *shader);
void ta_shader_set_vec3(ta_shader *shader, const char *name, const ta_vec3 *v);
void ta_shader_set_vec4(ta_shader *shader, const char *name, const ta_vec4 *v);
void ta_shader_set_mat4(ta_shader *shader, const char *name, const ta_mat4 *m);
void ta_shader_set_sampler2d(ta_shader *shader, const char *name, GLuint tex_id);
void ta_shader_set_light(ta_shader *shader, const char *name, ta_light *light);
void ta_shader_prerender(ta_shader *shader);