#include "ta_audio.h"
#include "ta_buffer.h"
#include "ta_button.h"
#include "ta_camera.h"
#include "ta_editor.h"
#include "ta_file.h"
#include "ta_font.h"
#include "ta_game.h"
#include "ta_intersect.h"
#include "ta_light.h"
#include "ta_log.h"
#include "ta_material.h"
#include "ta_model.h"
#include "ta_parse.h"
#include "ta_transform.h"
#include "ta_primitive.h"
#include "ta_rigid_body.h"
#include "ta_scene.h"
#include "ta_shader.h"
#include "ta_symbol.h"
#include "ta_texture.h"
#include "ta_token.h"
#include "ta_window.h"
#include "dlb/dlb_vector.h"
#include "dlb/dlb_index.h"
#include <stdlib.h>
#include <float.h>

static void scene_load_placeholders(ta_scene *scene)
{
    UNUSED(scene);
    // TODO: Fix fallback resources
#if 0
    // Fallback resources
    ta_texture *tex_albedo = ta_scene_alloc(scene, COMP_TEXTURE,
        INTERN("DEFAULT_TEXTURE_ALBEDO"));
    {
#if 0
        tex_albedo->path = INTERN("data/texture/default_1024_1024.png");
#else
        // Generate magenta/white grid pattern
        tex_albedo->width = 64;
        tex_albedo->height = 64;
        tex_albedo->channels = 3;
        tex_albedo->linear = true;
        u8 *albedo_pixels = 0;
        u32 bytes = tex_albedo->width * tex_albedo->height * tex_albedo->channels;
        dlb_vec_reserve(albedo_pixels, bytes);
        u8 toggle = 0;
        u8 toggle_width = 4;
        for (int y = 0; y < tex_albedo->height; y++) {
            if (y % toggle_width == 0) toggle = !toggle;
            for (int x = 0; x < tex_albedo->width; x++) {
                if (x % toggle_width == 0) toggle = !toggle;
                dlb_vec_push(albedo_pixels, 255);
                dlb_vec_push(albedo_pixels, toggle * 255);
                dlb_vec_push(albedo_pixels, 255);
            }
        }
        DLB_ASSERT(dlb_vec_len(albedo_pixels) == bytes);
        tex_albedo->pixels = albedo_pixels;
#endif
    }

    ta_texture *tex_metallic = ta_scene_alloc(scene, COMP_TEXTURE,
        INTERN("DEFAULT_TEXTURE_METALLIC"));
    {
#if 0
        tex_metallic->path = INTERN("data/texture/default_1024_1024.png");
#else
        tex_metallic->width = 1;
        tex_metallic->height = 1;
        tex_metallic->channels = 1;
        tex_metallic->linear = true;
        u8 *metallic = 0;
        dlb_vec_alloc(metallic);
        tex_metallic->pixels = metallic;
#endif
    }

    ta_material *material = ta_scene_alloc(scene, COMP_MATERIAL,
        INTERN("DEFAULT_MATERIAL"));
    // TODO: Hard-code default shader instead of hoping it's in the scene file
    material->shader_uid = INTERN("shader_mesh");
    material->texture_albedo_uid = tex_albedo->hnd.uid;
    material->texture_metallic_uid = tex_metallic->hnd.uid;

    ta_mesh_group *mesh_group = ta_scene_alloc(scene, COMP_MESH_GROUP,
        INTERN("DEFAULT_MESH_GROUP"));
    mesh_group->path = INTERN("data/mesh/default.obj");

    scene->components[COMP_MATERIAL][0] = material->hnd.uid;
    scene->components[COMP_MESH_GROUP][0] = mesh_group->hnd.uid;
#endif
}

