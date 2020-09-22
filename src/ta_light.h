#pragma once
#include "ta_schema.h"
#include "ta_math.h"
#include "ta_cubemap.h"
#include "dlb/dlb_types.h"
#include "misc/glad.h"

// NOTE: Has to match shader definition
#define TA_LIGHTING_MAX_ACTIVE_LIGHTS 4

struct ta_model;
struct ta_shader;
struct ta_transform;

// NOTE: This must match the GLSL ubo_lights structure byte-for-byte (including padding!)
// https://learnopengl.com/Advanced-OpenGL/Advanced-GLSL
// NOTE: This could obviously be packed tighter, but there are only 8 dynamic lights so it's not a priority
typedef struct ta_lighting_record {
    int type;                                 // ta_light_type: type of light
    float intensity;                          // light intensity [0.0, +INF]
    bool cast_shadows;                        // bool: light casts dynamic shadows if true
    float ___pad0;

    ta_vec3 position;   float ___pad1;        // light position in world space (note: for directional lights, position determines where the entity is renderered in the editor
    ta_vec3 color;      float ___pad2;        // RGB light color ([0.0, 1.0], [0.0, 1.0], [0.0, 1.0])
    ta_vec3 direction;  float ___pad3;        // light direction in world space (note: ignored for point lights)
    ta_mat4 light_pv;                         // light projection-view matrix

    float shadowmap_zfar;                     // z-far perspective divide for point light shadow maps (ignored for all other light types)
    u32 shadowmap_texture_pool_index;         // texture pool index where shadowmap is stored (note: pools are grouped by texture size)
    float ___pad4;
    float ___pad5;

    // NOTE: A LOT OF WASTED MEMORY, ME CRIES HARD. :'(
    // https://learnopengl.com/Advanced-OpenGL/Advanced-GLSL
    // "Array of scalars or vectors: Each element has a base alignment equal to that of a vec4."
    u32 shadowmap_texture_array_layer[6][4];  // array texture layer (determines which texture in the pool to use, where "pool" is an array texture)
} ta_lighting_record;

typedef struct ta_lighting {
    ta_lighting_record light_records[TA_LIGHTING_MAX_ACTIVE_LIGHTS];
    GLuint gl_ubo_lights;

    // TODO(cleanup): We probably don't need shadowmap UBOs if we're using the texture pool info in ta_lighting_record?
    //u32 shadowmap2d[TA_LIGHTING_MAX_ACTIVE_LIGHTS];  // NOTE: sampler2D
    //u32 shadowmap3d[TA_LIGHTING_MAX_ACTIVE_LIGHTS];  // NOTE: samplerCube
    //GLuint gl_ubo_shadowmap2d;
    //GLuint gl_ubo_shadowmap3d;
} ta_lighting;

typedef enum ta_light_type {
    TA_LIGHT_AMBIENT      = 0,  // @Name("Ambient")
    TA_LIGHT_DIRECTIONAL  = 1,  // @Name("Directional")
    TA_LIGHT_POINT        = 2,  // @Name("Point")
    TA_LIGHT_SPOT         = 3,  // @Name("Spot")
    TA_LIGHT_COUNT
} ta_light_type;

typedef struct ta_light_ambient {
    int unused;
} ta_light_ambient;

typedef struct ta_shadow_map_properties {
    const char  *shader;      //
    u32         resolution;   //
    float       znear;        //
    float       zfar;         //
    ta_mat4     projection;   //
} ta_shadow_map_properties;

typedef struct ta_light_directional {
    bool                     cast_shadows;
    ta_shadow_map_properties shadow_properties;
    const char               *shadow_map;
    GLuint                   framebuffer;       // shadowmap framebuffer
} ta_light_directional;

typedef struct ta_light_point {
    bool                     cast_shadows;
    ta_shadow_map_properties shadow_properties;
    ta_cubemap               shadow_map;
    GLuint                   framebuffer;       // shadowmap framebuffer
} ta_light_point;

typedef struct ta_light_spot {
    float                    theta_cone;
    float                    theta_falloff;
    bool                     cast_shadows;
    ta_shadow_map_properties shadow_properties;
    const char               *shadow_map;
    GLuint                   framebuffer;       // shadowmap framebuffer
} ta_light_spot;

typedef struct ta_light {
    TA_COMPONENT_HEADER
    float         intensity;
    ta_rgb        color;
    bool          enabled;
    ta_light_type type;
    union {
        ta_light_ambient     ambient;
        ta_light_directional directional;
        ta_light_point       point;
        ta_light_spot        spot;
    } data;
} ta_light;

void ta_lighting_init                   (ta_lighting *lighting);
void ta_lighting_bind_lights            (ta_lighting *lighting);

const char *ta_light_type_str           (int type);
void ta_light_init                      (ta_light *light);
void ta_light_init_void                 (void *light);
ta_vec3 ta_light_position               (ta_light *light);
ta_vec3 ta_light_direction              (ta_light *light);
ta_mat4 ta_light_pv                     (ta_light *light);
void ta_light_shadowpass_render         (ta_light *light, struct ta_model *models);
void ta_light_render_shadowmap_debug    (ta_light *light, int x, int y);