#include "ta_mesh.h"
#include "ta_file.h"
#include "ta_game.h"
#include "ta_log.h"
#include "ta_primitive.h"
#include "ta_schema.h"
#include "ta_shader.h"
#include "ta_symbol.h"
#include "dlb/dlb_vector.h"

#define TINYOBJ_MALLOC dlb_malloc
#define TINYOBJ_CALLOC dlb_calloc
#define TINYOBJ_REALLOC dlb_realloc
#define TINYOBJ_FREE dlb_free
#define TINYOBJ_LOADER_C_IMPLEMENTATION
#include "misc/tinyobj_loader_c.h"

ta_mesh *tg_mesh_default;

void ta_mesh_init(ta_mesh *mesh)
{
    if (mesh->path) {
        ta_mesh_load_file(mesh, mesh->path);
    }
}

void ta_mesh_load_file(ta_mesh *mesh, const char *filename)
{
    mesh->path = filename;

    // Load OBJ file
    // =========================================================================

    char *buf = ta_file_read_all(mesh->path);
    if (!buf) {
        DLB_ASSERT(!"ta_mesh_init: Failed to read obj file");
    }

    tinyobj_attrib_t attrib = { 0 };
    tinyobj_shape_t *shapes = NULL;
    size_t num_shapes = 0;
    tinyobj_material_t *materials = NULL;
    size_t num_materials = 0;

    unsigned int flags = TINYOBJ_FLAG_TRIANGULATE;
    int ret = tinyobj_parse_obj(&attrib, &shapes, &num_shapes, &materials, &num_materials, buf, dlb_vec_len(buf), flags);
    if (ret != TINYOBJ_SUCCESS) {
        DLB_ASSERT(!"ta_mesh_init: Failed to parse obj file");
    }

    // =========================================================================

    ta_vec3 group_min = VEC3_MAX;
    ta_vec3 group_max = VEC3_MIN;
    float group_radius = 0.0f;
    ta_vec3 mesh_min;
    ta_vec3 mesh_max;
    float mesh_radius;

    // We could make multiple meshes, but it doesn't really make sense unless
    // we have some sort of mesh_primitive, right?
    DLB_ASSERT(num_shapes == 1);

    // Each object in file
    for (size_t shape_idx = 0; shape_idx < num_shapes; shape_idx++) {
        tinyobj_shape_t shape = shapes[shape_idx];

        //u32 name_len = (u32)strlen(shapes[shape_idx].name);
        //const char *name = ta_symbol_intern(shapes[shape_idx].name, name_len);
        //ta_mesh *mesh = ta_game_alloc(RES_MESH, name, name_len);

        mesh_min = VEC3_MAX;
        mesh_max = VEC3_MIN;
        mesh_radius = 0.0f;

        dlb_vec_reserve(mesh->positions, shape.length * 3);
        dlb_vec_reserve(mesh->normals, shape.length * 3);
        dlb_vec_reserve(mesh->uvs, shape.length * 3);
        dlb_vec_reserve(mesh->tangents, shape.length * 3);

        // Each face in object
        for (size_t f_idx = 0; f_idx < shape.length; f_idx++) {
            const int face_verts = attrib.face_num_verts[shape.face_offset + f_idx];
            DLB_ASSERT(face_verts == 3);
            ta_vec3 positions[3] = { 0 };
            ta_vec2 uvs[3] = { 0 };

            for (int v_idx = 0; v_idx < 3; v_idx++) {
                tinyobj_vertex_index_t vert = attrib.faces[shape.face_offset * 3 + f_idx * 3 + v_idx];
                ta_vec3 *position = dlb_vec_alloc(mesh->positions);
                position->x = attrib.vertices[vert.v_idx * 3];
                position->y = attrib.vertices[vert.v_idx * 3 + 1];
                position->z = attrib.vertices[vert.v_idx * 3 + 2];
                ta_vec3 *normal = dlb_vec_alloc(mesh->normals);
                normal->x = attrib.normals[vert.vn_idx * 3];
                normal->y = attrib.normals[vert.vn_idx * 3 + 1];
                normal->z = attrib.normals[vert.vn_idx * 3 + 2];
                ta_vec2 *uv = dlb_vec_alloc(mesh->uvs);
                uv->x = attrib.texcoords[vert.vt_idx * 2];
                uv->y = attrib.texcoords[vert.vt_idx * 2 + 1];
                // TODO: Handle attrib.material_ids possibly for color data?

                positions[v_idx] = *position;
                uvs[v_idx] = *uv;

                mesh_min.x = MIN(mesh_min.x, position->x);
                mesh_min.y = MIN(mesh_min.y, position->y);
                mesh_min.z = MIN(mesh_min.z, position->z);
                mesh_max.x = MAX(mesh_max.x, position->x);
                mesh_max.y = MAX(mesh_max.y, position->y);
                mesh_max.z = MAX(mesh_max.z, position->z);
                mesh_radius = MAX(mesh_radius, vec3_len(*position));
            }

            ta_vec3 edge1 = vec3_sub(positions[1], positions[0]);
            ta_vec3 edge2 = vec3_sub(positions[2], positions[0]);
            ta_vec2 delta_uv1 = vec2_sub(uvs[1], uvs[0]);
            ta_vec2 delta_uv2 = vec2_sub(uvs[2], uvs[0]);

            // TODO: Use MikkTSpace (http://www.mikktspace.com/) (or, just pre-calculate in Blender and store on disk)
            ta_vec3 tangent = { 0 };
            float f = 1.0f / (delta_uv1.x * delta_uv2.y - delta_uv2.x * delta_uv1.y);
            tangent.x = f * (delta_uv2.y * edge1.x - delta_uv1.y * edge2.x);
            tangent.y = f * (delta_uv2.y * edge1.y - delta_uv1.y * edge2.y);
            tangent.z = f * (delta_uv2.y * edge1.z - delta_uv1.y * edge2.z);
            tangent = vec3_normalize(tangent);
            ta_vec4 tangent4 = { 0 };
            tangent4.x = tangent.x;
            tangent4.y = tangent.y;
            tangent4.z = tangent.z;
            tangent4.w = 1.0f;
            dlb_vec_push(mesh->tangents, tangent4);
            dlb_vec_push(mesh->tangents, tangent4);
            dlb_vec_push(mesh->tangents, tangent4);
        }

        ta_mesh_create(mesh);

        mesh->aabb.extents = vec3_scalef(vec3_sub(mesh_max, mesh_min), 0.5f);
        mesh->aabb.center = vec3_add(mesh_min, mesh->aabb.extents);
        //mesh->sphere.center = mesh->aabb.center;
        //mesh->sphere.radius = MIN(mesh_radius, vec3_len(mesh->aabb.extents));

        group_min.x = MIN(group_min.x, mesh_min.x);
        group_min.y = MIN(group_min.y, mesh_min.y);
        group_min.z = MIN(group_min.z, mesh_min.z);
        group_max.x = MAX(group_max.x, mesh_max.x);
        group_max.y = MAX(group_max.y, mesh_max.y);
        group_max.z = MAX(group_max.z, mesh_max.z);
        group_radius = MAX(group_radius, mesh_radius);

        ta_mesh_init_normals(mesh, 0.1f);
    }

    tinyobj_attrib_free(&attrib);
    tinyobj_shapes_free(shapes, num_shapes);
    tinyobj_materials_free(materials, num_materials);
    dlb_vec_free(buf);
}

