#pragma once
#include "ta_scene.h"

typedef enum {
    TA_LIGHT_SUN   = 0,
    TA_LIGHT_POINT = 1,
} ta_light_type;

typedef struct ta_sun_light_s {
    ta_vec3 direction;
    ta_rgb color;
} ta_sun_light;

typedef struct ta_point_light_s {
    ta_vec3 position;
    ta_rgb color;
} ta_point_light;

typedef struct ta_light_s {
    ta_scene *scene;
    const char *uid;
    ta_light_type type;
    union {
        ta_sun_light sun;
        ta_point_light point;
    } data;
} ta_light;

const char *ta_light_type_str(int type);
void ta_light_init(ta_light *light);