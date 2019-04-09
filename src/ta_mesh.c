#include "ta_mesh.h"
#include "ta_log.h"
#include "ta_primitive.h"
#include "ta_shader.h"
#include "dlb_types.h"
#include "dlb_memory.h"
#include "dlb_vector.h"
#include "dlb_hash.h"
#include "misc/gl3w.h"

#define TINYOBJ_MALLOC dlb_malloc
#define TINYOBJ_CALLOC dlb_calloc
#define TINYOBJ_REALLOC dlb_realloc
#define TINYOBJ_FREE dlb_free
#define TINYOBJ_LOADER_C_IMPLEMENTATION
#include "misc/tinyobj_loader_c.h"

dlb_hash tg_mesh_table;

static ta_mesh *meshes[TA_MESH_QUEUE_COUNT];
static GLuint *gl_ids[TA_MESH_QUEUE_COUNT];

ta_mesh *ta_mesh_init(const char *name, ta_mesh_queue queue, GLuint *arr_index,
	ta_vec3 *arr_position, ta_vec3 *arr_normal, ta_uv *arr_uv,
	ta_color *arr_color)
{
	const int vertCompLen = 3;
	const int uvCompLen = 2;

	// Create mesh
	ta_mesh *mesh = dlb_vec_alloc(meshes[queue]);
	glCreateVertexArrays(1, &mesh->vao);
	glBindVertexArray(mesh->vao);

	if (arr_index) {
		mesh->index_count = dlb_vec_len(arr_index);
		glCreateBuffers(1, &mesh->ui_arena[TA_MESH_BUFFER_INDEX]);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->ui_arena[TA_MESH_BUFFER_INDEX]);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh->index_count * sizeof(GLuint), arr_index,
			GL_STATIC_DRAW);
	}
	if (arr_position) {
		mesh->vertex_count = dlb_vec_len(arr_position);
		glCreateBuffers(1, &mesh->ui_arena[TA_MESH_BUFFER_POSITION]);
		glBindBuffer(GL_ARRAY_BUFFER, mesh->ui_arena[TA_MESH_BUFFER_POSITION]);
		glBufferData(GL_ARRAY_BUFFER, mesh->vertex_count * vertCompLen * sizeof(GLfloat), arr_position,
			GL_STATIC_DRAW);
		glEnableVertexAttribArray(TA_SHADER_ATTR_POSITION);
		glVertexAttribPointer(TA_SHADER_ATTR_POSITION, vertCompLen, GL_FLOAT,
			false, 0, 0);
	}
	if (arr_color) {
		int color_count = dlb_vec_len(arr_color) * 4;
		glCreateBuffers(1, &mesh->ui_arena[TA_MESH_BUFFER_COLOR]);
		glBindBuffer(GL_ARRAY_BUFFER, mesh->ui_arena[TA_MESH_BUFFER_COLOR]);
		glBufferData(GL_ARRAY_BUFFER, color_count * sizeof(GLfloat), arr_color, GL_STATIC_DRAW);
		glEnableVertexAttribArray(TA_SHADER_ATTR_COLOR);
		glVertexAttribPointer(TA_SHADER_ATTR_COLOR, 3, GL_FLOAT, false, 0, 0);
	}
	if (arr_uv) {
		int uv_count = dlb_vec_len(arr_uv) * uvCompLen;
		glCreateBuffers(1, &mesh->ui_arena[TA_MESH_BUFFER_UV]);
		glBindBuffer(GL_ARRAY_BUFFER, mesh->ui_arena[TA_MESH_BUFFER_UV]);
		glBufferData(GL_ARRAY_BUFFER, uv_count * sizeof(GLfloat), arr_uv, GL_STATIC_DRAW);
		glEnableVertexAttribArray(TA_SHADER_ATTR_UV);
		glVertexAttribPointer(TA_SHADER_ATTR_UV, uvCompLen, GL_FLOAT, false, 0,
			0);
	}
	if (arr_normal) {
		int normal_count = dlb_vec_len(arr_normal) * 3;
		glCreateBuffers(1, &mesh->ui_arena[TA_MESH_BUFFER_NORMAL]);
		glBindBuffer(GL_ARRAY_BUFFER, mesh->ui_arena[TA_MESH_BUFFER_NORMAL]);
		glBufferData(GL_ARRAY_BUFFER, normal_count * sizeof(GLfloat), arr_normal, GL_STATIC_DRAW);
		glEnableVertexAttribArray(TA_SHADER_ATTR_NORMAL);
		glVertexAttribPointer(TA_SHADER_ATTR_NORMAL, 3, GL_FLOAT, false, 0, 0);
	}

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	if (arr_index) {
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}

	mesh->name.length = (u32)strlen(name);
	mesh->name.data = dlb_malloc(mesh->name.length);
	memcpy(mesh->name.data, name, mesh->name.length);

	if (!tg_mesh_table.size) {
		dlb_hash_init(&tg_mesh_table, "tg_mesh_table", 128);
	}
	dlb_hash_insert(&tg_mesh_table, mesh->name.data, mesh->name.length, mesh);
	return mesh;
}

