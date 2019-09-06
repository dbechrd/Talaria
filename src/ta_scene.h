#pragma once
#include "ta_schema.h"
#include "dlb/dlb_hash.h"

struct ta_shader;
struct ta_file;
struct ta_camera;

typedef struct ta_scene {
    const char *filename;
    const char *name;

    const char *default_material_uid;
    const char *default_texture_uid;
    const char *default_mesh_group_uid;

    void *pools[TYP_COUNT_POOLS];
    dlb_hash pooled_uids[TYP_COUNT_POOLS];
} ta_scene;

void ta_scene_init(ta_scene *scene);
ta_scene *ta_scene_load(struct ta_file *file);
ta_scene *ta_scene_load_file(const char *filename);
void ta_scene_save_file(ta_scene *scene, const char *filename);
void ta_scene_free(ta_scene *scn);
void ta_scene_print(ta_scene *scn, FILE *hnd);
void *ta_scene_alloc(ta_scene *scene, ta_schema_field_type type, const char *uid);
void *ta_scene_exists(ta_scene *scene, ta_schema_field_type type, const char *uid,
    bool *exists);
void *ta_scene_find(ta_scene *scene, ta_schema_field_type type, const char *uid);
void *ta_scene_find_by_index(ta_scene *scene, ta_schema_field_type type, size_t idx);
void ta_scene_update(ta_scene *scene, float dt);
void ta_scene_shadow_pass(ta_scene *scene, struct ta_shader *shader, float alpha);
void ta_scene_render(ta_scene *scene, struct ta_camera *camera, float alpha);