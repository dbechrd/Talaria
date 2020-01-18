#pragma once
#include "ta_uid.h"
#include "ta_math.h"
#include "dlb/dlb_types.h"

struct ta_event;

typedef enum ta_shader_debug_channel {
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
} ta_shader_debug_channel;

typedef struct ta_camera {
    u32 index;
    const char *name;
    const char *entity_name;
    float position_smooth;      // how fast to blend to target [0, 1]
    float position_target_vel;  // how fast to move the target
    ta_xform target_xform;      // where the camera wants to be
    float follow_distance;      // how far to track target
    float yaw;
    float yaw_smooth;
    float yaw_target;
    float pitch;
    float pitch_smooth;
    float pitch_min;
    float pitch_max;
    float pitch_target;
    float fov;                  // field of view (degrees)
    float znear;                // near clip distance
    bool ortho;                 // true if orthographic, else perspective
    ta_vec3 focal_point;        // camera look target in world space
    ta_vec3 up;
    ta_vec3 front;
    ta_vec3 right;
    ta_mat4 look_at;
    ta_mat4 projection;
    ta_vec3 move_buffer;        // Delta for current frame
    bool dirty;
    bool debug_wireframe;
    bool debug_normals;
    bool debug_colliders;
    bool debug_nametags;
    bool debug_no_mesh;
    ta_shader_debug_channel debug_channel;
} ta_camera;

void ta_camera_init(ta_camera *camera);
void ta_camera_set_ortho(ta_camera *camera, bool ortho);
void ta_camera_set_position(ta_camera *camera, float x, float y, float z);
void ta_camera_set_rotation(ta_camera *camera, float yaw, float pitch);
void ta_camera_set_target_pos_absolute(ta_camera *camera, ta_vec3 target_pos);
void ta_camera_set_target_pos_relative(ta_camera *camera, ta_vec3 delta);
void ta_camera_yaw(ta_camera *camera, float delta);
void ta_camera_pitch(ta_camera *camera, float delta);
void ta_camera_move(ta_camera *camera, ta_vec3 v);
void ta_camera_recalc_projection(ta_camera *camera);
void ta_camera_update(ta_camera *camera, float dt);