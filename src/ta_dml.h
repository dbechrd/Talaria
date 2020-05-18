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
    OGX_LIGHT_NODE,
    OGX_CAMERA_NODE,
    OGX_GEOMETRY_NODE,
} ogx_node_type;

#define OGX_NODE_HEADER         \
    ogx_node_type type;         \
    const char *name;           \
    ogx_transform transform;    \
    union ogx_node *parent;     \
    union ogx_node **children;

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

typedef union ogx_node {
    ogx_basic_node basic_node;
    ogx_light_node light_node;
    ogx_camera_node camera_node;
    ogx_geometry_node geometry_node;
} ogx_node;

typedef enum ogx_key_type {
    OGX_KEY_FLOAT,
    OGX_KEY_MAT4,
} ogx_key_type;

typedef struct ogx_key {
    ogx_key_type type;
    union {
        float *as_float;
        ogx_mat4 *as_mat4;
    } data;
} ogx_key;

typedef struct ogx_time {
    ogx_key key;
} ogx_time;

typedef struct ogx_value {
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
    ogx_node base;
    ogx_animation animation;
} ogx_bone_node;

typedef struct ogx_vertex_array {
    const char *attrib;
    union {
        ogx_vec2 *as_vec2;
        ogx_vec3 *as_vec3;
    } data;
} ogx_vertex_array;

typedef struct ogx_index_array {
    const char *attrib;
    int *data;
} ogx_index_array;

typedef struct ogx_skeleton {
    const char *bones;
    ogx_mat4 *bind_poses;
} ogx_skeleton;

typedef struct ogx_skin {
    ogx_transform transform;
    ogx_skeleton skeleton;
    uint16_t *bone_counts_array;
    uint16_t *bone_index_array;
    float *bone_weight_array;
} ogx_skin;

typedef struct ogx_mesh {
    ogx_vertex_array *vertex_arrays;
    ogx_index_array index_array;
    ogx_skin skin;
} ogx_mesh;

typedef struct ogx_geometry {
    const char *name;
    ogx_mesh mesh;
} ogx_geometry;

typedef enum ogx_light_type {
    OGX_LIGHT_POINT,
} ogx_light_type;

typedef enum ogx_atten_type {
    OGX_ATTEN_DISTANCE,
} ogx_atten_type;

typedef enum ogx_curve {
    OGX_CURVE_INVERSE_SQUARE,
} ogx_curve;

typedef struct ogx_atten {
    ogx_atten_type type;
    ogx_curve curve;
    float scale;
} ogx_atten;

typedef struct ogx_light {
    const char *name;
    ogx_light_type type;
    int8_t shadow;  // bool
    ogx_vec3 color;
    float intensity;
    ogx_atten atten;
} ogx_light;

typedef struct ogx_camera {
    const char *name;
    float fov;
    float nearz;
    float farz;
} ogx_camera;

typedef struct ogx_scene {
    ogx_node *nodes;
    ogx_light *lights;
    ogx_camera *cameras;
} ogx_scene;

typedef enum dml_result {
    OGX_SUCCESS,
    OGX_FILE_INVALID,
    OGX_SYNTAX_ERROR,
    OGX_EMPTY_DOCUMENT,
    OGX_UNEXPECTED_FIELD,
    OGX_EXPECTED_LITERAL,
    OGX_EXPECTED_STRING,
    OGX_EXPECTED_FLOAT,
    OGX_EXPECTED_ARRAY,
    OGX_EXPECTED_OBJECT,
    OGX_INVALID_ARRAY_LENGTH,
    OGX_NOT_IMPLEMENTED,

    OGX_RESULT_COUNT
} dml_result;

dml_result dml_load(const char *filename);