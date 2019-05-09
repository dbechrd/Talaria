#include "ta_mesh_group.h"
#include "ta_mesh.h"
#include "ta_primitive.h"
#include "ta_symbol.h"
#include "ta_file.h"
#include "dlb_vector.h"

#define TINYOBJ_MALLOC dlb_malloc
#define TINYOBJ_CALLOC dlb_calloc
#define TINYOBJ_REALLOC dlb_realloc
#define TINYOBJ_FREE dlb_free
#define TINYOBJ_LOADER_C_IMPLEMENTATION
#include "misc/tinyobj_loader_c.h"

#define TA_MESH_NORMAL_LEN 0.5f

void ta_mesh_group_init(ta_mesh_group *group, const char *uid, const char *path)
{
    group->uid = uid;
    group->path = path;
}

void ta_mesh_group_load(ta_mesh_group *group)
{
    DLB_ASSERT(group->path);

    // Load OBJ file
    // =========================================================================

    ta_buffer *buf = ta_file_read_all(group->path);
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

        ta_mesh *mesh = dlb_vec_alloc(group->meshes);

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

        ta_mesh_create(mesh);

        u32 name_len = (u32)strlen(shapes[i].name);
        mesh->name = ta_symbol_intern(shapes[i].name, name_len);

        ta_mesh_init_vertex_normals(mesh, TA_MESH_NORMAL_LEN);
        ta_mesh_init_face_normals(mesh, TA_MESH_NORMAL_LEN);

#if 0
        if (!group->meshes_by_name.size) {
            dlb_hash_init(&group->meshes_by_name, DLB_HASH_STRING, group->uid, num_shapes);
            if (!group->meshes_by_name.size) {
                DLB_ASSERT(!"Failed to initialize mesh group hash table");
            }
        }
        dlb_hash_insert(&group->meshes_by_name, SYM(mesh->name), mesh);
#endif
    }

    tinyobj_attrib_free(&attrib);
    tinyobj_shapes_free(shapes, num_shapes);
    tinyobj_materials_free(materials, num_materials);
    ta_buffer_free(buf);
}

void ta_mesh_group_push_normals(ta_mesh_group *group)
{
    for (ta_mesh *dlb_vec_iter(group->meshes, mesh)) {
        ta_mesh_push_normals(mesh);
    }
}

void ta_mesh_group_render(ta_mesh_group *group)
{
    for (ta_mesh *dlb_vec_iter(group->meshes, mesh)) {
        ta_mesh_render(mesh);
    }
}

void ta_mesh_group_free(ta_mesh_group *group)
{
    for (ta_mesh *dlb_vec_iter(group->meshes, mesh)) {
        ta_mesh_free(mesh);
    }
    dlb_vec_free(group->meshes);
}