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

const char *tg_mesh_default;
GLuint tg_mesh_gl_default_bone_xforms;

const char *ta_vertex_attrib_type_str(int type) {
    switch (type) {
        case TA_VERTEX_ATTR_COLOR:           return "TA_VERTEX_ATTR_COLOR          ";
        case TA_VERTEX_ATTR_UV:              return "TA_VERTEX_ATTR_UV             ";
        case TA_VERTEX_ATTR_POSITION:        return "TA_VERTEX_ATTR_POSITION       ";
        case TA_VERTEX_ATTR_NORMAL:          return "TA_VERTEX_ATTR_NORMAL         ";
        case TA_VERTEX_ATTR_TANGENT:         return "TA_VERTEX_ATTR_TANGENT        ";
        case TA_VERTEX_ATTR_MORPH1_POSITION: return "TA_VERTEX_ATTR_MORPH1_POSITION";
        case TA_VERTEX_ATTR_MORPH1_NORMAL:   return "TA_VERTEX_ATTR_MORPH1_NORMAL  ";
        case TA_VERTEX_ATTR_MORPH1_TANGENT:  return "TA_VERTEX_ATTR_MORPH1_TANGENT ";
        case TA_VERTEX_ATTR_BONE_INDICES:    return "TA_VERTEX_ATTR_BONE_INDICES   ";
        case TA_VERTEX_ATTR_BONE_WEIGHTS:    return "TA_VERTEX_ATTR_BONE_WEIGHTS   ";
        default: DLB_ASSERT(0);              return "TA_VERTEX_ATTR_???            ";
    }
}

void ta_mesh_init(ta_mesh *mesh)
{
    if (mesh->path) {
        ta_mesh_load_file(mesh, mesh->path);
    }
}
void ta_mesh_init_void(void *mesh)
{
    ta_mesh_init(mesh);
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
    // NOTE: If we want to handle more than 1, we should create multiple ta_mesh_vertex_arrays.
    DLB_ASSERT(num_shapes == 1);

    // Each object in file
    for (size_t shape_idx = 0; shape_idx < num_shapes; shape_idx++) {
        tinyobj_shape_t shape = shapes[shape_idx];

        //u32 name_len = (u32)strlen(shapes[shape_idx].name);
        //const char *name = ta_symbol_intern(shapes[shape_idx].name, name_len);
        //ta_mesh *mesh = (ta_mesh *)ta_game_alloc(RES_MESH, name, name_len);

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
            dlb_vec_push(mesh->tangents, tangent);
            dlb_vec_push(mesh->tangents, tangent);
            dlb_vec_push(mesh->tangents, tangent);
        }

        ta_mesh_create(mesh);
        ta_mesh_update_buffers(mesh);

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

        ta_mesh_update_debug_lines(mesh, 0.1f);
    }

    tinyobj_attrib_free(&attrib);
    tinyobj_shapes_free(shapes, num_shapes);
    tinyobj_materials_free(materials, num_materials);
    dlb_vec_free(buf);
}

