#pragma once
#include "ta_scene.h"
#include "ta_primitive.h"
#include "dlb_types.h"

typedef enum {
    TA_CAMERA_FREECAM   = 0,
    TA_CAMERA_FPS       = 1,
    TA_CAMERA_ORBIT     = 2,
} ta_camera_mode;

typedef struct ta_camera_s {
    ta_scene_ref ref;

    ta_camera_mode mode;
    bool dirty;

    ta_vec3 position;           // where the camera is
    float position_smooth;      // how fast to blend to target [0, 1]
    float position_target_vel;  // how fast to move the target
    ta_vec3 position_target;    // where the camera wants to be
    float yaw;
    float yaw_smooth;
    float yaw_target;
    float pitch;
    float pitch_smooth;
    float pitch_min;
    float pitch_max;
    float pitch_target;
    float fov;
    float nearz;

    ta_vec3 focal_point;        // camera look target in world space
    ta_vec3 up;
    ta_vec3 front;
    ta_vec3 right;
    ta_mat4 look_at;
    ta_mat4 projection;
    bool ortho;

    bool debug_wireframe;
    bool debug_normals;
    bool debug_bounding_spheres;
    bool debug_bounding_boxes;
    bool debug_no_mesh;
    bool debug_follow_pinky;
} ta_camera;

const char *ta_camera_mode_str(int type);
void ta_camera_init(ta_camera *camera);
void ta_camera_set_ortho(ta_camera *camera, bool ortho);
void ta_camera_set_position(ta_camera *camera, float x, float y, float z);
void ta_camera_set_rotation(ta_camera *camera, float yaw, float pitch);
void ta_camera_set_target_pos_absolute(ta_camera *camera, ta_vec3 position_target);
void ta_camera_set_target_pos_relative(ta_camera *camera, ta_vec3 delta);
void ta_camera_yaw(ta_camera *camera, float delta);
void ta_camera_pitch(ta_camera *camera, float delta);
void ta_camera_recalc_projection(ta_camera *camera);
void ta_camera_update(ta_camera *camera, double dt);