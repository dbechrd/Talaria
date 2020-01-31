#pragma once
#include "ta_uid.h"
#include "ta_math.h"
#include "misc/gl3w.h"

enum {
    TA_MESH_BUFFER_POSITION,
    TA_MESH_BUFFER_COLOR,
    TA_MESH_BUFFER_UV,
    TA_MESH_BUFFER_NORMAL,
    TA_MESH_BUFFER_TANGENT,
    TA_MESH_BUFFER_INDEX,
    TA_MESH_BUFFER_COUNT
};

#pragma warning(push)
#pragma warning(disable: 4201)
typedef struct ta_mesh {
    size_t index;
    const char *name;
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
            GLuint *indexes;
        };
        void *buffers[TA_MESH_BUFFER_COUNT];
    };
    ta_line_3d *vertex_normals;
    ta_line_3d *face_normals;
    ta_line_3d *tangent_lines;
    ta_aabb aabb;
    GLuint gl_vao;
    GLuint gl_buffers[TA_MESH_BUFFER_COUNT];
} ta_mesh;
#pragma warning(pop)

ta_mesh *tg_mesh_default;

void ta_mesh_init(ta_mesh *mesh);
void ta_mesh_load_file(ta_mesh *mesh, const char *filename);
void ta_mesh_create(ta_mesh *mesh);
void ta_mesh_init_normals(ta_mesh *mesh, float scale);
void ta_mesh_push_normals(ta_mesh *mesh);
void ta_mesh_render(ta_mesh *mesh);
void ta_mesh_free(ta_mesh *mesh);