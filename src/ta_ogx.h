#pragma once
#include "dlb/dlb_types.h"

//typedef float ogx_vec2[2];  // xy
//typedef float ogx_vec3[3];  // xyz
//typedef float ogx_vec4[4];  // xyzw
//typedef float ogx_mat4[16]; // 00, 01, 02, 03, 10, ..., 33

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

#define OGX_INDEX_NULL -1

typedef struct ogx_node {
    ogx_node_type type;
    const char *name;
    s32 index;      // pool index (scene->nodes)
    s32 parent;     // parent index
                       // TODO: Do we need this, or is parent sufficient? I assume we do.
    s32 *children;  // vector of child indices
    ta_xform transform;
    ta_mat4 animated_transform;  // TODO(cleanup): temp cache thing for ta_game debug viz, probably doesn't belong here
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

// NOTE: The perfectly representable integer range of a float is [0 - 16,777,216]
// (2^24 because a float has 24 bits of mantissa)
typedef struct ogx_vertex_array {
    ta_vertex_attrib_type attrib_type;
    u16 morph_index;
    union {
        float *as_float;
        ta_vec2 *as_vec2;  // TODO: Remove these, I'm not using subarrays
        ta_vec3 *as_vec3;  // TODO: Remove these, I'm not using subarrays
    } values;
} ogx_vertex_array;

// NOTE: always GL_TRIANGLES (to allow other types, we would need a "kind" field)
typedef struct ogx_index_array {
    u16 material_slot;
    u16 *values;
} ogx_index_array;

// TODO: Replace these, and most or all other structures with ta_* rather than ogx_*. Need ta_vertex_array, etc.
//typedef struct ogx_skeleton {
//    const char **bones;                // i.e. joints
//    ogx_vec3 *bind_pose_positions;
//    ogx_vec4 *bind_pose_orientations;
//} ogx_skeleton;
//
//typedef struct ogx_skin {
//    ta_xform transform;
//    ogx_skeleton skeleton;
//    // TODO: Use u16 for these
//    //uint16_t *bone_count_array;
//    //uint16_t *bone_index_array;
//    u16 *bone_count_array;
//    u16 *bone_index_array;
//    float *bone_weight_array;
//} ogx_skin;

typedef struct ogx_morph_target {
    const char *name;
    //float index;  // TODO: What's this for?
    float base;   // TODO: u32
} ogx_morph_target;

typedef struct ogx_mesh {
    const char *name;
    ogx_morph_target *morph_targets;  // *
    ogx_vertex_array *vertex_arrays;  // +
    ogx_index_array *index_arrays;    // *  NOTE: If mesh has no index arrays, use first material
    ta_skin skin;                     // ?
} ogx_mesh;

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
    ta_vec3 color;
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
    ta_vec3     albedo_factor;
    const char *albedo_texture;
    ta_vec3     emissive_factor;
    const char *emissive_texture;
    float       metallic_factor;
    const char *metallic_texture;
    ta_vec3     normal_factor;
    const char *normal_texture;
    float       roughness_factor;
    const char *roughness_texture;
} ogx_material;

typedef struct ogx_scene {
    const char   *filename;
    ogx_node     *nodes;
    ogx_camera   *cameras;
    ogx_mesh     *meshes;
    ogx_light    *lights;
    ogx_material *materials;
    ogx_texture  *textures;
} ogx_scene;

void ta_ogx_load(ogx_scene *scene);