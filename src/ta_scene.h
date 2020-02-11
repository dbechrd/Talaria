#pragma once
#include "ta_schema.h"
#include "dlb/dlb_hash.h"
#include "dlb/dlb_index.h"

struct ta_resource;
struct ta_file;
enum ta_res_type;

typedef struct ta_scene {
    const char  *filename;                   // relative path to scene file
    const char  *name;                       // scene name
    ta_resource *resource_data[RES_COUNT];   // dense resource data (e.g. ta_texture *)
    dlb_index   index_by_name[RES_COUNT];    // name hash -> dense index
    dlb_index   index_by_entity[RES_COUNT];  // entity hash -> dense index
} ta_scene;

// Scene
void ta_scene_init              (ta_scene *scene);
void ta_scene_load              (ta_scene *scene, struct ta_file *file);
void ta_scene_load_file         (ta_scene *scene, const char *filename);
void ta_scene_free              (ta_scene *scene);
void ta_scene_save              (ta_scene *scene, char *buffer);
void ta_scene_save_file         (ta_scene *scene, const char *filename);
void ta_scene_print             (ta_scene *scene, FILE *hnd);
void ta_scene_save_file_json    (ta_scene *scene, const char *filename);
void ta_scene_print_json        (ta_scene *scene, FILE *hnd);

// Resources
void *ta_scene_alloc            (ta_scene *scene, enum ta_res_type type, const char *name, size_t name_len);
void ta_scene_destroy           (ta_scene *scene, enum ta_res_type type, const char *name, size_t name_len);
void *ta_scene_find_at          (ta_scene *scene, enum ta_res_type type, u32 index);
void *ta_scene_find_try         (ta_scene *scene, enum ta_res_type type, const char *name, size_t name_len);
void *ta_scene_find             (ta_scene *scene, enum ta_res_type type, const char *name, size_t name_len);
void *ta_scene_find_or_default  (ta_scene *scene, enum ta_res_type type, const char *name, size_t name_len);

// Entities/components (just resources w/ special name and some extra error handling)
void *ta_scene_component_add    (ta_scene *scene, const char *entity, enum ta_res_type type, const char *name, size_t name_len);
void *ta_scene_component_try    (ta_scene *scene, const char *entity, enum ta_res_type type);
void *ta_scene_component        (ta_scene *scene, const char *entity, enum ta_res_type type);