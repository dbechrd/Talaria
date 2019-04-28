#pragma once
#include "ta_buffer.h"
#include "ta_primitive.h"
#include "dlb_hash.h"
#include "misc/gl3w.h"

typedef enum {
	TA_MESH_QUEUE_STATIC,
	TA_MESH_QUEUE_LEVEL,
	TA_MESH_QUEUE_FRAME,
	TA_MESH_QUEUE_COUNT
} ta_mesh_queue;

typedef enum {
	TA_MESH_BUFFER_POSITION,
	TA_MESH_BUFFER_COLOR,
	TA_MESH_BUFFER_UV,
	TA_MESH_BUFFER_NORMAL,
	TA_MESH_BUFFER_INDEX,
	TA_MESH_BUFFER_COUNT
} ta_mesh_buffer;

typedef struct {
	const char *filename;
	ta_buffer name;

    GLuint *indexes;
    ta_vec3 *positions;
    ta_vec3 *normals;
    ta_uv *uvs;
    ta_rgba *colors;

    ta_line_3d *vertex_normals;
    ta_line_3d *face_normals;

	GLuint vao;
	GLuint buffers[TA_MESH_BUFFER_COUNT];
} ta_mesh;

extern dlb_hash tg_mesh_table;

void ta_mesh_load_obj_file(ta_mesh_queue queue, const char *filename);
void ta_mesh_init_vertex_normals(ta_mesh *mesh, float scale);
void ta_mesh_init_face_normals(ta_mesh *mesh, float scale);
void ta_mesh_push_normals(ta_mesh *mesh);
void debug_mesh_log_normals(ta_mesh *mesh);
void ta_mesh_free(ta_mesh *mesh);
void ta_mesh_clear(ta_mesh_queue queue);