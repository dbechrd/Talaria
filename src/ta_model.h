#pragma once
#include "ta_uid.h"
#include "ta_math.h"
#include "dlb/dlb_types.h"

typedef struct ta_model {
    u32 index;
    const char *name;
    const char *entity_name;
    // TODO: Do I need mesh groups, or should i just have multiple mesh components?
    const char **meshes;
    // TODO: If I need multiple materials per mesh group, probably easier to just
    //       split into multiple mesh components, with mesh/material pairs.
    const char *material;
    bool invisible;
    bool cast_shadows;
    bool receive_shadows;  // TODO: Pass as flag to PBR shader, skip shadows if false
} ta_model;

struct ta_camera;
struct ta_shader;

void ta_model_shadow_pass(ta_model *model, struct ta_shader *shader,
    ta_mat4 *light_pv);
void ta_model_render(ta_model *model, struct ta_camera *camera);
void ta_model_render_shader(ta_model *model, struct ta_camera *camera,
    struct ta_shader *shader);