#pragma once
#include "ta_math.h"
#include "misc/glad.h"

// NOTE: Has to match shader definition
#define TA_SKIN_MAX_BONES 64

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
    TA_VERTEX_ATTR_BONE_INDICES    = 8,
    TA_VERTEX_ATTR_BONE_WEIGHTS    = 9,
    TA_VERTEX_ATTR_COUNT
} ta_vertex_attrib_type;

#define MORPH_MAX (1)
#define MORPH_OFFSET (TA_VERTEX_ATTR_MORPH1_POSITION - TA_VERTEX_ATTR_POSITION)

typedef struct ta_morph_target {
    const char *name;             // Name of morph target
    u32 base_morph_target_index;  // Index of parent (for relative morph targets)... idk if I need this but OGX has it.
} ta_morph_target;

// NOTE: These are purposely not the same order as the GL_ variations
typedef enum ta_primitive_mode {
    TA_PRIMITIVE_MODE_UNKNOWN       ,
    TA_PRIMITIVE_MODE_TRIANGLES     ,
    TA_PRIMITIVE_MODE_TRIANGLE_STRIP,
    TA_PRIMITIVE_MODE_TRIANGLE_FAN  ,
    TA_PRIMITIVE_MODE_LINES         ,
    TA_PRIMITIVE_MODE_LINE_LOOP     ,
    TA_PRIMITIVE_MODE_LINE_STRIP    ,
    TA_PRIMITIVE_MODE_POINTS        ,
    TA_PRIMITIVE_MODE_COUNT
} ta_primitive_mode;

typedef struct ta_index_array {
    ta_primitive_mode mode;  // type of primitives to draw
    size_t offset_bytes;     // offset in gl_index_buffer where this array starts
    u32 material_slot;       // material id
    u16 *values;             // vector of index values
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
    u16 *bone_count_array;
    u16 *bone_index_array;
    float *bone_weight_array;

    ta_mat4 bone_xforms[TA_SKIN_MAX_BONES];         // NOTE: Re-calculated each frame
    ta_mat4 bone_normal_xforms[TA_SKIN_MAX_BONES];  // NOTE: Re-calculated each frame
    GLuint gl_ubo_bone_xforms;
    GLuint gl_ubo_bone_normal_xforms;
    bool bone_xforms_dirty;
    bool bone_normal_xforms_dirty;
} ta_skin;

#pragma warning(push)
#pragma warning(disable: 4201)
typedef struct ta_mesh {
    TA_RESOURCE_HEADER
    const char *path;
    ta_primitive_mode mode;  // type of primitives to draw (note: overriden by index_array->mode)
    bool dynamic_draw;

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
            ta_vec4 *bone_indices;          // u16 bone indices casted to float (up to 4)
            ta_vec4 *bone_weights;          // one weight for each joint
        };
        void *buffers[TA_VERTEX_ATTR_COUNT];
    };
    ta_index_array *index_arrays;

    GLuint gl_vao;
    GLuint gl_vertex_buffer;
    GLuint gl_index_buffer;

    ta_morph_target *morph_targets; // Array of morph targets

    ta_skin skin;

    struct {
        ta_rgba *colors;
        ta_vec3 *positions;
        size_t vertex_count;
        GLuint gl_vao;
        GLuint gl_vertex_buffer;
    } debug_lines;

    ta_aabb aabb;
} ta_mesh;
#pragma warning(pop)

extern const char *tg_mesh_default;
extern GLuint tg_mesh_gl_default_bone_xforms;
extern GLuint tg_mesh_gl_default_bone_normal_xforms;

const char *ta_vertex_attrib_type_str       (int type);
GLenum ta_mesh_gl_primitive_mode            (ta_primitive_mode mode);

void ta_mesh_init                           (ta_mesh *mesh);
void ta_mesh_init_void                      (void *mesh);
void ta_mesh_load_file                      (ta_mesh *mesh, const char *filename);
void ta_mesh_create                         (ta_mesh *mesh);
void ta_mesh_calculate_joints_and_weights   (ta_mesh *mesh);
void ta_mesh_update_buffers                 (ta_mesh *mesh);
void ta_mesh_clear_buffers                  (ta_mesh *mesh);
void ta_mesh_update_debug_lines             (ta_mesh *mesh, float scale);
void ta_mesh_render_debug_lines             (ta_mesh *mesh);
void ta_mesh_render                         (ta_mesh *mesh);
void ta_mesh_free                           (ta_mesh *mesh);
void ta_mesh_free_void                      (void *mesh);