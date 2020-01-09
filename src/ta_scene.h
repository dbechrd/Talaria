#pragma once
#include "ta_schema.h"
#include "dlb/dlb_hash.h"
#include "dlb/dlb_index.h"

struct ta_file;
struct ta_buffer;
struct ta_shader;
struct ta_camera;
enum ta_resource_type;

//|-----------------------------------------------------------------------------
//|
//|   TODO: Doc comment or delete placeholder if it becomes obvious
//|
//|-----------------------------------------------------------------------------
typedef struct ta_scene {
    const char *filename;
    const char *name;
    void *resource_data[RES_COUNT];        // dense resource data (e.g. ta_texture *)
    dlb_index index_by_name[RES_COUNT];    // name hash -> dense index
    dlb_index index_by_entity[RES_COUNT];  // entity hash -> dense index

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

// Resources
void *ta_scene_alloc(ta_scene *scene, enum ta_resource_type type, const char *name, u32 name_len);
void ta_scene_destroy(ta_scene *scene, enum ta_resource_type type, const char *name, u32 name_len);
void *ta_scene_find_at(ta_scene *scene, enum ta_resource_type type, u32 index);
void *ta_scene_find_try(ta_scene *scene, enum ta_resource_type type, const char *name, u32 name_len);
void *ta_scene_find(ta_scene *scene, enum ta_resource_type type, const char *name, u32 name_len);
void *ta_scene_find_or_default(ta_scene *scene, enum ta_resource_type type, const char *name, u32 name_len);

// Entities/components (just resources w/ special name and some extra error handling)
void *ta_scene_component_add(ta_scene *scene, const char *entity, enum ta_resource_type type, const char *name, u32 name_len);
void *ta_scene_component_try(ta_scene *scene, const char *entity, enum ta_resource_type type);
void *ta_scene_component(ta_scene *scene, const char *entity, enum ta_resource_type type);

void ta_scene_update(ta_scene *scene, float dt);
void ta_scene_shadow_pass(ta_scene *scene, float alpha);
void ta_scene_render(ta_scene *scene, struct ta_camera *render_camera, float alpha);