#pragma once
#include "dlb/dlb_types.h"

typedef float ogx_vec2[2];  // xy
typedef float ogx_vec3[3];  // xyz
typedef float ogx_vec4[4];  // xyzw
typedef float ogx_mat4[16]; // 00, 01, 02, 03, 10, ..., 33

typedef struct ogx_transform {
    ogx_vec3 position;
    ogx_vec4 orientation;
} ogx_transform;

typedef enum ogx_key_kind {
    OGX_KEY_KIND_UNKNOWN,
    OGX_KEY_KIND_VALUE,
    // NOTE: "+control" and "-control" only valid for time/value with curve = "bezier"
    OGX_KEY_KIND_POS_CONTROL,
    OGX_KEY_KIND_NEG_CONTROL,
    OGX_KEY_KIND_COUNT
} ogx_key_kind;

typedef enum ogx_type {
    OGX_TYPE_UNKNOWN,
    OGX_TYPE_FLOAT,
    OGX_TYPE_VEC2,
    OGX_TYPE_VEC3,
    OGX_TYPE_MAT4,
    OGX_TYPE_COUNT
} ogx_type;

typedef struct ogx_key {
    ogx_key_kind kind;
    ogx_type type;
    union {
        float *as_float;
        ogx_mat4* as_mat4;
    } values;
} ogx_key;

typedef enum ogx_time_curve {
    OGX_TIME_CURVE_UNKNOWN,
    OGX_TIME_CURVE_LINEAR,
    OGX_TIME_CURVE_BEZIER,
    OGX_TIME_CURVE_COUNT
} ogx_time_curve;

typedef struct ogx_time {
    ogx_time_curve curve;
    ogx_key key;
} ogx_time;

typedef enum ogx_value_curve {
    OGX_VALUE_CURVE_UNKNOWN,
    //OGX_VALUE_CURVE_CONSTANT, // Not used by Blender exporter
    OGX_VALUE_CURVE_LINEAR,
    OGX_VALUE_CURVE_BEZIER,
    //OGX_VALUE_CURVE_TCB,      // Not used by Blender exporter
    OGX_VALUE_CURVE_COUNT
} ogx_value_curve;

typedef struct ogx_value {
    ogx_time_curve curve;
    ogx_key key;
} ogx_value;

typedef struct ogx_track {
    const char *target;
    float morph_weight_idx;  // TODO: uint32 (morph weight target index, only applicable when target = "morph_weight")
    ogx_time time;
    ogx_value value;
} ogx_track;

typedef struct ogx_animation {
    float begin;
    float end;
    ogx_track track;
} ogx_animation;

typedef enum ogx_node_type {
    OGX_BASIC_NODE,
    OGX_BONE_NODE,
    OGX_CAMERA_NODE,
    OGX_GEOMETRY_NODE,
    OGX_LIGHT_NODE,
} ogx_node_type;

#if 0
typedef struct ogx_basic_node {
    ogx_node_type type;
    const char *name;
    ogx_transform transform;
    ogx_animation *animations;
    union ogx_node *parent;
    union ogx_node *children;
} ogx_basic_node;

typedef struct ogx_bone_node {
    ogx_basic_node base;
} ogx_bone_node;

typedef struct ogx_camera_node {
    ogx_basic_node base;
    const char *camera;
} ogx_camera_node;

// TODO: This will become a ta_model
typedef struct ogx_geometry_node {
    ogx_basic_node base;
    const char *mesh;
    const char **materials;
    float *morph_weights;
    ogx_animation *animations;
} ogx_geometry_node;

typedef struct ogx_light_node {
    ogx_basic_node base;
    const char *light;
} ogx_light_node;

typedef union ogx_node {
    ogx_basic_node basic_node;
    ogx_bone_node bone_node;
    ogx_camera_node camera_node;
    ogx_geometry_node geometry_node;
    ogx_light_node light_node;
} ogx_node;
#else
typedef struct ogx_bone_node {
    int unused;
} ogx_bone_node;

typedef struct ogx_camera_node {
    const char *camera;
} ogx_camera_node;

// TODO: This will become a ta_model
typedef struct ogx_geometry_node {
    const char *mesh;
    const char **materials;
    float *morph_weights;
} ogx_geometry_node;

typedef struct ogx_light_node {
    const char *light;
} ogx_light_node;

typedef struct ogx_node {
    ogx_node_type type;
    const char *name;
    ogx_transform transform;
    ogx_animation *animations;
    struct ogx_node *parent;    // TODO: Use index (or name), not pointer
    struct ogx_node *children;  // TODO: Use index (or name), not pointer
    union {
        ogx_bone_node bone;
        ogx_camera_node camera;
        ogx_geometry_node geometry;
        ogx_light_node light;
    } properties;
} ogx_node;
#endif