void ta_mesh_create(ta_mesh *mesh)
{
    glGenVertexArrays(1, &mesh->gl_vao);
    glGenBuffers(1, &mesh->gl_vertex_buffer);
    if (mesh->index_arrays) {
        glGenBuffers(1, &mesh->gl_index_buffer);
    }

    // Debug lines VAO/buffer
    glGenVertexArrays(1, &mesh->debug_lines.gl_vao);
    glGenBuffers(1, &mesh->debug_lines.gl_vertex_buffer);
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

void ta_mesh_calculate_joints_and_weights(ta_mesh *mesh)
{
    size_t bone_count_len = dlb_vec_len(mesh->skin.bone_count_array);
    if (!bone_count_len) {
        return;
    }

    // If there are any bone counts, there should be the same ## as there are vertices?
    DLB_ASSERT(bone_count_len == dlb_vec_len(mesh->positions));

    dlb_vec_clear(mesh->bone_indices);
    dlb_vec_clear(mesh->bone_weights);

    dlb_vec_reserve(mesh->bone_indices, bone_count_len);
    dlb_vec_reserve(mesh->bone_weights, bone_count_len);

    size_t bone_offset = 0;
    dlb_vec_each(u16 *, bone_count, mesh->skin.bone_count_array) {
        ta_vec4 indices = { 0 };
        ta_vec4 weights = { 0 };

        for (;;) {
            if (*bone_count == 0) break;

            indices.x = mesh->skin.bone_index_array[bone_offset];
            weights.x = mesh->skin.bone_weight_array[bone_offset];
            bone_offset++;
            if (*bone_count == 1) break;

            indices.y = mesh->skin.bone_index_array[bone_offset];
            weights.y = mesh->skin.bone_weight_array[bone_offset];
            bone_offset++;
            if (*bone_count == 2) break;

            indices.z = mesh->skin.bone_index_array[bone_offset];
            weights.z = mesh->skin.bone_weight_array[bone_offset];
            bone_offset++;
            if (*bone_count == 3) break;

            indices.w = mesh->skin.bone_index_array[bone_offset];
            weights.w = mesh->skin.bone_weight_array[bone_offset];
            bone_offset++;
            break;
        }

        // TODO: Pick the 4 biggest influences, rather than the first 4 (or just don't export > 4)
        if (*bone_count > 4) {
            bone_offset += *bone_count - 4;
        }

        dlb_vec_push(mesh->bone_indices, indices);
        dlb_vec_push(mesh->bone_weights, weights);
    }

    DLB_ASSERT(bone_count_len == dlb_vec_len(mesh->bone_indices));
    DLB_ASSERT(bone_count_len == dlb_vec_len(mesh->bone_weights));
}

void ta_mesh_update_buffers(ta_mesh *mesh)
{
    DLB_ASSERT(mesh->gl_vao);
    glBindVertexArray(mesh->gl_vao);

    // Calculate size of all vertex attribute data
    size_t vertex_size_total = 0;
    for (int i = 0; i < TA_VERTEX_ATTR_COUNT; ++i) {
        vertex_size_total += dlb_vec_size(mesh->buffers[i]);
    }
    DLB_ASSERT(vertex_size_total);

    // Create/fill vertex attribute buffer
    DLB_ASSERT(mesh->gl_vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, mesh->gl_vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, vertex_size_total, 0, GL_STATIC_DRAW);

#define FILL_BUFFER(shader_attr, data, c_type, gl_type)                                       \
    if (data) {                                                                               \
        size_t vertex_size = dlb_vec_size(data);                                              \
        glEnableVertexAttribArray(shader_attr);                                               \
        glVertexAttribPointer(shader_attr, sizeof(*data) / sizeof(c_type), gl_type, false, 0, \
            (void *)vertex_offset);                                                           \
        glBufferSubData(GL_ARRAY_BUFFER, vertex_offset, vertex_size, data);                   \
        vertex_offset += vertex_size;                                                         \
    }

    size_t vertex_offset = 0;
    FILL_BUFFER(TA_VERTEX_ATTR_COLOR,           mesh->colors,           GLfloat, GL_FLOAT);
    FILL_BUFFER(TA_VERTEX_ATTR_UV,              mesh->uvs,              GLfloat, GL_FLOAT);
    FILL_BUFFER(TA_VERTEX_ATTR_POSITION,        mesh->positions,        GLfloat, GL_FLOAT);
    FILL_BUFFER(TA_VERTEX_ATTR_NORMAL,          mesh->normals,          GLfloat, GL_FLOAT);
    FILL_BUFFER(TA_VERTEX_ATTR_TANGENT,         mesh->tangents,         GLfloat, GL_FLOAT);
    FILL_BUFFER(TA_VERTEX_ATTR_MORPH1_POSITION, mesh->morph1_positions, GLfloat, GL_FLOAT);
    FILL_BUFFER(TA_VERTEX_ATTR_MORPH1_NORMAL,   mesh->morph1_normals,   GLfloat, GL_FLOAT);
    FILL_BUFFER(TA_VERTEX_ATTR_MORPH1_TANGENT,  mesh->morph1_tangents,  GLfloat, GL_FLOAT);
    FILL_BUFFER(TA_VERTEX_ATTR_BONE_INDICES,    mesh->bone_indices,     GLfloat, GL_FLOAT);
    FILL_BUFFER(TA_VERTEX_ATTR_BONE_WEIGHTS,    mesh->bone_weights,     GLfloat, GL_FLOAT);

#undef FILL_BUFFER

    DLB_ASSERT(vertex_offset == vertex_size_total);

    // Calculate size of all index data
    size_t index_size_total = 0;
    dlb_vec_each(ta_index_array *, index_array, mesh->index_arrays) {
        index_size_total += dlb_vec_size(index_array->values);
    }

    // Create/fill index buffer
    if (index_size_total) {
        DLB_ASSERT(mesh->gl_index_buffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->gl_index_buffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_size_total, 0, GL_STATIC_DRAW);

        size_t byte_offset = 0;
        dlb_vec_each(ta_index_array *, index_array, mesh->index_arrays) {
            index_array->offset_bytes = (GLint)byte_offset;
            size_t index_size = dlb_vec_size(index_array->values);
            glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, byte_offset, index_size, index_array->values);
            byte_offset += index_size;
        }

        DLB_ASSERT(byte_offset == index_size_total);
    }

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    if (dlb_vec_len(mesh->skin.bone_count_array)) {
        glGenBuffers(1, &mesh->skin.gl_ubo_bone_xforms);
        glBindBuffer(GL_UNIFORM_BUFFER, mesh->skin.gl_ubo_bone_xforms);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(mesh->skin.bone_xforms), 0, GL_DYNAMIC_DRAW);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }
}
void ta_mesh_clear_buffers(ta_mesh *mesh)
{
    for (int i = 0; i < TA_VERTEX_ATTR_COUNT; ++i) {
        dlb_vec_clear(mesh->buffers[i]);
    }
}

