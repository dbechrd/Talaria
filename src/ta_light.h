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
    ta_vec3 direction;
} ta_light_directional;

typedef struct ta_light_point {
    float intensity;
    ta_rgb color;
} ta_light_point;

typedef struct ta_light_spot {
    float intensity;
    ta_rgb color;
    ta_vec3 direction;
    float theta_cone;
    float theta_falloff;
} ta_light_spot;

typedef struct ta_light_shadowmap {
    GLuint framebuffer;
    ta_texture texture;
    //u32 depthbuffer;
    GLsizei resolution;
    float znear;
    float zfar;
    ta_mat4 projection;
} ta_light_shadowmap;

typedef struct ta_light {
    u32 index;
    const char *name;
    const char *entity_name;
    ta_vec3 position;  // TODO: COMP_POSITION? Different from light position?
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
void ta_light_shadowpass_render(ta_light *light, struct ta_shader *shader,
    float alpha, struct ta_model *models);
void ta_light_render_shadowmap_debug(ta_light *light, int x, int y);