void ta_mesh_create(ta_mesh *mesh)
{
    glGenVertexArrays(1, &mesh->gl_vao);
    glBindVertexArray(mesh->gl_vao);

    if (mesh->positions) {
        size_t positions_count = dlb_vec_len(mesh->positions);
        glGenBuffers(1, &mesh->gl_buffers[TA_MESH_BUFFER_POSITION]);
        glBindBuffer(GL_ARRAY_BUFFER, mesh->gl_buffers[TA_MESH_BUFFER_POSITION]);
        glBufferData(GL_ARRAY_BUFFER, positions_count * 3 * sizeof(GLfloat), mesh->positions, GL_STATIC_DRAW);
        glEnableVertexAttribArray(TA_SHADER_ATTR_POSITION);
        glVertexAttribPointer(TA_SHADER_ATTR_POSITION, 3, GL_FLOAT, false, 0, 0);
    }
    if (mesh->colors) {
        size_t colors_count = dlb_vec_len(mesh->colors);
        glGenBuffers(1, &mesh->gl_buffers[TA_MESH_BUFFER_COLOR]);
        glBindBuffer(GL_ARRAY_BUFFER, mesh->gl_buffers[TA_MESH_BUFFER_COLOR]);
        glBufferData(GL_ARRAY_BUFFER, colors_count * 4 * sizeof(GLfloat), mesh->colors, GL_STATIC_DRAW);
        glEnableVertexAttribArray(TA_SHADER_ATTR_COLOR);
        glVertexAttribPointer(TA_SHADER_ATTR_COLOR, 4, GL_FLOAT, false, 0, 0);
    }
    if (mesh->uvs) {
        size_t uvs_count = dlb_vec_len(mesh->uvs);
        glGenBuffers(1, &mesh->gl_buffers[TA_MESH_BUFFER_UV]);
        glBindBuffer(GL_ARRAY_BUFFER, mesh->gl_buffers[TA_MESH_BUFFER_UV]);
        glBufferData(GL_ARRAY_BUFFER, uvs_count * 2 * sizeof(GLfloat), mesh->uvs, GL_STATIC_DRAW);
        glEnableVertexAttribArray(TA_SHADER_ATTR_UV);
        glVertexAttribPointer(TA_SHADER_ATTR_UV, 2, GL_FLOAT, false, 0, 0);
    }
    if (mesh->normals) {
        size_t normals_count = dlb_vec_len(mesh->normals);
        glGenBuffers(1, &mesh->gl_buffers[TA_MESH_BUFFER_NORMAL]);
        glBindBuffer(GL_ARRAY_BUFFER, mesh->gl_buffers[TA_MESH_BUFFER_NORMAL]);
        glBufferData(GL_ARRAY_BUFFER, normals_count * 3 * sizeof(GLfloat), mesh->normals, GL_STATIC_DRAW);
        glEnableVertexAttribArray(TA_SHADER_ATTR_NORMAL);
        glVertexAttribPointer(TA_SHADER_ATTR_NORMAL, 3, GL_FLOAT, false, 0, 0);
    }
    if (mesh->tangents) {
        size_t tangents_count = dlb_vec_len(mesh->tangents);
        glGenBuffers(1, &mesh->gl_buffers[TA_MESH_BUFFER_TANGENT]);
        glBindBuffer(GL_ARRAY_BUFFER, mesh->gl_buffers[TA_MESH_BUFFER_TANGENT]);
        glBufferData(GL_ARRAY_BUFFER, tangents_count * 4 * sizeof(GLfloat), mesh->tangents, GL_STATIC_DRAW);
        glEnableVertexAttribArray(TA_SHADER_ATTR_TANGENT);
        glVertexAttribPointer(TA_SHADER_ATTR_TANGENT, 4, GL_FLOAT, false, 0, 0);
    }
    if (mesh->joints) {
        DLB_ASSERT(!"This type of mesh init doesn't currently support joints");
    }
    if (mesh->weights) {
        DLB_ASSERT(!"This type of mesh init doesn't currently support weights");
    }
    if (mesh->indexes) {
        size_t indexes_count = dlb_vec_len(mesh->indexes);
        glGenBuffers(1, &mesh->gl_buffers[TA_MESH_BUFFER_INDEX]);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->gl_buffers[TA_MESH_BUFFER_INDEX]);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexes_count * sizeof(GLuint), mesh->indexes, GL_STATIC_DRAW);
    }

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

