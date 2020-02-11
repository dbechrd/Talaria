#pragma once
#include "ta_math.h"
#include "dlb/dlb_types.h"

struct ta_event;

typedef enum ta_glsl_dbg_channel {
    DBG_NONE            = 0,
    DBG_VTX_COLOR       = 1,
    DBG_VTX_UV          = 2,
    DBG_VTX_NORMAL      = 3,
    DBG_VTX_TANGENT     = 4,
    DBG_VTX_TBN_NORMAL  = 5,
    DBG_NORMAL_MAP      = 6,
    DBG_MTL_ALBEDO      = 7,
    DBG_MTL_METALLIC    = 8,
    DBG_MTL_ROUGHNESS   = 9,
    DBG_MTL_OCCLUSION   = 10,
} ta_glsl_dbg_channel;

typedef struct ta_camera {
    TA_COMPONENT_HEADER
    float       position_smooth;      // how fast to blend to target [0, 1]
    float       position_target_vel;  // how fast to move the target
    ta_xform    target_xform;         // where the camera wants to be
    float       follow_distance;      // how far to track target
    float       yaw;                  // current yaw
    float       yaw_smooth;           // yaw smoothing factor [0, 1]
    float       yaw_target;           // desired yaw
    float       pitch;                // current pitch
    float       pitch_smooth;         // pitch smoothing factor [0, 1]
    float       pitch_min;            // pitch limit (min)
    float       pitch_max;            // pitch limit (max)
    float       pitch_target;         // desired pitch
    float       fov;                  // field of view (degrees)
    float       znear;                // near clip distance
    bool        ortho;                // true if orthographic, else perspective
    ta_vec3     focal_point;          // camera look target in world space
    ta_vec3     up;                   // camera up vector    (world space)
    ta_vec3     front;                // camera front vector (world space)
    ta_vec3     right;                // camera right vector (world space)
    ta_mat4     frustum;              // look_at without the translation
    ta_mat4     look_at;              // look_at matrix
    ta_mat4     projection;           // projection matrix (e.g. ortho, perspective, perspective_inf)
    ta_vec3     move_buffer;          // translation net delta buffer for current frame
    bool        dirty;                // true if view matrix (look_at) needs to be recalculated
    bool        debug_wireframe;      // [DEBUG] render everything from as wireframe when using this camera
    bool        debug_normals;        // [DEBUG] render normals for every mesh in the world
    bool        debug_colliders;      // [DEBUG] render colliders for every rigid_body in the world
    bool        debug_nametags;       // [DEBUG] render name tags for every transform in the world
    bool        debug_no_mesh;        // [DEBUG] disable mesh rendering for every mesh in the world
    ta_glsl_dbg_channel dbg_channel;  // [DEBUG] render only the specified debug channel (see: mesh_f.glsl)
} ta_camera;

void ta_camera_init                     (ta_camera *camera);
void ta_camera_set_ortho                (ta_camera *camera, bool ortho);
void ta_camera_set_position             (ta_camera *camera, float x, float y, float z);
void ta_camera_set_rotation             (ta_camera *camera, float yaw, float pitch);
void ta_camera_set_target_pos_absolute  (ta_camera *camera, ta_vec3 target_pos);
void ta_camera_set_target_pos_relative  (ta_camera *camera, ta_vec3 delta);
void ta_camera_yaw                      (ta_camera *camera, float delta);
void ta_camera_pitch                    (ta_camera *camera, float delta);
void ta_camera_move                     (ta_camera *camera, ta_vec3 v);
void ta_camera_recalc_projection        (ta_camera *camera);
void ta_camera_update                   (ta_camera *camera, float dt);