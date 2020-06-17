#pragma once
#include "ta_schema.h"
#include "ta_math.h"

typedef struct ta_piece {
    const char *mesh;
    const char *material;
    const char **anim_targets;  // Array of animation target meshes
} ta_piece;

typedef struct ta_model {
    TA_COMPONENT_HEADER
    ta_piece   *pieces;               // Array of mesh pieces in this model
    const char *mesh;

    const char **materials;           // Array of material names
    const char **anim_targets;        // Array of animation target names
    float      *anim_target_weights;  // Array of animation target weights
    bool       invisible;             // If true, model is not rendered
    bool       cast_shadows;          // If true, allows model to cast real-time shadows
    bool       receive_shadows;       // If true, model will use shadow maps                      // TODO: Pass as flag to PBR shader, skip shadows if false
    //bool       no_serialize;          // If true, do not serialize this model to the scene file   // HACK: Exclude GLTF models from scene file
} ta_model;

struct ta_camera;
struct ta_shader;

void ta_model_free                      (ta_model *model);
float ta_model_get_morph_target_weight  (ta_model *model, const char *morph_target_name);
void ta_model_set_morph_target_weight   (ta_model *model, const char *morph_target_name, float weight);
void ta_model_shadow_pass               (ta_model *model, struct ta_shader *shader, ta_mat4 *light_pv);
void ta_model_render                    (ta_model *model, struct ta_camera *camera);
void ta_model_render_shader             (ta_model *model, struct ta_camera *camera, struct ta_shader *shader);