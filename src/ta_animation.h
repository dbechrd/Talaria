#pragma once
#include "ta_schema.h"

typedef enum ta_animation_interpolation_type {
    TA_ANIMATION_INTERP_STEP,
    TA_ANIMATION_INTERP_LINEAR,
    TA_ANIMATION_INTERP_CUBICSPLINE,
} ta_animation_interpolation_type;

typedef enum ta_animation_path_type {
    TA_ANIMATION_PATH_TRANSLATION,
    TA_ANIMATION_PATH_ROTATION,
    TA_ANIMATION_PATH_SCALE,
    TA_ANIMATION_PATH_WEIGHTS,
} ta_animation_path_type;

typedef struct ta_animation_sampler {
    float *input;  // time in seconds
#if 0
    ta_animation_path_type target_path;
    union {
        ta_vec3 translation;
        ta_vec4 rotation;
        ta_vec3 scale;
        float *weights;
    } output;
#else
    float *output;  // NOTE: Could be float, vector or matrix (depending on consuming channel's target_path)
#endif
    ta_animation_interpolation_type interpolation_mode;
} ta_animation_sampler;

typedef struct ta_animation_channel {
    size_t sampler_idx;
    // if path is trans/rot/scale, target is RES_COMP_TRANSFORM
    // if path is weights, target is RES_COMP_MODEL
    const char *target_bone;
    ta_animation_path_type target_path;
} ta_animation_channel;

typedef struct ta_animation {
    TA_RESOURCE_HEADER
    ta_animation_sampler *samplers;
    ta_animation_channel *channels;
} ta_animation;

const char *ta_animation_path_type_str(int type);
void ta_animation_free(ta_animation *animation);
