#pragma once
#include "ta_primitive.h"
#include "dlb_types.h"

typedef enum {
    TA_CAMERA_FPS
} ta_camera_mode;

typedef enum {
    TA_CAMERA_FORWARD,
    TA_CAMERA_BACKWARD,
    TA_CAMERA_RIGHT,
    TA_CAMERA_LEFT,
    TA_CAMERA_UP,
    TA_CAMERA_DOWN
} ta_camera_direction;

typedef struct {
	ta_vec3 position;
    float velocity;

    ta_camera_mode mode;
    ta_vec3 target;

    ta_vec3 front;
	ta_vec3 right;
	ta_vec3 up;
    ta_mat4 look_at;

    float yaw;
    float pitch;
    float accel_yaw;
    float accel_pitch;

    bool wireframe;
    bool dirty;
} ta_camera;

extern ta_camera tg_camera;

#if 0
void ta_camera_set_position(ta_camera *camera, float x, float y, float z);
void ta_camera_set_rotation(ta_camera *camera, float yaw, float pitch);
void ta_camera_set_rotate_accel(ta_camera *camera, float accel_yaw,
    float accel_pitch);
#endif
void ta_camera_move(ta_camera *camera, ta_camera_direction direction);
void ta_camera_update(ta_camera *camera);
ta_mat4 ta_camera_lookat(ta_vec3 position, ta_vec3 target, ta_vec3 world_up);