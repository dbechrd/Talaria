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

    ta_vec3 group_min = VEC3_MAX;
    ta_vec3 group_max = VEC3_MIN;
    float group_radius = 0.0f;
    ta_vec3 mesh_min;
    ta_vec3 mesh_max;
    float mesh_radius;

    // Each object in file
    for (size_t i = 0; i < num_shapes; i++) {
        tinyobj_shape_t shape = shapes[i];
        size_t offset = 0;

        ta_mesh *mesh = dlb_vec_alloc(group->meshes);

        mesh_min = VEC3_MAX;
        mesh_max = VEC3_MIN;
        mesh_radius = 0.0f;

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

                mesh_min.x = MIN(mesh_min.x, pos->x);
                mesh_min.y = MIN(mesh_min.y, pos->y);
                mesh_min.z = MIN(mesh_min.z, pos->z);
                mesh_max.x = MAX(mesh_max.x, pos->x);
                mesh_max.y = MAX(mesh_max.y, pos->y);
                mesh_max.z = MAX(mesh_max.z, pos->z);
                mesh_radius = MAX(mesh_radius, vec3_len(*pos));
            }
        }

        ta_mesh_create(mesh);

        u32 name_len = (u32)strlen(shapes[i].name);
        mesh->name = ta_symbol_intern(shapes[i].name, name_len);

        mesh->aabb.extents = vec3_scalef(vec3_sub(mesh_max, mesh_min), 0.5f);
        mesh->aabb.center = vec3_add(mesh_min, mesh->aabb.extents);
        mesh->sphere.center = mesh->aabb.center;
        mesh->sphere.radius = MIN(mesh_radius, vec3_len(mesh->aabb.extents));

        group_min.x = MIN(group_min.x, mesh_min.x);
        group_min.y = MIN(group_min.y, mesh_min.y);
        group_min.z = MIN(group_min.z, mesh_min.z);
        group_max.x = MAX(group_max.x, mesh_max.x);
        group_max.y = MAX(group_max.y, mesh_max.y);
        group_max.z = MAX(group_max.z, mesh_max.z);
        group_radius = MAX(group_radius, mesh_radius);

        ta_mesh_init_normals(mesh, TA_MESH_NORMAL_LEN);

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

    group->aabb.extents = vec3_scalef(vec3_sub(group_max, group_min), 0.5f);
    group->aabb.center = vec3_add(group_min, group->aabb.extents);
    group->sphere.center = group->aabb.center;
    group->sphere.radius = MIN(group_radius, vec3_len(group->aabb.extents));

    tinyobj_attrib_free(&attrib);
    tinyobj_shapes_free(shapes, num_shapes);
    tinyobj_materials_free(materials, num_materials);
    ta_buffer_free(buf);
}

#if 0
ta_aabb ta_mesh_group_aabb(ta_mesh_group *group)
{
    ta_vec3 min = VEC3_MAX;
    ta_vec3 max = VEC3_MIN;
    ta_aabb mesh_aabb;
    ta_vec3 mesh_min;
    ta_vec3 mesh_max;

    dlb_vec_each(ta_mesh *, mesh, group->meshes) {
        mesh_aabb = ta_mesh_aabb(mesh);
        mesh_min = vec3_sub(mesh_aabb.center, mesh_aabb.extents);
        mesh_max = vec3_add(mesh_aabb.center, mesh_aabb.extents);

        min.x = MIN(min.x, mesh_min.x);
        min.y = MIN(min.y, mesh_min.y);
        min.z = MIN(min.z, mesh_min.z);
        max.x = MAX(max.x, mesh_max.x);
        max.y = MAX(max.y, mesh_max.y);
        max.z = MAX(max.z, mesh_max.z);
    }

    ta_aabb aabb = { 0 };
    aabb.extents = vec3_scalef(vec3_sub(max, min), 0.5f);
    aabb.center = vec3_add(min, aabb.extents);
    return aabb;
}
#endif

void ta_mesh_group_push_normals(ta_mesh_group *group)
{
    dlb_vec_each(ta_mesh *, mesh, group->meshes) {
        ta_mesh_push_normals(mesh);
    }
}

void ta_mesh_group_render(ta_mesh_group *group)
{
    dlb_vec_each(ta_mesh *, mesh, group->meshes) {
        ta_mesh_render(mesh);
    }
}

void ta_mesh_group_free(ta_mesh_group *group)
{
    dlb_vec_each(ta_mesh *, mesh, group->meshes) {
        ta_mesh_free(mesh);
    }
    dlb_vec_free(group->meshes);
}