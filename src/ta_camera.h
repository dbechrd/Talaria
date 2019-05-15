#pragma once
#include "ta_scene.h"
#include "ta_primitive.h"
#include "dlb_types.h"

typedef enum {
    TA_CAMERA_FPS,
    TA_CAMERA_ORBIT
} ta_camera_mode;

typedef struct ta_camera_s {
    ta_scene *scene;
    const char *uid;

    ta_camera_mode mode;
    ta_vec3 look_target;  // where the camera is looking

    ta_vec3 front;
	ta_vec3 right;
	ta_vec3 up;
    ta_mat4 look_at;

    ta_vec3 position;     // where the camera is

    float yaw;
    float yaw_smooth;

    float pitch;
    float pitch_min;
    float pitch_max;
    float pitch_smooth;

    ta_vec3 target_pos;   // where the camera wants to be
    float target_yaw;
    float target_pitch;
    float target_vel;     // how fast to move the target
    float target_smooth;  // how fast to blend to target [0, 1]

    bool wireframe;
    bool dirty;
} ta_camera;

void ta_camera_init(ta_camera *camera);
void ta_camera_toggle_wireframe(ta_camera *camera);
void ta_camera_set_position(ta_camera *camera, float x, float y, float z);
void ta_camera_set_rotation(ta_camera *camera, float yaw, float pitch);
void ta_camera_set_target_pos_absolute(ta_camera *camera, ta_vec3 target_pos);
void ta_camera_set_target_pos_relative(ta_camera *camera, ta_vec3 delta);
void ta_camera_yaw(ta_camera *camera, float delta);
void ta_camera_pitch(ta_camera *camera, float delta);
void ta_camera_update(ta_camera *camera, double dt);