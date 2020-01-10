#pragma once
#include "ta_math.h"
#include "ta_texture.h"
#include "dlb/dlb_types.h"
#include "misc/gl3w.h"

struct ta_model;
struct ta_shader;

typedef enum ta_light_type {
    TA_LIGHT_AMBIENT,
    TA_LIGHT_DIRECTIONAL,
    TA_LIGHT_POINT,
    TA_LIGHT_SPOT,
    TA_LIGHT_COUNT
} ta_light_type;

typedef struct ta_light_ambient {
    float intensity;
    ta_rgb color;
} ta_light_ambient;

typedef struct ta_light_directional {
    float intensity;
    ta_rgb color;
} ta_light_directional;

typedef struct ta_light_point {
    float intensity;
    ta_rgb color;
} ta_light_point;

typedef struct ta_light_spot {
    float intensity;
    ta_rgb color;
    float theta_cone;
    float theta_falloff;
} ta_light_spot;

typedef struct ta_light_shadowmap {
    const char *shader;
    GLsizei resolution;
    float znear;
    float zfar;

    ta_mat4 projection;
    GLuint framebuffer;
    ta_texture texture;
} ta_light_shadowmap;

typedef struct ta_light {
    u32 index;
    const char *name;
    const char *entity_name;
    bool disabled;
    bool cast_shadows;
    ta_light_shadowmap shadowmap;
    ta_light_type type;
    union {
        struct {
            float intensity;
            ta_rgb color;
        } common;
        ta_light_ambient ambient;
        ta_light_directional directional;
        ta_light_point point;
        ta_light_spot spot;
    } data;
} ta_light;

const char *ta_light_type_str(int type);
void ta_light_init(ta_light *light);
ta_vec3 ta_light_position(ta_light *light);
ta_vec3 ta_light_direction(ta_light *light);
ta_mat4 ta_light_pv(ta_light *light);
void ta_light_shadowpass_render(ta_light *light, struct ta_model *models);
void ta_light_render_shadowmap_debug(ta_light *light, int x, int y);