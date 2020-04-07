#pragma once
#include "ta_schema.h"
#include "ta_math.h"
#include "ta_texture.h"
#include "dlb/dlb_types.h"
#include "misc/glad.h"

struct ta_model;
struct ta_shader;

typedef enum ta_light_type {
    TA_LIGHT_AMBIENT,       // @Name("Ambient")
    TA_LIGHT_DIRECTIONAL,   // @Name("Directional")
    TA_LIGHT_POINT,         // @Name("Point")
    TA_LIGHT_SPOT,          // @Name("Spot")
    TA_LIGHT_COUNT
} ta_light_type;

typedef struct ta_light_ambient {
    int unused;
} ta_light_ambient;

typedef struct ta_light_directional {
    int unused;
} ta_light_directional;

typedef struct ta_light_point {
    int unused;
} ta_light_point;

typedef struct ta_light_spot {
    float theta_cone;     //
    float theta_falloff;  //
} ta_light_spot;

typedef struct ta_light_shadowmap {
    const char  *shader;      //
    u32         resolution;   //
    float       znear;        //
    float       zfar;         //
    ta_mat4     projection;   //
    GLuint      framebuffer;  //
    ta_texture  texture;      //
} ta_light_shadowmap;

typedef struct ta_light {
    TA_COMPONENT_HEADER
    float         intensity;      // Light intensity
    ta_rgb        color;          // Light color
    bool          disabled;       // If true, light will not illuminate or cast shadows
    bool          cast_shadows;   // If true, light will cast dynamic shadows
    ta_light_type type;           // Light type
    union {
        ta_light_ambient     ambient;      // Type-specific light properties
        ta_light_directional directional;
        ta_light_point       point;
        ta_light_spot        spot;
    } data;
    ta_light_shadowmap shadowmap;  // Shadow map properties
} ta_light;

const char *ta_light_type_str           (int type);
void ta_light_init                      (ta_light *light);
ta_vec3 ta_light_position               (ta_light *light);
ta_vec3 ta_light_direction              (ta_light *light);
ta_mat4 ta_light_pv                     (ta_light *light);
void ta_light_shadowpass_render         (ta_light *light, struct ta_transform *transforms);
void ta_light_render_shadowmap_debug    (ta_light *light, int x, int y);