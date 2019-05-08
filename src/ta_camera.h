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

typedef struct ta_scene_s ta_scene;

typedef struct {
    ta_scene *scene;
    const char *uid;
	ta_vec3 position;
    float velocity;

    ta_camera_mode mode;
    ta_vec3 target;

    ta_vec3 front;
	ta_vec3 right;
	ta_vec3 up;
    ta_mat4 look_at;

    float yaw;
    float yaw_accel;

    float pitch;
    float pitch_min;
    float pitch_max;
    float pitch_accel;

    bool wireframe;
    bool dirty;
} ta_camera;

void ta_camera_toggle_wireframe(ta_camera *camera);
void ta_camera_move(ta_camera *camera, ta_camera_direction direction);
void ta_camera_yaw(ta_camera *camera, float delta);
void ta_camera_pitch(ta_camera *camera, float delta);
void ta_camera_update(ta_camera *camera);