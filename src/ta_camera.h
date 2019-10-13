#pragma once
#include "ta_uid.h"
#include "ta_math.h"
#include "dlb/dlb_types.h"

struct ta_event;

typedef struct ta_camera {
    ta_vec3 position;           // where the camera is
    float position_smooth;      // how fast to blend to target [0, 1]
    float position_target_vel;  // how fast to move the target
    ta_transform target_xform;  // where the camera wants to be
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

    // Temp frame buffers
    ta_vec3 move_buffer;

    bool dirty;
    bool debug_wireframe;
    bool debug_normals;
    bool debug_bounding_boxes;
    bool debug_no_mesh;
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
void ta_camera_event(ta_camera *camera, struct ta_event *event);
void ta_camera_update(ta_camera *camera, double dt);