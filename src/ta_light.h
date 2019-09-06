#pragma once
#include "ta_uid.h"
#include "ta_math.h"
#include "dlb/dlb_types.h"

struct ta_node;
struct ta_shader;

typedef enum ta_light_type {
    TA_LIGHT_AMBIENT,
    TA_LIGHT_DIRECTIONAL,
    TA_LIGHT_POINT,
    TA_LIGHT_SPOT,
    TA_LIGHT_COUNT
} ta_light_type;

typedef struct ta_light_ambient {
    bool unused;
} ta_light_ambient;

typedef struct ta_light_directional {
    ta_vec3 direction;
} ta_light_directional;

typedef struct ta_light_point {
    bool unused;
} ta_light_point;

typedef struct ta_light_spot {
    ta_vec3 direction;
    float theta_cone;
    float theta_falloff;
} ta_light_spot;

typedef struct ta_light_shadowmap {
    u32 framebuffer;
    u32 texture;
    u32 depthbuffer;
    s32 resolution;
    float znear;
    float zfar;
    ta_mat4 projection;
} ta_light_shadowmap;

typedef struct ta_light {
    ta_uid uid;
    bool disabled;
    float intensity;
    ta_vec3 position;
    ta_rgb color;
    ta_light_type type;
    union {
        ta_light_ambient ambient;
        ta_light_directional directional;
        ta_light_point point;
        ta_light_spot spot;
    } data;
    bool cast_shadows;
    ta_light_shadowmap shadowmap;
} ta_light;

const char *ta_light_type_str(int type);
void ta_light_init(ta_light *light);
void ta_light_shadowpass_render(ta_light *light, struct ta_shader *shader,
    float alpha, struct ta_node *entities);
void ta_light_render_shadowmap_debug(ta_light *light);