typedef struct ogx_camera {
    const char *name;
    float fov;
    float nearz;
    float farz;
} ogx_camera;

typedef enum ogx_vertex_attrib {
    OGX_VERTEX_ATTRIB_UNKNOWN,
    OGX_VERTEX_ATTRIB_POSIITON,   // vec3
    OGX_VERTEX_ATTRIB_NORMAL,     // vec3
    OGX_VERTEX_ATTRIB_TANGENT,    // vec3
    OGX_VERTEX_ATTRIB_TEXCOORD0,  // vec2
    OGX_VERTEX_ATTRIB_COUNT,
} ogx_vertex_attrib;

// NOTE: The perfectly representable integer range of a float is [0 - 16,777,216]
// (2^24 because a float has 24 bits of mantissa)
typedef struct ogx_vertex_array {
    ogx_vertex_attrib attrib;
    float morph;  // TODO: uint32 (why 32.. there should only be a few morphs?)
    union {
        float *as_float;
        ogx_vec2 *as_vec2;
        ogx_vec3 *as_vec3;
    } values;
} ogx_vertex_array;

// NOTE: always GL_TRIANGLES (to allow other types, we would need a "kind" field)
typedef struct ogx_index_array {
    float material_slot;  // TODO: uint16
    union {
        // TODO: Use u16 for these
        //uint16_t as_u16;
        float *as_float;
        //ogx_vec2 *as_vec2;
        //ogx_vec3 *as_vec3;
    } values;
} ogx_index_array;

typedef struct ogx_skeleton {
    const char **bones;
    ogx_vec3 *bind_pose_positions;
    ogx_vec4 *bind_pose_orientations;
} ogx_skeleton;

typedef struct ogx_skin {
    ogx_transform transform;
    ogx_skeleton skeleton;
    // TODO: Use u16 for these
    //uint16_t *bone_count_array;
    //uint16_t *bone_index_array;
    float *bone_count_array;
    float *bone_index_array;
    float *bone_weight_array;
} ogx_skin;

typedef struct ogx_mesh {
    ogx_vertex_array *vertex_arrays;  // +
    ogx_index_array *index_arrays;    // *  NOTE: If mesh has no index arrays, use first material
    ogx_skin skin;                    // ?
} ogx_mesh;

typedef struct ogx_morph {
    const char *name;
    //float index;  // TODO: What's this for?
    float base;   // TODO: u32
} ogx_morph;

typedef struct ogx_geometry {
    const char *name;
    ogx_morph *morphs;  // *
    ogx_mesh *meshes;   // +
} ogx_geometry;

typedef enum ogx_light_type {
    OGX_LIGHT_TYPE_POINT,
} ogx_light_type;

typedef enum ogx_light_atten_kind {
    OGX_LIGHT_ATTEN_KIND_DISTANCE,
} ogx_light_atten_kind;

typedef enum ogx_light_atten_curve {
    OGX_LIGHT_ATTEN_CURVE_INVERSE_SQUARE,
} ogx_light_atten_curve;

typedef struct ogx_light_atten {
    ogx_light_atten_kind kind;
    ogx_light_atten_curve curve;
    float scale;
} ogx_light_atten;

typedef struct ogx_light {
    const char *name;
    ogx_light_type type;
    bool shadow;
    ogx_vec3 color;
    float intensity;
    ogx_light_atten *attens;
} ogx_light;

typedef struct ogx_texture {
    const char *name;
    const char *path;
} ogx_texture;

typedef struct ogx_material {
    const char *name;
    float       alpha_factor;
    const char *alpha_texture;
    ogx_vec3    albedo_factor;
    const char *albedo_texture;
    ogx_vec3    emissive_factor;
    const char *emissive_texture;
    float       metallic_factor;
    const char *metallic_texture;
    ogx_vec3    normal_factor;
    const char *normal_texture;
    float       roughness_factor;
    const char *roughness_texture;
} ogx_material;

typedef struct ogx_scene {
    ogx_node *nodes;
    ogx_camera *cameras;
    ogx_geometry *geometry;
    ogx_light *lights;
    ogx_material *materials;
    ogx_texture *textures;
} ogx_scene;

const char *ogx_key_kind_str[OGX_KEY_KIND_COUNT];
const char *ogx_type_str[OGX_TYPE_COUNT];
const char *ogx_time_curve_str[OGX_TIME_CURVE_COUNT];
const char *ogx_value_curve_str[OGX_VALUE_CURVE_COUNT];

void ta_ogx_load(ogx_scene *scene);