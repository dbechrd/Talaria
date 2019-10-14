#pragma once
#include "ta_uid.h"
#include "ta_schema.h"
#include "dlb/dlb_hash.h"
#include "dlb/dlb_pool.h"

struct ta_shader;
struct ta_file;
struct ta_camera;

typedef enum ta_resource_type {
    RES_COMP_AUDIO_SOURCE,
    RES_COMP_BUTTON,
    RES_COMP_CAMERA,
    RES_COMP_LIGHT,
    RES_COMP_MODEL,
    RES_COMP_POSITION,
    RES_COMP_RIGID_BODY,
    RES_COMP_COUNT,

    RES_AUDIO_BUFFER = RES_COMP_COUNT,
    RES_ENTITY,
    RES_FONT,
    RES_MATERIAL,
    RES_MESH_GROUP,
    RES_SHADER,
    RES_TEXTURE,
    RES_COUNT,
} ta_resource_type;

extern ta_schema_field_type ta_resource_types[RES_COUNT];

// TODO: Binary format should just store component pools as-is, with the uid
// pool intact to map data to entity uid. This struct is for human-readable
// scene files where we want the notion of entities being a collection of
// components more explicitly:
//
//  ta_transform: {
//    uid: "player_transform"
//    position: { x: 0.0, y: 0.0, z: 0.0 }
//  }
//
//  ta_entity: {
//    uid: "player"
//    components: ["player_transform", "player_mesh", "player_audio_source"]
//  }
//
typedef struct ta_entity {
    dlb_id components[RES_COMP_COUNT];
} ta_entity;

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

    //dlb_id *resource_ids[RES_COUNT];    // vector of resource ids by type
    dlb_pool resource_names[RES_COUNT]; // dense set of resource names (const char *)
    dlb_pool resource_data[RES_COUNT];  // dense set of resource data (e.g. ta_texture *)

    dlb_hash resource_id_by_name[RES_COUNT]; // const char *name -> ta_uid
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
dlb_id ta_scene_entity_create(ta_scene *scene, const char *name);
void ta_scene_entity_destroy(ta_scene *scene, dlb_id entity_id);
void *ta_scene_add_component(ta_scene *scene, dlb_id entity_id,
    ta_resource_type type);
void *ta_scene_get_component(ta_scene *scene, dlb_id entity_id,
    ta_resource_type type);
void *ta_scene_get_component_or_default(ta_scene *scene, dlb_id entity_id,
    ta_resource_type type);
bool ta_scene_entity_valid(ta_scene *scene, ta_entity entity);
void ta_scene_entity_validate(ta_scene *scene, ta_entity entity);
//ta_entity *ta_scene_entity_by_uid_try(ta_scene *scene, const char *uid);
//ta_entity *ta_scene_entity_by_uid(ta_scene *scene, const char *uid);
void ta_scene_update(ta_scene *scene, float dt);
void ta_scene_shadow_pass(ta_scene *scene, struct ta_shader *shader, float alpha);
void ta_scene_render(ta_scene *scene, struct ta_camera *camera, float alpha);