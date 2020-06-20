#version 330 core

//------------------------------------------------------
// Vertex attributes
//------------------------------------------------------
layout(location =  0) in vec3 attr_position;
layout(location =  1) in vec4 attr_color;
layout(location =  2) in vec2 attr_uv;
layout(location =  3) in vec3 attr_normal;
layout(location =  4) in vec3 attr_tangent;
layout(location =  5) in vec3 attr_morph0_position;
layout(location =  6) in vec4 attr_morph0_color;
layout(location =  7) in vec2 attr_morph0_uv;
layout(location =  8) in vec3 attr_morph0_normal;
layout(location =  9) in vec3 attr_morph0_tangent;
layout(location = 10) in vec4 attr_joints;
layout(location = 11) in vec4 attr_weights;

//------------------------------------------------------
// Camera, model, animation
//------------------------------------------------------
// TODO: Uniform block(s) grouped by update frequency
uniform float u_morph_weights[1];
uniform mat4 u_proj;
uniform mat4 u_view;
uniform mat4 u_model;
uniform vec3 u_camera_pos;

//------------------------------------------------------
// Lights
//------------------------------------------------------
#define TA_LIGHTING_MAX_ACTIVE_LIGHTS 8

struct Light {
    float intensity;
    vec3 position;
    vec3 color;
    int type;
    bool cast_shadows;
    // Directional / Spot
    vec3 direction;
    sampler2D shadowmap2d;
    mat4 light_pv;
    // Point
    samplerCube shadowmap3d;
    float shadowmap_zfar;
};
//uniform int u_lights_count;
uniform Light u_lights[TA_LIGHTING_MAX_ACTIVE_LIGHTS];

struct LightNew {
    // Common
    int type;
    float intensity;
    bool cast_shadows;
    vec3 position;
    vec3 color;

    // Directional / Spot
    vec3 direction;
    mat4 light_pv;

    // Point
    float shadowmap_zfar;

    float shadowmap_texture_pool_index;  // NOTE: point light "cubemaps" will use 6 consecutive layers
    float shadowmap_texture_array_layer;
};
layout (std140) uniform u_lights_new_block {
    LightNew u_lights_new[TA_LIGHTING_MAX_ACTIVE_LIGHTS];
};
uniform int u_lights_count;

//------------------------------------------------------
// Textures
//------------------------------------------------------
// NOTE: Needs to match #defines in ta_texture.h
#define TA_TEXTURE_POOL_1      0
//#define TA_TEXTURE_POOL_2
//#define TA_TEXTURE_POOL_4
//#define TA_TEXTURE_POOL_8
//#define TA_TEXTURE_POOL_16
#define TA_TEXTURE_POOL_32     1
//#define TA_TEXTURE_POOL_64
//#define TA_TEXTURE_POOL_128
//#define TA_TEXTURE_POOL_256
#define TA_TEXTURE_POOL_512    2
#define TA_TEXTURE_POOL_1024   3
#define TA_TEXTURE_POOL_2048   4
#define TA_TEXTURE_POOL_4096   5
#define TA_TEXTURE_POOL_COUNT  6

uniform sampler2DArray u_textures[TA_TEXTURE_POOL_COUNT];
// TODO: Use sampler2DArray * 6, map direction into 2D somehow?
uniform samplerCube u_cubemaps[TA_LIGHTING_MAX_ACTIVE_LIGHTS];
//------------------------------------------------------

out vs_out {
    vec3 position;
    vec4 color;
	vec2 uv;
    vec3 normal;
    vec3 tangent;
    vec3 tbn_position;
	vec3 tbn_normal;
    vec3 tbn_camera_pos;
    vec3 tbn_light_pos[TA_LIGHTING_MAX_ACTIVE_LIGHTS];
    vec3 tbn_light_dir[TA_LIGHTING_MAX_ACTIVE_LIGHTS];
    vec4 light_pvm[TA_LIGHTING_MAX_ACTIVE_LIGHTS];
} vertex;

