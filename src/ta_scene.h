#pragma once
#include "ta_schema.h"
#include "dlb/dlb_hash.h"
#include "dlb/dlb_index.h"

struct ta_file;
struct ta_buffer;
struct ta_shader;
struct ta_camera;
enum ta_resource_type;

//|-------------------------------------------------------------------------
//| Find all textures:
//|
//|   for (u32 idx = 0; idx < resource_ids[RES_TEXTURE].size; ++idx) {
//|       dlb_id id = dlb_pool_at(&resource_ids[RES_TEXTURE], idx);
//|       ta_texture *texture = dlb_pool_at(&resource_data[RES_TEXTURE], idx);
//|       const char *name = dlb_pool_at(&resource_names[RES_TEXTURE], idx);
//|   }
//|
//| Find all entities with a model component:
//|
//|   for (u32 idx = 0; idx < resource_ids[RES_COMP_MODEL].size; ++idx) {
//|       dlb_id id = dlb_pool_at(&resource_ids[RES_COMP_MODEL], idx);
//|       ta_model *model = dlb_pool_at(&resource_data[RES_COMP_MODEL], idx);
//|       const char *name = dlb_pool_at(&resource_names[RES_COMP_MODEL], idx);
//|   }
//|
//| Find all components for a given entity:
//|
//|   dlb_id entity_id = { 42, 0 };
//|   ta_entity *entity = dlb_pool_at(&resource_data[RES_ENTITY], entity_id);
//|   dlb_id model_id = entity->components[RES_MODEL];
//|
//|-------------------------------------------------------------------------
typedef struct ta_scene {
    const char *filename;
    const char *name;
    void *resource_data[RES_COUNT];         // dense resource data (e.g. ta_texture *)
    dlb_index index_by_name[RES_COUNT];     // name hash -> dense index

    // Internal data, need to persist between frames for when sim is paused
    struct {
        struct ta_manifold *manifolds;
        struct ta_rigid_body_pair *pairs;
    } data;
} ta_scene;

/*
typedef struct cc_vector {
    void *begin;         // dlb_vec
    void *end;           // dlb_vec
    size_t capacity;     // current capacity
    char buffer[16];     // C++: sizeof(T) * count
} cc_vector;
*/

void ta_scene_init(ta_scene *scene);
void ta_scene_load(ta_scene *scene, struct ta_file *file);
void ta_scene_load_file(ta_scene *scene, const char *filename);
void ta_scene_free(ta_scene *scene);
void ta_scene_save(struct ta_buffer *buffer);
void ta_scene_save_file(ta_scene *scene, const char *filename);
void ta_scene_print(ta_scene *scene, FILE *hnd);
void ta_scene_save_file_json(ta_scene *scene, const char *filename);
void ta_scene_print_json(ta_scene *scene, FILE *hnd);
void *ta_scene_alloc(ta_scene *scene, enum ta_resource_type type, const char *name, u32 name_len);
void ta_scene_destroy(ta_scene *scene, enum ta_resource_type type, const char *name, u32 name_len);
void *ta_scene_find_at(ta_scene *scene, enum ta_resource_type type, u32 index);
void *ta_scene_find_by_name_try(ta_scene *scene, enum ta_resource_type type, const char *name, u32 name_len);
void *ta_scene_find_by_name(ta_scene *scene, enum ta_resource_type type, const char *name, u32 name_len);
void *ta_scene_find_by_name_or_default(ta_scene *scene, enum ta_resource_type type, const char *name, u32 name_len);
void *ta_scene_component_add(ta_scene *scene, enum ta_resource_type type, const char *entity);
void *ta_scene_component_try(ta_scene *scene, enum ta_resource_type type, const char *entity);
void *ta_scene_component(ta_scene *scene, enum ta_resource_type type, const char *entity);
void ta_scene_update(ta_scene *scene, float dt);
void ta_scene_shadow_pass(ta_scene *scene, struct ta_shader *shader, float alpha);
void ta_scene_render(ta_scene *scene, struct ta_camera *render_camera, float alpha);