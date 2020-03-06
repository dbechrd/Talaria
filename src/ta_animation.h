#pragma once
#include "ta_schema.h"

typedef enum ta_animation_interpolation {
    TA_ANIM_INTERP_STEP,
    TA_ANIM_INTERP_LINEAR,
    TA_ANIM_INTERP_CUBIC,
} ta_animation_interpolation;

typedef struct ta_animation_sampler {
    void *input;    // TODO: How to store arbitrarily typed data (based on mesh_buffer_type?).. union?
    void *output;
    ta_animation_interpolation interpolation;
} ta_animation_sampler;

typedef struct ta_animation_target {
    const char *entity;
    enum ta_mesh_buffer_type attribute;
} ta_animation_target;

typedef struct ta_animation_channel {
    size_t sampler_idx;
    ta_animation_target target;
} ta_animation_channel;

typedef struct ta_animation {
    TA_RESOURCE_HEADER
    ta_animation_channel *channels;
} ta_animation;
