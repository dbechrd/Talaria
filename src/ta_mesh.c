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

static void ta_mesh_init_gl(ta_mesh *mesh)
{
	const int vertCompLen = 3;
	const int uvCompLen = 2;

	glCreateVertexArrays(1, &mesh->vao);
	glBindVertexArray(mesh->vao);

	if (mesh->indexes) {
		int index_count = dlb_vec_len(mesh->indexes);
		glCreateBuffers(1, &mesh->buffers[TA_MESH_BUFFER_INDEX]);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->buffers[TA_MESH_BUFFER_INDEX]);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_count * sizeof(GLuint),
            mesh->indexes, GL_STATIC_DRAW);
	}
	if (mesh->positions) {
		int vertex_count = dlb_vec_len(mesh->positions);
		glCreateBuffers(1, &mesh->buffers[TA_MESH_BUFFER_POSITION]);
		glBindBuffer(GL_ARRAY_BUFFER, mesh->buffers[TA_MESH_BUFFER_POSITION]);
		glBufferData(GL_ARRAY_BUFFER, vertex_count * vertCompLen * sizeof(GLfloat),
            mesh->positions, GL_STATIC_DRAW);
		glEnableVertexAttribArray(TA_SHADER_ATTR_POSITION);
		glVertexAttribPointer(TA_SHADER_ATTR_POSITION, vertCompLen, GL_FLOAT,
			false, 0, 0);
	}
	if (mesh->colors) {
		int color_count = dlb_vec_len(mesh->colors) * 4;
		glCreateBuffers(1, &mesh->buffers[TA_MESH_BUFFER_COLOR]);
		glBindBuffer(GL_ARRAY_BUFFER, mesh->buffers[TA_MESH_BUFFER_COLOR]);
		glBufferData(GL_ARRAY_BUFFER, color_count * sizeof(GLfloat), mesh->colors,
            GL_STATIC_DRAW);
		glEnableVertexAttribArray(TA_SHADER_ATTR_COLOR);
		glVertexAttribPointer(TA_SHADER_ATTR_COLOR, 3, GL_FLOAT, false, 0, 0);
	}
	if (mesh->uvs) {
		int uv_count = dlb_vec_len(mesh->uvs) * uvCompLen;
		glCreateBuffers(1, &mesh->buffers[TA_MESH_BUFFER_UV]);
		glBindBuffer(GL_ARRAY_BUFFER, mesh->buffers[TA_MESH_BUFFER_UV]);
		glBufferData(GL_ARRAY_BUFFER, uv_count * sizeof(GLfloat), mesh->uvs,
            GL_STATIC_DRAW);
		glEnableVertexAttribArray(TA_SHADER_ATTR_UV);
		glVertexAttribPointer(TA_SHADER_ATTR_UV, uvCompLen, GL_FLOAT, false, 0,
			0);
	}
	if (mesh->normals) {
		int normal_count = dlb_vec_len(mesh->normals) * 3;
		glCreateBuffers(1, &mesh->buffers[TA_MESH_BUFFER_NORMAL]);
		glBindBuffer(GL_ARRAY_BUFFER, mesh->buffers[TA_MESH_BUFFER_NORMAL]);
		glBufferData(GL_ARRAY_BUFFER, normal_count * sizeof(GLfloat), mesh->normals,
            GL_STATIC_DRAW);
		glEnableVertexAttribArray(TA_SHADER_ATTR_NORMAL);
		glVertexAttribPointer(TA_SHADER_ATTR_NORMAL, 3, GL_FLOAT, false, 0, 0);
	}

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	if (mesh->indexes) {
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}
}

