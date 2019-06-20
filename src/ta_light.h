#pragma once
#include "ta_scene.h"

typedef enum {
    TA_LIGHT_AMBIENT     = 0,
    TA_LIGHT_DIRECTIONAL = 1,
    TA_LIGHT_POINT       = 2,
    TA_LIGHT_SPOT        = 3,
} ta_light_type;

typedef struct ta_ambient_light_s {
    bool unused;
} ta_ambient_light;

typedef struct ta_directional_light_s {
    ta_vec3 direction;
} ta_directional_light;

typedef struct ta_point_light_s {
    bool unused;
} ta_point_light;

typedef struct ta_spot_light_s {
    ta_vec3 direction;
    float theta_cone;
    float theta_falloff;
} ta_spot_light;

typedef struct ta_light_s {
    ta_scene_ref ref;
    bool disabled;
    float intensity;
    ta_vec3 position;
    ta_rgb color;
    ta_light_type type;
    union {
        ta_ambient_light ambient;
        ta_directional_light directional;
        ta_point_light point;
        ta_spot_light spot;
    } data;
} ta_light;

const char *ta_light_type_str(int type);
void ta_light_init(ta_light *light);