#pragma once
#include "ta_schema.h"

typedef enum ta_animation_track_key_kind {
    TA_ANIMATION_TRACK_KEY_UNKNOWN,
    TA_ANIMATION_TRACK_KEY_VALUE,
    // NOTE: "+control" and "-control" only valid for time/value with curve = "bezier"
    TA_ANIMATION_TRACK_KEY_POS_CONTROL,
    TA_ANIMATION_TRACK_KEY_NEG_CONTROL,
    TA_ANIMATION_TRACK_KEY_COUNT
} ta_animation_track_key_kind;

extern const char *ta_animation_track_key_kind_str[TA_ANIMATION_TRACK_KEY_COUNT];

typedef enum ta_animation_track_curve_type {
    TA_ANIMATION_TRACK_CURVE_UNKNOWN,
    //TA_ANIMATION_TRACK_CURVE_CONSTANT, // Not used by Blender exporter (only values, not time)
    TA_ANIMATION_TRACK_CURVE_LINEAR,
    TA_ANIMATION_TRACK_CURVE_BEZIER,
    //TA_ANIMATION_TRACK_CURVE_TCB,      // Not used by Blender exporter (only values, not time)
    TA_ANIMATION_TRACK_CURVE_COUNT
} ta_animation_track_curve_type;

extern const char *ta_animation_track_curve_type_str[TA_ANIMATION_TRACK_CURVE_COUNT];

typedef struct ta_animation_track_key {
    ta_animation_track_key_kind kind;
    ta_schema_field_type type;
    union {
        float *as_float;
        ta_vec2* as_vec2;
        ta_vec3* as_vec3;
        ta_vec4* as_vec4;
        ta_mat4* as_mat4;
    } values;
} ta_animation_track_key;

// NOTE: Could consolidate *track_time and *track_value, or hard-code time to always be as_float
typedef struct ta_animation_track_time {
    ta_animation_track_curve_type curve;
    ta_animation_track_key key;
} ta_animation_track_time;

typedef struct ta_animation_track_value {
    ta_animation_track_curve_type curve;
    ta_animation_track_key key;
} ta_animation_track_value;

typedef struct ta_animation_track {
    const char *target_node;
    const char *target_path; // TODO: This should probably be an enum? ("transform", "rotation", etc.)
    float morph_weight_idx;  // TODO: uint32 (morph weight target index, only applicable when target = "morph_weight")
    ta_animation_track_time time;
    ta_animation_track_value value;
} ta_animation_track;

typedef struct ta_animation {
    TA_RESOURCE_HEADER
    float begin;
    float end;
    ta_animation_track *tracks;
} ta_animation;

#if 0  // GLTF Animation data
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
void ta_animation_free      (ta_animation *animation);
void ta_animation_free_void (void *animation);
#endif