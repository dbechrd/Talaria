#pragma once
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

// TODO: Map DML string to enum at runtime to use switch instead of if/else?
#if 0
typedef enum {
    TA_SHADER_UNIFORM_UINT,
    TA_SHADER_UNIFORM_MAT4,
    TA_SHADER_UNIFORM_TEXTURE,
} ta_shader_variable_type;
#endif

typedef struct ta_shader_attribute_s {
    const char *name;
    int location;
    const char *type;
} ta_shader_attribute;

typedef struct ta_shader_uniform_mat4_s {
    ta_mat4 matrix;
} ta_shader_uniform_mat4;

typedef struct ta_shader_uniform_sampler2d_s {
    GLuint texture_id;
} ta_shader_uniform_sampler2d;

typedef struct ta_shader_uniform_s {
    const char *name;
    GLint location;
    const char *type;
    union {
        ta_shader_uniform_mat4 mat4;
        ta_shader_uniform_sampler2d sampler2d;
    } value;
} ta_shader_uniform;

typedef struct ta_scene_s ta_scene;

typedef struct ta_shader_s {
    ta_scene *scene;
    const char *uid;
    const char *path_vert;
    const char *path_frag;
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
extern ta_shader *tg_shader_mesh;

void ta_shader_init(ta_shader *shader, const char *path_vert,
    const char *path_frag);
void ta_shader_create(ta_shader *shader);
void ta_shader_delete(ta_shader *shader);
void ta_shader_free(ta_shader *shader);
void ta_shader_bind(ta_shader *shader);
void ta_shader_unbind(ta_shader *shader);
void ta_shader_set_mat4(ta_shader *shader, const char *name,
    const ta_mat4 *matrix);
void ta_shader_set_sampler2d(ta_shader *shader, const char *name,
    GLuint texture_id);
void ta_shader_prerender(ta_shader *shader);