// NOTE: Leave this separate from mesh_group load because it's only useful in
// debug mode
void ta_mesh_init_normals(ta_mesh *mesh, float scale)
{
    DLB_ASSERT(!mesh->vertex_normals);

    size_t normals_count = dlb_vec_len(mesh->normals);
    DLB_ASSERT(normals_count > 0);
    DLB_ASSERT(normals_count == dlb_vec_len(mesh->positions));
    dlb_vec_reserve(mesh->vertex_normals, normals_count);

    size_t face_count = normals_count / 3;
    dlb_vec_reserve(mesh->face_normals, face_count);
    dlb_vec_reserve(mesh->tangent_lines, face_count);

    ta_line_3d *line;
    size_t i = 0;
    for (; i < face_count; i++) {
        line = dlb_vec_alloc(mesh->vertex_normals);
        ta_vec3 vertex_normal = vec3_scalef(mesh->normals[i], scale);
        line->p0 = mesh->positions[i];
        line->p1 = vec3_add(mesh->positions[i], vertex_normal);

        line = dlb_vec_alloc(mesh->face_normals);
        ta_vec3 v0 = mesh->positions[i * 3];
        ta_vec3 v1 = mesh->positions[i * 3 + 1];
        ta_vec3 v2 = mesh->positions[i * 3 + 2];
        ta_vec3 edge0 = vec3_sub(v1, v0);
        ta_vec3 edge1 = vec3_sub(v2, v1);
        ta_vec3 face_normal = vec3_scalef(vec3_normalize(vec3_cross(edge0, edge1)), scale);
        ta_vec3 face_center = vec3_scalef(vec3_add(vec3_add(v0, v1), v2), 1.0f / 3.0f);
        line->p0 = face_center;
        line->p1 = vec3_add(face_center, face_normal);

        line = dlb_vec_alloc(mesh->tangent_lines);
        ta_vec4 t_orig = mesh->tangents[i * 3];
        ta_vec3 tangent = { 0 };
        tangent.x = t_orig.x * scale;
        tangent.y = t_orig.y * scale;
        tangent.z = t_orig.z * scale;
        line->p0 = face_center;
        line->p1 = vec3_add(face_center, tangent);
    }
    for (; i < normals_count; i++) {
        line = dlb_vec_alloc(mesh->vertex_normals);
        ta_vec3 vertex_normal = vec3_scalef(mesh->normals[i], scale);
        line->p0 = mesh->positions[i];
        line->p1 = vec3_add(mesh->positions[i], vertex_normal);
    }
}

