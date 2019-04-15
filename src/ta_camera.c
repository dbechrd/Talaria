#include "ta_camera.h"
#include "ta_log.h"

ta_mat4 ta_camera_lookat(ta_camera *camera, ta_vec3 position, ta_vec3 target,
	ta_vec3 up)
{
	ta_vec3 direction = vec3_normalize(vec3_sub(position, target));

	camera->position = position;
	camera->front = vec3_negate(direction);
	camera->right = vec3_normalize(vec3_cross(up, direction));
	camera->up = vec3_cross(direction, camera->right);

	// [ rx, ry, rz, 0 ]
	// [ ux, uy, uz, 0 ]
	// [ dx, dy, dz, 0 ]
	// [  0,  0,  0, 1 ]
	ta_mat4 transform = { 0 };
	transform.rows.v[0].x = camera->right.x;
	transform.rows.v[0].y = camera->right.y;
	transform.rows.v[0].z = camera->right.z;
	transform.rows.v[1].x = camera->up.x;
	transform.rows.v[1].y = camera->up.y;
	transform.rows.v[1].z = camera->up.z;
	transform.rows.v[2].x = direction.x;
	transform.rows.v[2].y = direction.y;
	transform.rows.v[2].z = direction.z;
	transform.rows.v[3].w = 1.0f;

	// [ 1, 0, 0, -px ]
	// [ 0, 1, 0, -py ]
	// [ 0, 0, 1, -pz ]
	// [ 0, 0, 0,   1 ]
	ta_mat4 translate = { 0 };
	translate.rows.v[0].x = 1.0f;
	translate.rows.v[0].w = -camera->position.x;
	translate.rows.v[1].y = 1.0f;
	translate.rows.v[1].w = -camera->position.y;
	translate.rows.v[2].z = 1.0f;
	translate.rows.v[2].w = -camera->position.z;
	translate.rows.v[3].w = 1.0f;

	ta_mat4 look_at = mat4_mul(transform, translate);
	ta_log_write(tg_debug_log, "transform:\n");
	ta_mat4_print(&transform);
	ta_log_write(tg_debug_log, "translate:\n");
	ta_mat4_print(&translate);
	ta_log_write(tg_debug_log, "look_at:\n");
	ta_mat4_print(&look_at);

	return look_at;
}