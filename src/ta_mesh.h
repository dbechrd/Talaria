#pragma once
#include "ta_math.h"
#include "misc/glad.h"

#define VERTEX_MAX_JOINTS 4

// Note: Morphable attributes must be contiguous and in the same order as their respective morph target attributes
// for MORPH_OFFSET to work.
typedef enum ta_vertex_attrib_type {
    TA_VERTEX_ATTR_COLOR           = 0,
    TA_VERTEX_ATTR_UV              = 1,
    TA_VERTEX_ATTR_POSITION        = 2,
    TA_VERTEX_ATTR_NORMAL          = 3,
    TA_VERTEX_ATTR_TANGENT         = 4,
    TA_VERTEX_ATTR_MORPH1_POSITION = 5,
    TA_VERTEX_ATTR_MORPH1_NORMAL   = 6,
    TA_VERTEX_ATTR_MORPH1_TANGENT  = 7,
    TA_VERTEX_ATTR_JOINTS          = 8,
    TA_VERTEX_ATTR_WEIGHTS         = 9,
    TA_VERTEX_ATTR_COUNT
} ta_vertex_attrib_type;

#define MORPH_MAX (1)
#define MORPH_OFFSET (TA_VERTEX_ATTR_MORPH1_POSITION - TA_VERTEX_ATTR_POSITION)

typedef struct ta_morph_target {
    const char *name;             // Name of morph target
    u32 base_morph_target_index;  // Index of parent (for relative morph targets)... idk if I need this but OGX has it.
} ta_morph_target;

typedef struct ta_mesh_joint_array {
    GLushort ids[VERTEX_MAX_JOINTS];  // NOTE: GLTF only supports 4 joints per vertex
} ta_mesh_joint_array;

typedef struct ta_index_array {
    size_t offset_bytes; // offset in gl_index_buffer where this array starts
    u32 material_slot;   // material id
    u16 *values;         // vector of index values
} ta_index_array;

typedef struct ta_skeleton {
    const char **bones;        // i.e. joints
    // TODO: These could probably be ta_mat4 *bind_pose_transforms; we're not interpolating bind poses, right?
    ta_vec3 *bind_pose_positions;     // bone bind-pose transforms
    ta_vec4 *bind_pose_orientations;
} ta_skeleton;

typedef struct ta_skin {
    ta_xform transform;        // skin bind-pose transform
    ta_skeleton skeleton;
    u16 *bone_count_array;     // vector
    u16 *bone_index_array;     // vector
    float *bone_weight_array;  // vector
} ta_skin;

#pragma warning(push)
#pragma warning(disable: 4201)
typedef struct ta_mesh {
    TA_RESOURCE_HEADER
    const char *path;
    // TODO: Get rid of offset now that parenting is working correctly
    ta_vec3 offset;
    ta_morph_target *morph_targets; // Array of morph targets
    //ta_vertex_array vertex_arrays[TA_VERTEX_ATTR_COUNT];
    union {
        struct {
            ta_rgba *colors;
            ta_vec2 *uvs;
            ta_vec3 *positions;
            ta_vec3 *normals;
            ta_vec3 *tangents;
            ta_vec3 *morph1_positions;
            ta_vec3 *morph1_normals;
            ta_vec3 *morph1_tangents;
            ta_mesh_joint_array *joints;
            ta_vec4 *weights;               // one weight for each joint
        };
        void *buffers[TA_VERTEX_ATTR_COUNT];
    };
    ta_index_array *index_arrays;
    ta_line_3d *vertex_normals;
    ta_line_3d *face_normals;
    ta_line_3d *tangent_lines;
    ta_skin skin;
    ta_aabb aabb;
    GLuint gl_vao;
    //GLuint gl_buffers[TA_VERTEX_ATTRIB_COUNT];
    GLuint gl_vertex_buffer;
    GLuint gl_index_buffer;
} ta_mesh;
#pragma warning(pop)

ta_mesh *tg_mesh_default;

const char *ta_vertex_attrib_type_str(int type);
void ta_mesh_init           (ta_mesh *mesh);
void ta_mesh_init_void      (void *mesh);
void ta_mesh_load_file      (ta_mesh *mesh, const char *filename);
void ta_mesh_create         (ta_mesh *mesh);
void ta_mesh_update_buffers (ta_mesh *mesh);
void ta_mesh_init_normals   (ta_mesh *mesh, float scale);
void ta_mesh_push_normals   (ta_mesh *mesh);
void ta_mesh_render         (ta_mesh *mesh, struct ta_shader *shader);
void ta_mesh_free           (ta_mesh *mesh);
void ta_mesh_free_void      (void *mesh);