#if 0
ta_aabb ta_mesh_aabb(ta_mesh *mesh)
{
    ta_vec3 min = VEC3_MAX;
    ta_vec3 max = VEC3_MIN;

    dlb_vec_each(ta_vec3 *, pos, mesh->positions) {
        min.x = MIN(min.x, pos->x);
        min.y = MIN(min.y, pos->y);
        min.z = MIN(min.z, pos->z);
        max.x = MAX(max.x, pos->x);
        max.y = MAX(max.y, pos->y);
        max.z = MAX(max.z, pos->z);
    }

    ta_aabb aabb = { 0 };
    aabb.extents = vec3_scalef(vec3_sub(max, min), 0.5f);
    aabb.center = vec3_add(min, aabb.extents);
    return aabb;
}
#endif

void ta_mesh_push_normals(ta_mesh *mesh)
{
    DLB_ASSERT(mesh->vertex_normals);
    DLB_ASSERT(mesh->face_normals);
    DLB_ASSERT(mesh->tangent_lines);

    dlb_vec_each(ta_line_3d *, line, mesh->vertex_normals) {
        ta_primitive_push_line_3d(0, *line, TA_COLOR_MAGENTA, TA_COLOR_MAGENTA);
    }
    dlb_vec_each(ta_line_3d *, line, mesh->face_normals) {
        ta_primitive_push_line_3d(0, *line, TA_COLOR_CYAN, TA_COLOR_CYAN);
    }
    dlb_vec_each(ta_line_3d *, line, mesh->tangent_lines) {
        ta_primitive_push_line_3d(0, *line, TA_COLOR_RED, TA_COLOR_GREEN);
    }
}

#if 0
void ta_mesh_log_normals_dbg(ta_mesh *mesh)
{
    ta_log_write(&tg_debug_log, "Normals:\n");
    DLB_ASSERT(mesh->normals_count == mesh->positions_count);
    for (u32 i = 0; i < mesh->normals_count; i++) {
        ta_line_3d line = { 0 };
        line.p0 = mesh->positions[i];
        line.p1 = vec3_add(mesh->positions[i], mesh->normals[i]);
        ta_log_write(&tg_debug_log, "[%d] { %f, %f, %f } -> { %f, %f, %f }\n", i,
            line.p0.x, line.p0.y, line.p0.z, line.p1.x, line.p1.y, line.p1.z);
    }
}
#endif

void ta_mesh_render(ta_mesh *mesh)
{
    if (!mesh->gl_vao) {
        mesh = tg_mesh_default;
        DLB_ASSERT(mesh);  // No mesh & no default mesh, this seems undesirable!
    }
    glBindVertexArray(mesh->gl_vao);
    size_t indexes_count = dlb_vec_len(mesh->indexes);
    if (indexes_count) {
        glDrawElements(GL_TRIANGLES, (GLsizei)indexes_count, GL_UNSIGNED_INT, 0);
    } else {
        size_t positions_count = dlb_vec_len(mesh->positions);
        glDrawArrays(GL_TRIANGLES, 0, (GLsizei)positions_count);
    }
    glBindVertexArray(0);
}

void ta_mesh_free(ta_mesh *mesh)
{
    //dlb_hash_delete(&mesh->group->meshes_by_name, SYM(mesh->name));
    dlb_vec_free(mesh->indexes);
    dlb_vec_free(mesh->positions);
    dlb_vec_free(mesh->uvs);
    dlb_vec_free(mesh->colors);
    dlb_vec_free(mesh->normals);
    dlb_vec_free(mesh->tangents);
    glDeleteVertexArrays(1, &mesh->gl_vao);
    // TODO: This is probably going to break, need to either use gl_ids like
    //       ta_texture_clear() or individually check if each buffer exists.
    glDeleteBuffers(TA_MESH_BUFFER_COUNT, mesh->gl_buffers);
}