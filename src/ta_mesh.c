#include "ta_mesh.h"
#include "ta_mesh_group.h"
#include "ta_log.h"
#include "ta_shader.h"
#include "ta_symbol.h"
#include "ta_game.h"
#include "ta_primitive.h"
#include "dlb/dlb_vector.h"

void ta_mesh_create(ta_mesh *mesh)
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
    if (mesh->tangents) {
        int tangent_count = dlb_vec_len(mesh->tangents) * 3;
        glCreateBuffers(1, &mesh->buffers[TA_MESH_BUFFER_TANGENT]);
        glBindBuffer(GL_ARRAY_BUFFER, mesh->buffers[TA_MESH_BUFFER_TANGENT]);
        glBufferData(GL_ARRAY_BUFFER, tangent_count * sizeof(GLfloat), mesh->tangents,
            GL_STATIC_DRAW);
        glEnableVertexAttribArray(TA_SHADER_ATTR_TANGENT);
        glVertexAttribPointer(TA_SHADER_ATTR_TANGENT, 3, GL_FLOAT, false, 0, 0);
    }

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    if (mesh->indexes) {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }
}

// NOTE: Leave this separate from mesh_group load because it's only useful in
// debug mode
void ta_mesh_init_normals(ta_mesh *mesh, float scale)
{
    DLB_ASSERT(!mesh->vertex_normals);

    u32 normal_count = dlb_vec_len(mesh->normals);
    DLB_ASSERT(normal_count > 0);
    DLB_ASSERT(normal_count == dlb_vec_len(mesh->positions));
    dlb_vec_reserve(mesh->vertex_normals, normal_count);

    u32 face_count = normal_count / 3;
    dlb_vec_reserve(mesh->face_normals, face_count);
    dlb_vec_reserve(mesh->tangent_lines, face_count);

    ta_line_3d *line;
    u32 i = 0;
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
        ta_vec3 t0 = mesh->tangents[i * 3];
        ta_vec3 tangent = vec3_scalef(t0, scale);
        line->p0 = face_center;
        line->p1 = vec3_add(face_center, tangent);
    }
    for (; i < normal_count; i++) {
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

    //dlb_vec_each(ta_line_3d *, line, mesh->vertex_normals) {
    //    ta_primitive_push_line_3d(*line, TA_COLOR_MAGENTA, TA_COLOR_MAGENTA);
    //}
    //dlb_vec_each(ta_line_3d *, line, mesh->face_normals) {
    //    ta_primitive_push_line_3d(*line, TA_COLOR_CYAN, TA_COLOR_CYAN);
    //}
    dlb_vec_each(ta_line_3d *, line, mesh->tangent_lines) {
        ta_primitive_push_line_3d(*line, TA_COLOR_RED, TA_COLOR_GREEN);
    }
}

#if 0
void ta_mesh_log_normals_dbg(ta_mesh *mesh)
{
    ta_log_write(&tg_debug_log, "Normals:\n");
    u32 normal_count = dlb_vec_len(mesh->normals);
    DLB_ASSERT(normal_count == dlb_vec_len(mesh->positions));
    for (u32 i = 0; i < normal_count; i++) {
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
    glBindVertexArray(mesh->vao);
    int index_count = dlb_vec_len(mesh->indexes);
    if (index_count) {
        glDrawElements(GL_TRIANGLES, index_count, GL_UNSIGNED_INT, 0);
    } else {
        int vertex_count = dlb_vec_len(mesh->positions);
        glDrawArrays(GL_TRIANGLES, 0, vertex_count);
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
    glDeleteVertexArrays(1, &mesh->vao);
    // TODO: This is probably going to break, need to either use gl_ids like
    //       ta_texture_clear() or individually check if each buffer exists.
    glDeleteBuffers(TA_MESH_BUFFER_COUNT, mesh->buffers);
}