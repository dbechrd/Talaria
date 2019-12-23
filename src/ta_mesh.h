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

typedef struct ta_mesh {
    u32 index;
    const char *name;
    GLuint *indexes;
    u32 indexes_count;
    ta_vec3 *positions;
    u32 positions_count;
    ta_vec2 *uvs;
    u32 uvs_count;
    ta_rgba *colors;
    u32 colors_count;
    ta_vec3 *normals;
    u32 normals_count;
    ta_vec3 *tangents;
    u32 tangents_count;
    ta_line_3d *vertex_normals;
    ta_line_3d *face_normals;
    ta_line_3d *tangent_lines;
    ta_aabb aabb;

    GLuint vao;
    GLuint buffers[TA_MESH_BUFFER_COUNT];
} ta_mesh;

void ta_mesh_create(ta_mesh *mesh);
void ta_mesh_init_normals(ta_mesh *mesh, float scale);
void ta_mesh_push_normals(ta_mesh *mesh);
void ta_mesh_render(ta_mesh *mesh);
void ta_mesh_free(ta_mesh *mesh);