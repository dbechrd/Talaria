#pragma once
#include "ta_math.h"
#include "misc/glad.h"

#define VERTEX_MAX_JOINTS 4

// TODO: This is an exact copy of ta_shader_attr.. do we really need both?
typedef enum ta_vertex_attrib_type {
    TA_VERTEX_ATTRIB_POSITION,
    TA_VERTEX_ATTRIB_COLOR,
    TA_VERTEX_ATTRIB_UV,
    TA_VERTEX_ATTRIB_NORMAL,
    TA_VERTEX_ATTRIB_TANGENT,
    TA_VERTEX_ATTRIB_MORPH0_POSITION,
    TA_VERTEX_ATTRIB_MORPH0_COLOR,
    TA_VERTEX_ATTRIB_MORPH0_UV,
    TA_VERTEX_ATTRIB_MORPH0_NORMAL,
    TA_VERTEX_ATTRIB_MORPH0_TANGENT,
    TA_VERTEX_ATTRIB_JOINTS,
    TA_VERTEX_ATTRIB_WEIGHTS,
    TA_VERTEX_ATTRIB_COUNT
} ta_vertex_attrib_type;

typedef struct ta_mesh_joint_array {
    GLushort ids[VERTEX_MAX_JOINTS];  // NOTE: GLTF only supports 4 joints per vertex
} ta_mesh_joint_array;

typedef struct ta_mesh_index_array {
    const char *material;  // material ID
    GLuint *values;        // vector of index values
    GLint base_vertex;
} ta_mesh_index_array;

#pragma warning(push)
#pragma warning(disable: 4201)
typedef struct ta_mesh {
    TA_RESOURCE_HEADER
    const char *path;
    ta_vec3 offset;
    union {
        // NOTE: Order of pointers must match enum
        struct {
            ta_vec3 *positions;
            ta_rgba *colors;
            ta_vec2 *uvs;
            ta_vec3 *normals;
            ta_vec3 *tangents;
            ta_vec3 *morph0_positions;
            ta_rgba *morph0_colors;
            ta_vec2 *morph0_uvs;
            ta_vec3 *morph0_normals;
            ta_vec3 *morph0_tangents;
            ta_mesh_joint_array *joints;
            ta_vec4 *weights;  // one weight for each joint
        };
        void *buffers[TA_VERTEX_ATTRIB_COUNT];
    };
    ta_mesh_index_array *indexes;
    ta_line_3d *vertex_normals;
    ta_line_3d *face_normals;
    ta_line_3d *tangent_lines;
    ta_aabb aabb;
    GLuint gl_vao;
    //GLuint gl_buffers[TA_VERTEX_ATTRIB_COUNT];
    GLuint gl_vertex_buffer;
    GLuint gl_index_buffer;
} ta_mesh;
#pragma warning(pop)

ta_mesh *tg_mesh_default;

const char *ta_vertex_attrib_type_str(int type);
void ta_mesh_init           (ta_mesh *mesh);
void ta_mesh_load_file      (ta_mesh *mesh, const char *filename);
void ta_mesh_create         (ta_mesh *mesh);
void ta_mesh_init_normals   (ta_mesh *mesh, float scale);
void ta_mesh_push_normals   (ta_mesh *mesh);
void ta_mesh_render         (ta_mesh *mesh);
void ta_mesh_free           (ta_mesh *mesh);