void ta_mesh_load_obj_file(ta_mesh_queue queue, const char *filename)
{
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
		size_t offset = 0;

        ta_mesh *mesh = dlb_vec_alloc(meshes[queue]);

		// Each face in object
		for (size_t fn = shape.face_offset; fn < shape.length; fn++) {
			int face_verts = attrib.face_num_verts[fn];
			DLB_ASSERT(face_verts == 3);
			for (int f = 0; f < face_verts; f++) {
				tinyobj_vertex_index_t face = attrib.faces[shape.face_offset + offset];
				ta_vec3 *pos = dlb_vec_alloc(mesh->positions);
				pos->x = attrib.vertices[face.v_idx * 3];
				pos->y = attrib.vertices[face.v_idx * 3 + 1];
				pos->z = attrib.vertices[face.v_idx * 3 + 2];
				ta_vec3 *norm = dlb_vec_alloc(mesh->normals);
				norm->x = attrib.normals[face.vn_idx * 3];
				norm->y = attrib.normals[face.vn_idx * 3 + 1];
				norm->z = attrib.normals[face.vn_idx * 3 + 2];
				ta_uv *uv = dlb_vec_alloc(mesh->uvs);
				uv->u = attrib.texcoords[face.vt_idx * 2];
				uv->v = attrib.texcoords[face.vt_idx * 2 + 1];
				// TODO: Handle attrib.material_ids possibly for color data?
				offset++;
			}
		}

		ta_mesh_init_gl(mesh);

        mesh->name.length = (u32)strlen(shapes[i].name);
        mesh->name.data = dlb_malloc(mesh->name.length);
        memcpy(mesh->name.data, shapes[i].name, mesh->name.length);

        if (!tg_mesh_table.size) {
            dlb_hash_init(&tg_mesh_table, DLB_HASH_STRING, "tg_mesh_table", 128);
            if (!tg_mesh_table.size) {
                DLB_ASSERT(!"Failed to initialize mesh hash table");
            }
        }
        dlb_hash_insert(&tg_mesh_table, mesh->name.data, mesh->name.length, mesh);
	}

	tinyobj_attrib_free(&attrib);
	tinyobj_shapes_free(shapes, num_shapes);
	tinyobj_materials_free(materials, num_materials);
	ta_buffer_free(buf);
}

void ta_mesh_push_normals(ta_mesh *mesh)
{
    u32 normal_count = dlb_vec_len(mesh->normals);
    DLB_ASSERT(normal_count == dlb_vec_len(mesh->positions));
    for (u32 i = 0; i < normal_count; i++) {
        ta_line_3d line = { 0 };
        line.p0 = mesh->positions[i];
        line.p1 = vec3_add(mesh->positions[i], mesh->normals[i]);
        //ta_primitive_push_line_3d(&line, &TA_COLOR_RED, &TA_COLOR_GREEN);
        ta_primitive_push_line_3d(&line, &TA_COLOR_BLUE, &TA_COLOR_BLUE);
    }
}

void debug_mesh_log_normals(ta_mesh *mesh)
{
    ta_log_write(tg_debug_log, "Normals:\n");
    u32 normal_count = dlb_vec_len(mesh->normals);
    DLB_ASSERT(normal_count == dlb_vec_len(mesh->positions));
    for (u32 i = 0; i < normal_count; i++) {
        ta_line_3d line = { 0 };
        line.p0 = mesh->positions[i];
        line.p1 = vec3_add(mesh->positions[i], mesh->normals[i]);
        ta_log_write(tg_debug_log, "[%d] { %f, %f, %f } -> { %f, %f, %f }\n", i,
            line.p0.x, line.p0.y, line.p0.z, line.p1.x, line.p1.y, line.p1.z);
    }
}

void ta_mesh_free(ta_mesh *mesh)
{
    dlb_hash_delete(&tg_mesh_table, mesh->name.data, mesh->name.length);
    ta_buffer_free(&mesh->name);
    dlb_vec_free(mesh->indexes);
    dlb_vec_free(mesh->positions);
    dlb_vec_free(mesh->normals);
    dlb_vec_free(mesh->uvs);
    dlb_vec_free(mesh->colors);
    glDeleteVertexArrays(1, &mesh->vao);
    // TODO: This is probably going to break, need to either use gl_ids like
    //       ta_texture_clear() or individually check if each buffer exists.
    glDeleteBuffers(TA_MESH_BUFFER_COUNT, mesh->buffers);
}

void ta_mesh_clear(ta_mesh_queue queue)
{
	ta_mesh *mesh = meshes[queue];
	while (mesh != dlb_vec_end(meshes[queue])) {
		ta_mesh_free(mesh);
		mesh++;
	}
	dlb_vec_clear(meshes[queue]);
}