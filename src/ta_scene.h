#pragma once
#include "ta_uid.h"
#include "ta_schema.h"
#include "dlb/dlb_hash.h"
#include "dlb/dlb_pool.h"

struct ta_shader;
struct ta_file;
struct ta_camera;

typedef struct ta_scene {
    const char *filename;
    const char *name;

    //|-------------------------------------------------------------------------
    //| Find all textures:
    //|
    //|   for (u32 idx = 0; idx < resource_uids[RES_TEXTURE].size; ++idx) {
    //|       ta_uid *uid = dlb_pool_at(&resource_uids[RES_TEXTURE], idx);
    //|       ta_texture *texture = dlb_pool_at(&resource_data[RES_TEXTURE], idx);
    //|       const char *name = dlb_pool_at(&resource_names[RES_TEXTURE], idx);
    //|   }
    //|
    //| Find all entities with a model component:
    //|
    //|   for (u32 idx = 0; idx < resource_uids[RES_COMP_MODEL].size; ++idx) {
    //|       ta_uid *uid = dlb_pool_at(&resource_uids[RES_COMP_MODEL], idx);
    //|       ta_model *model = dlb_pool_at(&resource_data[RES_COMP_MODEL], idx);
    //|       const char *name = dlb_pool_at(&resource_names[RES_COMP_MODEL], idx);
    //|   }
    //|
    //| Find all components for a given entity:
    //|
    //|   ta_uid entity_uid = { 42, 0 };
    //|   ta_uid resource_uid = dlb_pool_at(&resource_uids, entity_uid.index);
    //|   DLB_ASSERT(entity_uid.generation == resource_uid.generation);
    //|   ta_entity *entity = dlb_pool_at(&resource_data[RES_ENTITY], entity_uid.index);
    //|   ta_uid model_uid = entity->components[RES_MODEL];
    //|
    //|-------------------------------------------------------------------------
    dlb_pool resource_uids[RES_COUNT];  // dense set of resource uids (ta_uid *)
    dlb_pool resource_names[RES_COUNT]; // dense set of resource names (const char *)
    dlb_pool resource_data[RES_COUNT];  // dense set of resource data (e.g. ta_texture *)

    // DEBUG: Map resource names to their current uid
    dlb_hash resource_uid_by_name[RES_COUNT]; // const char *name -> ta_uid

#if 0
    // These pools should stay in sync
    dlb_pool resource_data[RES_COUNT];    // dense set of resource data, by type
    dlb_pool resources[RES_COUNT];        // dense set of resource uids, by type
    // NOTE: If uid not present, resource has been unloaded
    dlb_hash resources_by_uid[RES_COUNT]; // u32 uid -> u32 index
    dlb_hash resource_names;              // u32 uid -> const char *name

    // These pools should stay in sync
    dlb_pool components[COMP_COUNT]; // dense set of component data, by type
    dlb_pool entities[COMP_COUNT];   // dense set of entity uids, by type
    // NOTE: If uid not present, entity has been deleted
    dlb_hash entities_by_uid;        // u32 uid -> component_set
    dlb_hash entity_names;           // u32 uid -> const char *name
#endif
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
ta_entity *ta_scene_alloc_entity(ta_scene *scene, const char *uid);
void *ta_scene_entity_add_component(ta_scene *scene, ta_entity *entity,
    ta_component_type type);
void *ta_scene_component(ta_scene *scene, ta_entity *entity,
    ta_component_type type);
void *ta_scene_component_or_default(ta_scene *scene,
    ta_entity *entity, ta_component_type type);
bool ta_scene_entity_valid(ta_scene *scene, ta_entity entity);
void ta_scene_entity_validate(ta_scene *scene, ta_entity entity);
//ta_entity *ta_scene_entity_by_uid_try(ta_scene *scene, const char *uid);
//ta_entity *ta_scene_entity_by_uid(ta_scene *scene, const char *uid);
void ta_scene_update(ta_scene *scene, float dt);
void ta_scene_shadow_pass(ta_scene *scene, struct ta_shader *shader, float alpha);
void ta_scene_render(ta_scene *scene, struct ta_camera *camera, float alpha);