void ta_scene_init(ta_scene *scene)
{
    DLB_ASSERT(scene->filename);
    if (!scene->name) {
        scene->name = scene->filename;  // TODO: Load name from scene file
    }

    // TODO(perf): Fine-tune reservations (e.g. scene header)
    // TODO(perf): This is a lot of back-to-back allocations, can we avoid?
    for (ta_resource_type type = 0; type < RES_COUNT; type++) {
        dlb_index_init(&scene->index_by_name[type], 128, 128);
        dlb_index_init(&scene->index_by_entity[type], 128, 128);
    }
    scene_load_placeholders(scene);
}
// TODO: This should take a ta_buffer pointer. Load entire file into memory
//       and refactor all of the e.g. read_char and expect_char logic out from
//       ta_file into ta_buffer.
void ta_scene_load(ta_scene *scene, ta_file *file)
{
    ta_log_write(&tg_debug_log, SRC_SCENE, "Loading %s\n", file->filename);
    scene->filename = file->filename;
    scene->name = file->filename;  // TODO: Load name from scene file
    ta_scene_init(scene);

    // TODO: Reserve arrays based on scene header (which doesn't exist yet)
    //dlb_vec_reserve(scene->entities, 2);
    token *tokens = tokenize(file);

    //tokens_print(tg_debug_log->stream, tokens);
    //tokens_print_debug(tg_debug_log.stream, tokens);
    tokens_parse(scene, tokens);
    dlb_vec_free(tokens);

    // Initialize resources
    // NOTE: Iterate backward to ensure resources are initialized before
    // components, which might depend on them.
    for (ta_resource_type res_type = RES_COUNT - 1; res_type >= 0; --res_type) {
        ta_schema_field_type schema_type = res_to_typ(res_type);
        if (tg_schemas[schema_type].init) {
            ta_log_write(&tg_debug_log, SRC_SCENE, "Initializing %s\n",
                ta_schema_field_type_str(schema_type));
            size_t size = tg_schemas[schema_type].size;
            void *pool = scene->resource_data[res_type];
            u8 *end = dlb_vec_end_size(pool, size);
            for (u8 *ptr = pool; ptr != end; ptr += size) {
                tg_schemas[schema_type].init(ptr);
            }
        }
    }
}
void ta_scene_load_file(ta_scene *scene, const char *filename)
{
    //ta_buffer *buffer = ta_file_read_all(filename);
    //ta_scene *scene = ta_scene_load(buffer);
    ta_file *file = ta_file_open(filename, FILE_READ);
    ta_scene_load(scene, file);
}
void ta_scene_free(ta_scene *scene)
{
    // Free resources
    for (ta_resource_type res_type = 0; res_type < RES_COUNT; ++res_type) {
        ta_schema_field_type schema_type = res_to_typ(res_type);
        if (tg_schemas[schema_type].free) {
            size_t size = tg_schemas[schema_type].size;
            void *pool = scene->resource_data[res_type];
            u8 *end = dlb_vec_end_size(pool, size);
            for (u8 *ptr = pool; ptr != end; ptr += size) {
                tg_schemas[schema_type].free(ptr);
            }
        }
        dlb_vec_free(scene->resource_data[res_type]);
        dlb_index_free(&scene->index_by_name[res_type]);
    }
}
void ta_scene_save(ta_buffer *buffer)
{
    UNUSED(buffer);
    // TODO: Write scene to memory buffer
    DLB_ASSERT(0);
}
void ta_scene_save_file(ta_scene *scene, const char *filename)
{
    // TODO: Alloc dynamic buffer to write arbitrary data to
    //ta_buffer *buffer = ??
    ta_file *file = ta_file_open(filename, FILE_WRITE);
    ta_scene_print(scene, file->hnd);
    ta_file_close(file);
}
void ta_scene_print(ta_scene *scene, FILE *hnd)
{
    fprintf(hnd, "#-------------------------------------------------------------------------------\n");
    fprintf(hnd, "# [SCENE] %s\n", scene->name);
    for (ta_resource_type res_type = 0; res_type < RES_COUNT; ++res_type) {
        ta_schema_field_type schema_type = res_to_typ(res_type);
        fprintf(hnd, "#-------------------------------------------------------------------------------\n");
        fprintf(hnd, "# %s\n", ta_schema_field_type_str(schema_type));
        fprintf(hnd, "#-------------------------------------------------------------------------------\n");

        size_t size = tg_schemas[schema_type].size;
        void *pool = scene->resource_data[res_type];
        u8 *end = dlb_vec_end_size(pool, size);
        for (u8 *ptr = pool; ptr != end; ptr += size) {
            ta_schema_print(hnd, schema_type, ptr, 0, 0);
        }
    }
    fflush(hnd);
}
void ta_scene_save_file_json(ta_scene *scene, const char *filename)
{
    // TODO: Alloc dynamic buffer to write arbitrary data to
    //ta_buffer *buffer = ??
    ta_file *file = ta_file_open(filename, FILE_WRITE);
    ta_scene_print_json(scene, file->hnd);
    ta_file_close(file);
}
void ta_scene_print_json(ta_scene *scene, FILE *f)
{
    fprintf(f, "{\n");
    for (ta_resource_type res_type = 0; res_type < RES_COUNT; ++res_type) {
        ta_schema_field_type schema_type = res_to_typ(res_type);
        // NOTE: We may have mesh properties at some point, but, for now, meshes are
        // created at run-time via mesh_groups.
        if (res_type == RES_MESH) {
            continue;
        }

        ta_schema *schema = &tg_schemas[schema_type];

        fprintf(f, "  \"%s\": [\n", schema->name);

        void *pool = scene->resource_data[res_type];
        size_t pool_len = dlb_vec_len(pool);
        u8 *ptr = pool;
        for (size_t i = 0; i < pool_len; ++i) {
            fprintf(f, "    {\n");
            ta_schema_print_json(f, schema_type, ptr, 2, 0);
            fprintf(f, "    }");
            if (pool_len && i < pool_len - 1) {
                fprintf(f, ",");
            }
            fprintf(f, "\n");
            ptr += schema->size;
        }

        fprintf(f, "  ]");
        if (res_type < RES_COUNT - 1) {
            fprintf(f, ",");
        }
        fprintf(f, "\n");
    }
    fprintf(f, "}\n");
    fflush(f);
}
void *ta_scene_alloc(ta_scene *scene, ta_resource_type type, const char *name,
    size_t name_len)
{
    DLB_ASSERT(scene);
    DLB_ASSERT(type < RES_COUNT);
    DLB_ASSERT(name);
    DLB_ASSERT(name_len);

    ta_schema_field_type schema_type = res_to_typ(type);
    size_t size = tg_schemas[schema_type].size;

    ta_resource *res = dlb_vec_alloc_size(scene->resource_data[type], size);
    res->index = dlb_vec_len(scene->resource_data[type]) - 1;
    res->name = ta_symbol_intern(name, name_len);

    dlb_index *store = &scene->index_by_name[type];
    u32 hash = dlb_murmur3(SYM32(res->name));
    dlb_index_insert(store, hash, res->index);

    if (tg_schemas[schema_type].init) {
        tg_schemas[schema_type].init(res);
    }

    return res;
}
void ta_scene_destroy(ta_scene *scene, ta_resource_type type, const char *name,
    size_t name_len)
{
    DLB_ASSERT(scene);
    // TODO: if type is a component type, find and update parent entity:
    // entity->components[type] = 0
    DLB_ASSERT(type >= RES_COMP_COUNT && type < RES_COUNT);

    ta_schema_field_type schema_type = res_to_typ(type);
    size_t size = tg_schemas[schema_type].size;

    // TODO: Find resource
    DLB_ASSERT(0);
    ta_resource *res = 0000000;
    if (tg_schemas[schema_type].free) {
        tg_schemas[schema_type].free(res);
    }

    // Remove name from index
    dlb_index *store = &scene->index_by_name[type];
    u32 hash = dlb_murmur3(name, (u32)name_len);
    // TODO: Find index
    DLB_ASSERT(0);
    u32 index = 0000000;
    dlb_index_delete(store, hash, index);

    // TODO: Remove data from pool (swap last element into empty slot, then
    // update index for the moved element)
    DLB_ASSERT(0);
    //dlb_vec_delete(scene->resource_data[type], index);
}
void *ta_scene_find_at(ta_scene *scene, ta_resource_type type, u32 index)
{
    DLB_ASSERT(scene);
    DLB_ASSERT(type >= 0 && type < RES_COUNT);

    ta_schema_field_type schema_type = res_to_typ(type);
    size_t size = tg_schemas[schema_type].size;
    void *resource = dlb_vec_index_size(scene->resource_data[type], index, size);
    return resource;
}
// If not found, returns NULL
void *ta_scene_find_try(ta_scene *scene, ta_resource_type type, const char *name,
    size_t name_len)
{
    DLB_ASSERT(scene);
    DLB_ASSERT(name);

    ta_schema_field_type schema_type = res_to_typ(type);
    size_t size = tg_schemas[schema_type].size;

    u32 hash = dlb_murmur3(name, (u32)name_len);
    dlb_index *store = &scene->index_by_name[type];
    for (size_t i = dlb_index_first(store, hash); i != DLB_INDEX_EMPTY; i = dlb_index_next(store, i)) {
        ta_resource *res = dlb_vec_index_size(scene->resource_data[type], i, size);
        if (res->name == name) {
            DLB_ASSERT(res->index == i);
            return res;
        }
    }
    return 0;
}
// If not found, ASSERT
void *ta_scene_find(ta_scene *scene, ta_resource_type type,
    const char *name, size_t name_len)
{
    void *resource = ta_scene_find_try(scene, type, name, name_len);
    DLB_ASSERT(resource);
    return resource;
}
// If not found, returns the first resource of the given type
void *ta_scene_find_or_default(ta_scene *scene, ta_resource_type type,
    const char *name, size_t name_len)
{
    ta_schema_field_type schema_type = res_to_typ(type);
    size_t size = tg_schemas[schema_type].size;

    void *resource = ta_scene_find_try(scene, type, name, name_len);
    if (!resource) {
        resource = dlb_vec_index_size(scene->resource_data[type], 0, size);
    }
    return resource;
}
void *ta_scene_component_add(ta_scene *scene, const char *entity,
    ta_resource_type type, const char *name, size_t name_len)
{
    DLB_ASSERT(scene);
    DLB_ASSERT(type >= 0 && type < RES_COMP_COUNT);

    // Prevent duplicates
    ta_component *component = ta_scene_component(scene, entity, type);
    DLB_ASSERT(!component);

    component = ta_scene_alloc(scene, type, name, name_len);
    DLB_ASSERT(component);
    component->entity_name = entity;

    dlb_index *store = &scene->index_by_entity[type];
    u32 hash = dlb_murmur3(SYM32(component->entity_name));
    dlb_index_insert(store, hash, component->index);

    return component;
}
void *ta_scene_component_try(ta_scene *scene, const char *entity,
    ta_resource_type type)
{
    DLB_ASSERT(scene);
    DLB_ASSERT(entity);
    DLB_ASSERT(type >= 0 && type < RES_COMP_COUNT);

    void *component = 0;

    ta_schema_field_type schema_type = res_to_typ(type);
    size_t size = tg_schemas[schema_type].size;

    u32 hash = dlb_murmur3(SYM32(entity));
    dlb_index *store = &scene->index_by_entity[type];
    for (size_t i = dlb_index_first(store, hash); i != DLB_INDEX_EMPTY; i = dlb_index_next(store, i)) {
        ta_component *comp = dlb_vec_index_size(scene->resource_data[type], i, size);
        if (comp->entity_name == entity) {
            DLB_ASSERT(comp->index == i);
            component = comp;
            break;
        }
    }

    return component;
}
void *ta_scene_component(ta_scene *scene, const char *entity,
    ta_resource_type type)
{
    DLB_ASSERT(scene);
    DLB_ASSERT(entity);
    DLB_ASSERT(type >= 0 && type < RES_COMP_COUNT);

    void *component = ta_scene_component_try(scene, entity, type);
    DLB_ASSERT(component);
    return component;
}