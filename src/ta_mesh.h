#pragma once
#include "ta_scene.h"
#include "ta_primitive.h"
#include "ta_rigid_body.h"
#include "misc/gl3w.h"

enum {
    TA_MESH_BUFFER_POSITION,
    TA_MESH_BUFFER_COLOR,
    TA_MESH_BUFFER_UV,
    TA_MESH_BUFFER_NORMAL,
    TA_MESH_BUFFER_INDEX,
    TA_MESH_BUFFER_COUNT
};

typedef struct ta_mesh_group_s ta_mesh_group;

typedef struct ta_mesh_s {
    ta_mesh_group *group;
    const char *name;

    GLuint *indexes;
    ta_vec3 *positions;
    ta_vec3 *normals;
    ta_uv *uvs;
    ta_rgba *colors;

    ta_line_3d *vertex_normals;
    ta_line_3d *face_normals;
    ta_aabb aabb;

	GLuint vao;
	GLuint buffers[TA_MESH_BUFFER_COUNT];
} ta_mesh;

void ta_mesh_create(ta_mesh *mesh);
void ta_mesh_init_normals(ta_mesh *mesh, float scale);
void ta_mesh_push_normals(ta_mesh *mesh);
void ta_mesh_log_normals_dbg(ta_mesh *mesh);
void ta_mesh_render(ta_mesh *mesh);
void ta_mesh_free(ta_mesh *mesh);