void main()
{
    // Morph target blending
#if 0
    // http://antongerdelan.net/opengl/blend_shapes.html
    // if other weights add up to less than 1, use neutral target
    float neutral_w = 1.0 - u_morph_weights[0];  // - u_morph_weights[1]
    clamp(neutral_w, 0.0, 1.0);

    // get a sum of weights and work out factors for each target
    float sum_w = neutral_w + u_morph_weights[0];
    float neutral_f = neutral_w / sum_w;
    float morph0_f = u_morph_weights[0] / sum_w;

    // interpolate targets to give us current pose
    vec3 morphed_pos =
        neutral_f * attr_position +
        morph0_f * attr_morph0_position;
#else
    // add weighted morph targets to give us current pose
    vec3 morphed_pos =
        attr_position +
        u_morph_weights[0] * attr_morph0_position;
#endif

    // TODO: Premultiply MVP matrix and pass as uniform
    vec4 position = u_model * vec4(morphed_pos, 1.0);
    vertex.position = vec3(position);
	vertex.color = attr_color;
	vertex.uv = attr_uv;
    vertex.normal = attr_normal;
    vertex.tangent = attr_tangent;

    // TODO: Calculate model inverse on CPU side
    mat3 normal_matrix = transpose(inverse(mat3(u_model)));
    vec3 T = normalize(normal_matrix * attr_tangent);
    vec3 N = normalize(normal_matrix * attr_normal);
    T = normalize(T - dot(T, N) * N);  // re-orthogonalize T with respect to N
    vec3 B = cross(N, T);
    mat3 TBN = transpose(mat3(T, B, N));

    vertex.tbn_normal = TBN * normalize(vec3(u_model * vec4(attr_normal, 0.0)));

    vertex.tbn_position = TBN * vertex.position;
    vertex.tbn_camera_pos = TBN * u_camera_pos;
#if 0
    for (int i = 0; i < u_lights_count; i++) {
        vertex.tbn_light_pos[i] = TBN * u_lights[i].position;
        vertex.tbn_light_dir[i] = TBN * u_lights[i].direction;

        // TODO: Why the fuck isn't this working? Causes linker error!?!?
        //vertex.light_pvm[i] = u_lights[i].light_pv * position;
    }

    // HACK: GLSL bullshit preventing this from happening in for loop
    vertex.light_pvm[0] = u_lights[0].light_pv * position;
    vertex.light_pvm[1] = u_lights[1].light_pv * position;
    vertex.light_pvm[2] = u_lights[2].light_pv * position;
    vertex.light_pvm[3] = u_lights[3].light_pv * position;
    vertex.light_pvm[4] = u_lights[4].light_pv * position;
    vertex.light_pvm[5] = u_lights[5].light_pv * position;
    vertex.light_pvm[6] = u_lights[6].light_pv * position;
    vertex.light_pvm[7] = u_lights[7].light_pv * position;
#else
    for (int i = 0; i < u_lights_count; i++) {
        vertex.tbn_light_pos[i] = TBN * u_lights_new[i].position;
        vertex.tbn_light_dir[i] = TBN * u_lights_new[i].direction;

        // TODO: Why the fuck isn't this working? Causes linker error!?!?
        //vertex.light_pvm[i] = u_lights_new[i].light_pv * position;
    }

    // HACK: GLSL bullshit preventing this from happening in for loop
    // https://community.khronos.org/t/samplers-inside-structs/58843/13 (loop unroll macro)
    vertex.light_pvm[0] = u_lights_new[0].light_pv * position;
    vertex.light_pvm[1] = u_lights_new[1].light_pv * position;
    vertex.light_pvm[2] = u_lights_new[2].light_pv * position;
    vertex.light_pvm[3] = u_lights_new[3].light_pv * position;
    vertex.light_pvm[4] = u_lights_new[4].light_pv * position;
    vertex.light_pvm[5] = u_lights_new[5].light_pv * position;
    vertex.light_pvm[6] = u_lights_new[6].light_pv * position;
    vertex.light_pvm[7] = u_lights_new[7].light_pv * position;
#endif
    gl_Position = u_proj * u_view * position;
}
