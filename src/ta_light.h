#pragma once
#include "ta_scene.h"

typedef struct ta_sun_light_s {
    ta_scene *scene;
    const char *uid;
    ta_vec3 direction;
    ta_vec3 color;
} ta_sun_light;

typedef struct ta_point_light_s {
    ta_scene *scene;
    const char *uid;
    ta_vec3 position;
    ta_vec3 color;
} ta_point_light;