void ta_mesh_load_obj_file(ta_mesh_queue queue, const char *filename)
{
	GLuint *arr_index = 0;
	ta_vec3 *arr_position = 0;
	ta_vec3 *arr_normal = 0;
	ta_uv *arr_uv = 0;
	ta_color *arr_color = 0;

	// Load OBJ file
	// =========================================================================

	ta_buffer *buf = ta_file_read_all(filename);
	if (!buf) {
		DLB_ASSERT(!"ta_mesh_init: Failed to read obj file");
	}

	tinyobj_attrib_t attrib = { 0 };
	tinyobj_shape_t *shapes = NULL;
	size_t num_shapes = 0;
	tinyobj_material_t *materials = NULL;
	size_t num_materials = 0;

	unsigned int flags = TINYOBJ_FLAG_TRIANGULATE;
	int ret = tinyobj_parse_obj(&attrib, &shapes, &num_shapes, &materials,
		&num_materials, buf->data, buf->length, flags);
	if (ret != TINYOBJ_SUCCESS) {
		DLB_ASSERT(!"ta_mesh_init: Failed to parse obj file");
	}

	// =========================================================================

	// Each object in file
	for (size_t i = 0; i < num_shapes; i++) {
		tinyobj_shape_t shape = shapes[i];
		// Each face in object
		size_t offset = 0;
		for (size_t fn = shape.face_offset; fn < shape.length; fn++) {
			int face_verts = attrib.face_num_verts[fn];
			DLB_ASSERT(face_verts == 3);
			for (int f = 0; f < face_verts; f++) {
				tinyobj_vertex_index_t face = attrib.faces[shape.face_offset + offset];
				ta_vec3 *pos = dlb_vec_alloc(arr_position);
				pos->x = attrib.vertices[face.v_idx * 3];
				pos->y = attrib.vertices[face.v_idx * 3 + 1];
				pos->z = attrib.vertices[face.v_idx * 3 + 2];
				ta_vec3 *norm = dlb_vec_alloc(arr_normal);
				norm->x = attrib.normals[face.vn_idx];
				norm->y = attrib.normals[face.vn_idx * 3 + 1];
				norm->z = attrib.normals[face.vn_idx * 3 + 2];
				ta_uv *uv = dlb_vec_alloc(arr_uv);
				uv->u = attrib.texcoords[face.vt_idx * 2];
				uv->v = attrib.texcoords[face.vt_idx * 2 + 1];
				// TODO: Handle attrib.material_ids possibly for color data?
				offset++;
			}
		}

		u32 pos_len = dlb_vec_len(arr_position);
		u32 normal_len = dlb_vec_len(arr_normal);
		u32 uv_len = dlb_vec_len(arr_uv);
		UNUSED(pos_len);
		UNUSED(normal_len);
		UNUSED(uv_len);

		ta_mesh_init(shapes[i].name, queue, arr_index, arr_position, arr_normal,
			arr_uv, arr_color);
	}

	tinyobj_attrib_free(&attrib);
	tinyobj_shapes_free(shapes, num_shapes);
	tinyobj_materials_free(materials, num_materials);
	ta_buffer_free(buf);
}

void ta_mesh_clear(ta_mesh_queue queue)
{
	glDeleteTextures(dlb_vec_len(gl_ids[queue]), gl_ids[queue]);
	ta_mesh *mesh = meshes[queue];
	while (mesh != dlb_vec_end(meshes[queue])) {
		dlb_hash_delete(&tg_mesh_table, mesh->name.data, mesh->name.length);
		ta_buffer_free(&mesh->name);
		mesh++;
	}
	dlb_vec_clear(meshes[queue]);
	dlb_vec_clear(gl_ids[queue]);
}