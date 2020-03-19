#pragma once
#include "ta_schema.h"
#include "ta_math.h"
#include "dlb/dlb_types.h"

typedef struct ta_piece {
    const char *mesh;
    const char *material;
    const char **anim_targets;  // Array of animation target meshes
} ta_piece;

typedef struct ta_model {
    TA_COMPONENT_HEADER
    ta_piece *pieces;          // Array of mesh pieces in this model
    const char **anim_targets; // Array of animation target names
    bool     invisible;        // If true, model is not rendered
    bool     cast_shadows;     // If true, allows model to cast real-time shadows
    bool     receive_shadows;  // If true, model will use shadow maps                      // TODO: Pass as flag to PBR shader, skip shadows if false
    //bool     no_serialize;     // If true, do not serialize this model to the scene file   // HACK: Exclude GLTF models from scene file
} ta_model;

struct ta_camera;
struct ta_shader;

void ta_model_free          (ta_model *model);
void ta_model_shadow_pass   (ta_model *model, struct ta_shader *shader, ta_mat4 *light_pv);
void ta_model_render        (ta_model *model, struct ta_camera *camera);
void ta_model_render_shader (ta_model *model, struct ta_camera *camera, struct ta_shader *shader);