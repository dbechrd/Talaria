#pragma once
#include <stdint.h>

typedef float ogx_vec2[2];
typedef float ogx_vec3[3];
typedef float ogx_mat4[16];

typedef struct ogx_transform {
    const char *type;
    ogx_mat4 data;
} ogx_transform;

typedef enum ogx_node_type {
    OGX_BASIC_NODE,
    OGX_BONE_NODE,
    OGX_LIGHT_NODE,
    OGX_CAMERA_NODE,
    OGX_GEOMETRY_NODE,
} ogx_node_type;

#define OGX_NODE_HEADER         \
    ogx_node_type type;         \
    const char *name;           \
    ogx_transform transform;    \
    union ogx_node *parent;     \
    union ogx_node *children;

typedef struct ogx_basic_node {
    OGX_NODE_HEADER
} ogx_basic_node;

typedef struct ogx_light_node {
    ogx_basic_node base;
    const char *light;
} ogx_light_node;

typedef struct ogx_camera_node {
    ogx_basic_node base;
    const char *camera;
} ogx_camera_node;

typedef struct ogx_geometry_node {
    ogx_basic_node base;
    const char *mesh;
    const char **materials;
} ogx_geometry_node;

#undef OGX_NODE_HEADER

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
    ogx_time time;
    ogx_value value;
} ogx_track;

typedef struct ogx_animation {
    ogx_track track;
} ogx_animation;

typedef struct ogx_bone_node {
    ogx_basic_node base;
    ogx_animation animation;
} ogx_bone_node;

typedef union ogx_node {
    ogx_basic_node basic_node;
    ogx_bone_node bone_node;
    ogx_light_node light_node;
    ogx_camera_node camera_node;
    ogx_geometry_node geometry_node;
} ogx_node;

typedef struct ogx_camera {
    const char *name;
    float fov;
    float nearz;
    float farz;
} ogx_camera;

typedef enum ogx_vertex_attrib {
    OGX_VERTEX_ATTRIB_UNKNOWN,
    OGX_VERTEX_ATTRIB_POSIITON,
    OGX_VERTEX_ATTRIB_NORMAL,
    OGX_VERTEX_ATTRIB_TEXCOORD0,
    OGX_VERTEX_ATTRIB_COUNT,
} ogx_vertex_attrib;

typedef struct ogx_vertex_array {
    ogx_vertex_attrib attrib;
    union {
        float *as_float;
        ogx_vec2 *as_vec2;
        ogx_vec3 *as_vec3;
    } values;
} ogx_vertex_array;

// NOTE: always GL_TRIANGLES (to allow other types, we would need a "kind" field)
typedef struct ogx_index_array {
    const char *material;
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
    ogx_mat4 *bind_poses;
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
    ogx_vertex_array *vertex_arrays;
    ogx_index_array *index_arrays;
    ogx_skin skin;
} ogx_mesh;

typedef struct ogx_geometry {
    const char *name;
    ogx_mesh mesh;
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

typedef struct ogx_material {
    const char *name;
    ogx_vec3 albedo_factor;
    ogx_vec3 normal_factor;
    float roughness_factor;
} ogx_material;

typedef struct ogx_scene {
    ogx_node *nodes;
    ogx_camera *cameras;
    ogx_geometry *geometry;
    ogx_light *lights;
    ogx_material *materials;
} ogx_scene;

typedef enum ogx_result {
    OGX_SUCCESS,
    OGX_FILE_INVALID,
    OGX_SYNTAX_ERROR,
    OGX_EMPTY_DOCUMENT,
    OGX_UNEXPECTED_VALUE,
    OGX_UNEXPECTED_TYPE,
    OGX_UNEXPECTED_FIELD,
    OGX_EXPECTED_LITERAL,
    OGX_EXPECTED_BOOL,
    OGX_EXPECTED_STRING,
    OGX_EXPECTED_FLOAT,
    OGX_EXPECTED_ARRAY,
    OGX_EXPECTED_OBJECT,
    OGX_INVALID_ARRAY_LENGTH,
    OGX_UNKNOWN_TYPE,
    OGX_NOT_IMPLEMENTED,

    OGX_RESULT_COUNT
} ogx_result;

ogx_result dml_load(const char *filename);