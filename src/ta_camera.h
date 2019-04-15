#pragma once
#include "ta_primitive.h"

typedef struct {
	ta_vec3 position;
	ta_vec3 front;
	ta_vec3 right;
	ta_vec3 up;
} ta_camera;

ta_mat4 ta_camera_lookat(ta_camera *camera, ta_vec3 position, ta_vec3 target,
	ta_vec3 up);