// NOTE: Leave this separate from mesh_group load because it's only useful in debug mode
void ta_mesh_update_debug_lines(ta_mesh *mesh, float scale)
{
    dlb_vec_clear(mesh->debug_lines.positions);
    dlb_vec_clear(mesh->debug_lines.colors);

    //=========================================================================
    // Generate debug lines on CPU
    //=========================================================================
    // NOTE: These roughly match Blender 2.8 colors
    ta_vec4 face_normal_color = vec4_init(0.13f, 0.87f, 0.87f, 1.0f);
    ta_vec4 face_tangent_color = vec4_init(0.98f, 0.87f, 0.13f, 1.0f);
    ta_vec4 vertex_normal_color = vec4_init(0.13f, 0.38f, 0.98f, 1.0f);

    size_t normals_count = dlb_vec_len(mesh->normals);
    DLB_ASSERT(normals_count > 0);  // Did you forget to export normals?
    DLB_ASSERT(normals_count == dlb_vec_len(mesh->positions));
    size_t tangents_count = dlb_vec_len(mesh->tangents);
    DLB_ASSERT(tangents_count > 0);  // Did you forget to export tangents (or triangulate faces before gltf export)?
    DLB_ASSERT(tangents_count == dlb_vec_len(mesh->positions));

    size_t face_count = normals_count / 3;

    size_t debug_lines_count = normals_count + face_count * 2;
    dlb_vec_reserve(mesh->debug_lines.positions, debug_lines_count);
    dlb_vec_reserve(mesh->debug_lines.colors, debug_lines_count);

    ta_line_3d line;
    size_t i = 0;
    for (; i < face_count; i++) {
        ta_vec3 v0 = mesh->positions[i * 3];
        ta_vec3 v1 = mesh->positions[i * 3 + 1];
        ta_vec3 v2 = mesh->positions[i * 3 + 2];
        ta_vec3 edge0 = vec3_sub(v1, v0);
        ta_vec3 edge1 = vec3_sub(v2, v1);
        ta_vec3 face_normal = vec3_scalef(vec3_normalize(vec3_cross(edge0, edge1)), scale);
        ta_vec3 face_center = vec3_scalef(vec3_add(vec3_add(v0, v1), v2), 1.0f / 3.0f);
        line.p0 = face_center;
        line.p1 = vec3_add(face_center, face_normal);
        dlb_vec_push(mesh->debug_lines.positions, line.p0);
        dlb_vec_push(mesh->debug_lines.positions, line.p1);
        dlb_vec_push(mesh->debug_lines.colors, *(ta_rgba *)&face_normal_color);
        dlb_vec_push(mesh->debug_lines.colors, *(ta_rgba *)&face_normal_color);

#if 0
        ta_vec3 tangent = mesh->tangents[i * 3];
        tangent = vec3_scalef(tangent, scale);
        line.p0 = face_center;
        line.p1 = vec3_add(face_center, tangent);
        dlb_vec_push(mesh->debug_lines.positions, line.p0);
        dlb_vec_push(mesh->debug_lines.positions, line.p1);
        dlb_vec_push(mesh->debug_lines.colors, *(ta_rgba *)&face_tangent_color);
        dlb_vec_push(mesh->debug_lines.colors, *(ta_rgba *)&face_tangent_color);
#endif
    }
    for (; i < normals_count; i++) {
        ta_vec3 vertex_normal = vec3_scalef(mesh->normals[i], scale);
        line.p0 = mesh->positions[i];
        line.p1 = vec3_add(mesh->positions[i], vertex_normal);
        dlb_vec_push(mesh->debug_lines.positions, line.p0);
        dlb_vec_push(mesh->debug_lines.positions, line.p1);
        dlb_vec_push(mesh->debug_lines.colors, *(ta_rgba *)&vertex_normal_color);
        dlb_vec_push(mesh->debug_lines.colors, *(ta_rgba *)&vertex_normal_color);
    }
    mesh->debug_lines.vertex_count = dlb_vec_len(mesh->debug_lines.positions);

    //=========================================================================
    // Update GL buffers
    //=========================================================================
    DLB_ASSERT(mesh->debug_lines.gl_vao);
    glBindVertexArray(mesh->debug_lines.gl_vao);

    // Calculate size of all vertex attribute data
    size_t vertex_size_total = 0;
    vertex_size_total += dlb_vec_size(mesh->debug_lines.colors);
    vertex_size_total += dlb_vec_size(mesh->debug_lines.positions);
    DLB_ASSERT(vertex_size_total);

    // Create/fill vertex attribute buffer
    DLB_ASSERT(mesh->debug_lines.gl_vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, mesh->debug_lines.gl_vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, vertex_size_total, 0, GL_STATIC_DRAW);

#define FILL_BUFFER(shader_attr, data, c_type, gl_type)                                       \
    if (data) {                                                                               \
        size_t vertex_size = dlb_vec_size(data);                                              \
        glEnableVertexAttribArray(shader_attr);                                               \
        glVertexAttribPointer(shader_attr, sizeof(*data) / sizeof(c_type), gl_type, false, 0, \
            (void *)vertex_offset);                                                           \
        glBufferSubData(GL_ARRAY_BUFFER, vertex_offset, vertex_size, data);                   \
        vertex_offset += vertex_size;                                                         \
    }

    size_t vertex_offset = 0;
    FILL_BUFFER(TA_VERTEX_ATTR_COLOR,    mesh->debug_lines.colors,    GLfloat, GL_FLOAT);
    FILL_BUFFER(TA_VERTEX_ATTR_POSITION, mesh->debug_lines.positions, GLfloat, GL_FLOAT);

#undef FILL_BUFFER

    DLB_ASSERT(vertex_offset == vertex_size_total);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    //=========================================================================
    // Free CPU buffers
    //=========================================================================
    dlb_vec_free(mesh->debug_lines.positions);
    dlb_vec_free(mesh->debug_lines.colors);
}
void ta_mesh_render_debug_lines(ta_mesh *mesh)
{
    DLB_ASSERT(mesh->debug_lines.gl_vao);
    DLB_ASSERT(mesh->debug_lines.gl_vertex_buffer);
    DLB_ASSERT(mesh->debug_lines.vertex_count);

    ta_shader_bind(tg_shader_lines);
    glBindVertexArray(mesh->debug_lines.gl_vao);
    glDrawArrays(GL_LINES, 0, (GLsizei)mesh->debug_lines.vertex_count);
    glBindVertexArray(0);
    ta_shader_unbind();
}
void ta_mesh_render(ta_mesh *mesh, ta_shader *shader)
{
    // TODO: If we want to use binding point 1 for other things in other shaders, then this mapping needs to be a
    // bit more abstract.
    if (mesh->skin.gl_ubo_bone_xforms) {
        glBindBufferBase(GL_UNIFORM_BUFFER, TA_GLSL_UBO_BONE_XFORMS, mesh->skin.gl_ubo_bone_xforms);
        if (mesh->skin.bone_xforms_dirty) {
            glBufferData(GL_UNIFORM_BUFFER, sizeof(mesh->skin.bone_xforms), mesh->skin.bone_xforms, GL_DYNAMIC_DRAW);
        }
    } else {
        glBindBufferBase(GL_UNIFORM_BUFFER, TA_GLSL_UBO_BONE_XFORMS, tg_mesh_gl_default_bone_xforms);
    }

    if (mesh->skin.gl_ubo_bone_normal_xforms) {
        glBindBufferBase(GL_UNIFORM_BUFFER, TA_GLSL_UBO_BONE_NORMAL_XFORMS, mesh->skin.gl_ubo_bone_normal_xforms);
        if (mesh->skin.bone_normal_xforms_dirty) {
            glBufferData(GL_UNIFORM_BUFFER, sizeof(mesh->skin.bone_normal_xforms), mesh->skin.bone_normal_xforms, GL_DYNAMIC_DRAW);
        }
    } else {
        glBindBufferBase(GL_UNIFORM_BUFFER, TA_GLSL_UBO_BONE_NORMAL_XFORMS, tg_mesh_gl_default_bone_xforms);
    }

    if (!mesh->gl_vao) {
        mesh = ta_game_by_sym(RES_MESH, tg_mesh_default);
    }

    glBindVertexArray(mesh->gl_vao);
    if (mesh->index_arrays) {
        dlb_vec_each(ta_index_array *, index_array, mesh->index_arrays) {
            // TODO: Material slots
            // TODO: Bind all needed materials at once via a UBO? Pass material indices as uniform/attrib?
            // https://www.khronos.org/opengl/wiki/Uniform_Buffer_Object
            UNUSED(shader);
            //ta_shader_set_uint(shader, SYM_U_MATERIAL_SLOT, index_array->material_slot);

            size_t index_count = dlb_vec_len(index_array->values);
            DLB_ASSERT(index_count);
            glDrawElements(GL_TRIANGLES, (GLsizei)index_count, GL_UNSIGNED_SHORT, (void *)index_array->offset_bytes);
        }
    } else {
        size_t positions_count = dlb_vec_len(mesh->positions);
        glDrawArrays(GL_TRIANGLES, 0, (GLsizei)positions_count);
    }

    glBindVertexArray(0);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void ta_mesh_free(ta_mesh *mesh)
{
    for (int i = 0; i < TA_VERTEX_ATTR_COUNT; ++i) {
        dlb_vec_free(mesh->buffers[i]);
    }
    dlb_vec_each(ta_index_array *, index_array, mesh->index_arrays) {
        dlb_vec_free(index_array->values);
    }
    dlb_vec_free(mesh->debug_lines.colors);
    dlb_vec_free(mesh->debug_lines.positions);

    glDeleteVertexArrays(1, &mesh->gl_vao);
    glDeleteVertexArrays(1, &mesh->debug_lines.gl_vao);
    glDeleteBuffers(1, &mesh->gl_vertex_buffer);
    glDeleteBuffers(1, &mesh->gl_index_buffer);
    glDeleteBuffers(1, &mesh->debug_lines.gl_vertex_buffer);
}
void ta_mesh_free_void(void *mesh)
{
    ta_mesh_free(mesh);
}