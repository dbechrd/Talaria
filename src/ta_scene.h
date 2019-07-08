#pragma once
#include "ta_schema.h"
#include "ta_file.h"
#include "ta_math.h"
#include "dlb_types.h"
#include "dlb_hash.h"

typedef struct ta_camera_s ta_camera;
typedef struct ta_scene_s ta_scene;
typedef struct ta_shader_s ta_shader;

typedef struct ta_uid_s {
    const char *uid;
    ta_scene *scene;
} ta_uid;

typedef struct ta_scene_s {
    const char *name;

    const char *default_material_uid;
    const char *default_texture_uid;
    const char *default_mesh_group_uid;

    void *pools[TA_COUNT_POOLS];
    dlb_hash pooled_uids[TA_COUNT_POOLS];
} ta_scene;

ta_scene *ta_scene_init(const char *name);
ta_scene *ta_scene_load(ta_file *f);
void ta_scene_free(ta_scene *scn);
void ta_scene_print(ta_scene *scn, FILE *hnd);
void *ta_scene_alloc(ta_scene *scene, ta_schema_field_type type, const char *uid);
void *ta_scene_find(ta_scene *scene, ta_schema_field_type type, const char *uid);
void ta_scene_update(ta_scene *scene, float dt);
void ta_scene_shadow_pass(ta_scene *scene, ta_shader *shader, float alpha);
void ta_scene_render(ta_scene *scene, ta_camera *camera, float alpha);