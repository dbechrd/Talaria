#pragma once
#include "ta_scene.h"

typedef enum {
    TA_LIGHT_AMBIENT,
    TA_LIGHT_DIRECTIONAL,
    TA_LIGHT_POINT,
    TA_LIGHT_SPOT,
    TA_LIGHT_COUNT
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

typedef struct ta_shadowmap_s {
    u32 framebuffer;
    u32 texture;
    u32 depthbuffer;
    s32 resolution;
    float nearz;
    float farz;
    ta_mat4 projection;
} ta_shadowmap;

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
    bool cast_shadows;
    ta_shadowmap shadowmap;
} ta_light;

typedef struct ta_entity_s ta_entity;

const char *ta_light_type_str(int type);
void ta_light_init(ta_light *light);
void ta_light_shadowpass_render(ta_light *light, ta_shader *shader,
    float alpha, ta_entity *entities);