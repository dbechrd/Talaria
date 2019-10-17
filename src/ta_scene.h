#pragma once
#include "ta_uid.h"
#include "ta_schema.h"
#include "ta_resource_type.h"
#include "dlb/dlb_hash.h"
#include "dlb/dlb_pool.h"

struct ta_shader;
struct ta_file;
struct ta_camera;
struct ta_entity;

//|-------------------------------------------------------------------------
//|
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
    u32 next_id[RES_COUNT];

    // TODO: Store ids in each resource, then this is probably redundant
    //dlb_pool resource_ids[RES_COUNT];   // vector of resource ids by type

    // NOTE: This could probably be a vector. We don't need generations because
    // presence is determined by resource_names_by_name containing the index,
    // and the const char * in resource_names matching the queried name.
    dlb_pool resource_names[RES_COUNT]; // dense set of resource names (const char **)
    dlb_pool resource_data[RES_COUNT];  // dense set of resource data (e.g. ta_texture *)
    dlb_hash id_by_name[RES_COUNT];     // const char *name -> resource id
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
ta_scene *ta_scene_load(struct ta_file *file);
ta_scene *ta_scene_load_file(const char *filename);
void ta_scene_save_file(ta_scene *scene, const char *filename);
void ta_scene_free(ta_scene *scn);
void ta_scene_print(ta_scene *scn, FILE *hnd);

void *ta_scene_find_at(ta_scene *scene, ta_resource_type type, u32 index);
void *ta_scene_find_by_id_try(ta_scene *scene, ta_resource_type type, u32 id);
void *ta_scene_find_by_id(ta_scene *scene, ta_resource_type type, u32 id);
void *ta_scene_find_by_id_or_default(ta_scene *scene, ta_resource_type type,
    u32 id);

u32 ta_scene_entity_create(ta_scene *scene, const char *name);
void ta_scene_entity_destroy(ta_scene *scene, u32 entity_id);
void *ta_scene_entity_add_component(ta_scene *scene, u32 entity_id,
    ta_resource_type type);
void *ta_scene_entity_component_try(ta_scene *scene, struct ta_entity *entity,
    ta_resource_type type);
void *ta_scene_entity_component(ta_scene *scene, struct ta_entity *entity,
    ta_resource_type type);

//ta_entity *ta_scene_entity_by_uid_try(ta_scene *scene, const char *uid);
//ta_entity *ta_scene_entity_by_uid(ta_scene *scene, const char *uid);
void ta_scene_update(ta_scene *scene, float dt);
void ta_scene_shadow_pass(ta_scene *scene, struct ta_shader *shader, float alpha);
void ta_scene_render(ta_scene *scene, struct ta_camera *camera, float alpha);