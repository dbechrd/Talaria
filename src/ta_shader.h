#pragma once
#include "ta_math.h"
#include "misc/glad.h"

// TODO: Map DML string to enum to avoid confusing int values in DML
typedef enum ta_glsl_type {
    TA_GLSL_INT             = 0,
    TA_GLSL_UINT            = 1,
    TA_GLSL_FLOAT           = 2,
    TA_GLSL_SAMPLER2D       = 3,
    TA_GLSL_VEC2            = 4,
    TA_GLSL_VEC3            = 5,
    TA_GLSL_VEC4            = 6,
    TA_GLSL_MAT3            = 7,
    TA_GLSL_MAT4            = 8,
    TA_GLSL_STRUCT          = 9,
    TA_GLSL_SAMPLER_CUBE    = 10,  // TODO: Re-number these? Handle enums by string
    TA_GLSL_BOOL            = 11,
    TA_GLSL_SAMPLER2DARRAY  = 12,  // NOTE: This is an array texture sampler, not an array of sampler2Ds
    TA_GLSL_UINT_ARRAY      = 13,
} ta_glsl_type;

typedef enum ta_glsl_ubo_type {
    TA_GLSL_UBO_LIGHTS,
    TA_GLSL_UBO_MATERIALS,
    TA_GLSL_UBO_BONE_XFORMS,
    TA_GLSL_UBO_BONE_NORMAL_XFORMS,
    TA_GLSL_UBO_COUNT
} ta_glsl_ubo_type;

typedef struct ta_shader_attribute {
    const char      *name;      // [GL] vertex attribute name
    ta_glsl_type    type;       // [GL] vertex attribute type
    GLint           location;   // [GL] vertex attribute location
} ta_shader_attribute;

typedef struct ta_shader_uniform {
    const char      *name;      // [GL] uniform name
    ta_glsl_type    type;       // uniform value type
    union {                     // uniform value
        GLboolean   glbool;
        GLint       glint;
        GLuint      gluint;
        GLuint      *gluint_array;
        GLfloat     glfloat;
        ta_vec2     vec2;
        ta_vec3     vec3;
        ta_rgb      rgb;  // NOTE: Gets converted to vec3 in shader
        ta_vec4     vec4;
        ta_mat3     mat3;
        ta_mat4     mat4;
        GLuint      sampler_2d;
        GLuint      sampler_2darray;  // NOTE: This is an array texture sampler, not an array of sampler2Ds
        GLuint      sampler_cube;
        // NOTE: for structs, array of struct properties
        struct ta_shader_uniform *properties;
    } value;
    GLint   location;           // [GL] uniform location
    bool    dirty;              // true if uniform value has changed since last update on GPU
} ta_shader_uniform;

typedef struct ta_shader {
    TA_RESOURCE_HEADER
    const char          *path_vert;             // relative file path to vertex shader
    const char          *path_frag;             // relative file path to fragment shader
    GLint               max_attrib_name_len;    // [DEBUG] max length of a vertex attribute name
    GLint               max_uniform_name_len;   // [DEBUG] max length of a shader uniform name
    ta_shader_attribute *attributes;            // Array of vertex attributes
    ta_shader_uniform   *uniforms;              // Array of uniforms
    GLuint              program_id;             // [GL] shader program id
} ta_shader;

extern ta_shader *tg_shader_lines;
extern ta_shader *tg_shader_quads;
extern ta_shader *tg_shader_cubemap;

struct ta_light;
struct ta_material;

const char *ta_glsl_type_str                (int type);     // NOTE: Has to be int because it's used by ta_schema with other types
void ta_shader_init                         (ta_shader *shader);
void ta_shader_init_void                    (void *shader);
void ta_shader_load                         (ta_shader *shader);
void ta_shader_delete                       (ta_shader *shader);
void ta_shader_free                         (ta_shader *shader);
void ta_shader_free_void                    (void *shader);
ta_shader_attribute *find_attribute_by_name (ta_shader *shader, const char *name, ta_glsl_type type);
ta_shader_uniform *find_uniform_by_name_try (ta_shader_uniform *uniforms, const char *name, ta_glsl_type type);
ta_shader_uniform *find_uniform_by_name     (ta_shader_uniform *uniforms, const char *name, ta_glsl_type type);
void ta_shader_set_bool                     (ta_shader *shader, const char *name, GLboolean value);
void ta_shader_set_int                      (ta_shader *shader, const char *name, GLint value);
void ta_shader_set_int_try                  (ta_shader *shader, const char *name, GLint value);
void ta_shader_set_uint                     (ta_shader *shader, const char *name, GLuint value);
void ta_shader_set_uint_array               (ta_shader *shader, const char *name, GLuint *values);
void ta_shader_set_float                    (ta_shader *shader, const char *name, GLfloat value);
void ta_shader_set_sampler_2d               (ta_shader *shader, const char *name, GLuint tex_id);
void ta_shader_set_sampler_2d_array         (ta_shader *shader, const char *name, GLuint tex_id);
void ta_shader_set_sampler_cube             (ta_shader *shader, const char *name, GLuint tex_id);
void ta_shader_set_vec2                     (ta_shader *shader, const char *name, const ta_vec2 *v);
void ta_shader_set_vec3                     (ta_shader *shader, const char *name, const ta_vec3 *v);
void ta_shader_set_vec3_try                 (ta_shader *shader, const char *name, const ta_vec3 *v);
void ta_shader_set_vec4                     (ta_shader *shader, const char *name, const ta_vec4 *v);
void ta_shader_set_mat3                     (ta_shader *shader, const char *name, const ta_mat3 *m);
void ta_shader_set_mat4                     (ta_shader *shader, const char *name, const ta_mat4 *m);
void ta_shader_set_mat4_try                 (ta_shader *shader, const char *name, const ta_mat4 *m);
void ta_shader_set_light                    (ta_shader *shader, const char *name, int index, struct ta_light *light);
void ta_shader_set_material                 (ta_shader *shader, const char *name, struct ta_material *material);
void ta_shader_reset_pvm                    (ta_shader *shader);
void ta_shader_state_save                   (ta_shader *shader, ta_shader_uniform *store);
void ta_shader_state_load                   (ta_shader_uniform *uniforms);
void ta_shader_bind                         (ta_shader *shader);
void ta_shader_unbind                       ();