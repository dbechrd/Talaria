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
	GLuint vao;
	int vertex_count;
	int index_count;
	GLuint ui_arena[TA_MESH_BUFFER_COUNT];
} ta_mesh;

dlb_hash tg_mesh_table;

ta_mesh *ta_mesh_init(const char *name, ta_mesh_queue queue, GLuint *arr_index,
	ta_vec3 *arr_position, ta_vec3 *arr_normal, ta_uv *arr_uv,
	ta_color *arr_color);
void ta_mesh_load_obj_file(ta_mesh_queue queue, const char *filename);
void ta_mesh_clear(ta_mesh_queue queue);