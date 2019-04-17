#include "ta_camera.h"
#include "ta_log.h"
#include <math.h>

ta_camera tg_camera;

#if 0
void ta_camera_set_position(ta_camera *camera, float x, float y, float z)
{
    camera->position.x = x;
    camera->position.y = y;
    camera->position.z = z;
}

void ta_camera_set_rotation(ta_camera *camera, float yaw, float pitch)
{
    camera->yaw = yaw;
    camera->pitch = pitch;
}

void ta_camera_set_rotate_accel(ta_camera *camera, float accel_yaw,
    float accel_pitch)
{
    camera->accel_yaw = accel_yaw;
    camera->accel_pitch = accel_pitch;
}
#endif

void ta_camera_move(ta_camera *camera, ta_camera_direction direction)
{
    ta_vec3 dir = { 0 };
    switch (direction) {
        case TA_CAMERA_FORWARD: {
            dir.x = tg_camera.front.x;
            dir.z = tg_camera.front.z;
            dir = vec3_normalize(dir);
            break;
        } case TA_CAMERA_BACKWARD: {
            dir.x = -tg_camera.front.x;
            dir.z = -tg_camera.front.z;
            dir = vec3_normalize(dir);
            break;
        } case TA_CAMERA_RIGHT: {
            dir.x = tg_camera.right.x;
            dir.z = tg_camera.right.z;
            dir = vec3_normalize(dir);
            break;
        } case TA_CAMERA_LEFT: {
            dir.x = -tg_camera.right.x;
            dir.z = -tg_camera.right.z;
            dir = vec3_normalize(dir);
            break;
        } case TA_CAMERA_UP: {
            dir.y = 1.0f;
            break;
        } case TA_CAMERA_DOWN: {
            dir.y = -1.0f;
            break;
        } default: {
            DLB_ASSERT(0);
        }
    }

    ta_vec3 offset = vec3_scalef(dir, tg_camera.velocity);
    camera->position = vec3_add(camera->position, offset);
    camera->dirty = true;
}

// pitch: -89.0f - 89.0f deg
// yaw: 0.0f - 360.0f deg
static ta_vec3 ta_camera_fps_target(ta_camera *camera)
{
    DLB_ASSERT(camera->pitch > -90.0f);
    DLB_ASSERT(camera->pitch < 90.0f);
    DLB_ASSERT(camera->yaw >= 0.0f);
    DLB_ASSERT(camera->yaw < 360.0f);
    ta_vec3 result = { 0 };
    ta_vec3 dir = { 0 };
    float pitch_rads = DEG_TO_RADF(camera->pitch);
    float yaw_rads = DEG_TO_RADF(camera->yaw);
    dir.x = cosf(pitch_rads) * cosf(yaw_rads);
    dir.y = sinf(pitch_rads);
    dir.z = cosf(pitch_rads) * -sinf(yaw_rads);
    result = vec3_add(camera->position, dir);
    return result;
}

void ta_camera_update(ta_camera *camera)
{
    if (camera->mode == TA_CAMERA_FPS) {
        camera->target = ta_camera_fps_target(camera);
    }

    // NOTE: There's a bunch of duplicate work here, don't care for now
    ta_vec3 dir = vec3_normalize(vec3_sub(camera->position, camera->target));
    camera->front = vec3_negate(dir);
    camera->right = vec3_normalize(vec3_cross(VEC3_Y, dir));
    camera->up = vec3_cross(dir, camera->right);

    camera->look_at = ta_camera_lookat(camera->position, camera->target, VEC3_Y);
}

ta_mat4 ta_camera_lookat(ta_vec3 position, ta_vec3 target, ta_vec3 world_up)
{
	ta_vec3 dir = vec3_normalize(vec3_sub(position, target));
	ta_vec3 front = vec3_negate(dir);
	ta_vec3 right = vec3_normalize(vec3_cross(world_up, dir));
	ta_vec3 up = vec3_cross(dir, right);

	// [ rx, ry, rz, 0 ]
	// [ ux, uy, uz, 0 ]
	// [ dx, dy, dz, 0 ]
	// [  0,  0,  0, 1 ]
	ta_mat4 transform = { 0 };
	transform.rows.v[0].x = right.x;
	transform.rows.v[0].y = right.y;
	transform.rows.v[0].z = right.z;
	transform.rows.v[1].x = up.x;
	transform.rows.v[1].y = up.y;
	transform.rows.v[1].z = up.z;
	transform.rows.v[2].x = dir.x;
	transform.rows.v[2].y = dir.y;
	transform.rows.v[2].z = dir.z;
	transform.rows.v[3].w = 1.0f;

	// [ 1, 0, 0, -px ]
	// [ 0, 1, 0, -py ]
	// [ 0, 0, 1, -pz ]
	// [ 0, 0, 0,   1 ]
	ta_mat4 translate = { 0 };
	translate.rows.v[0].x = 1.0f;
	translate.rows.v[0].w = -position.x;
	translate.rows.v[1].y = 1.0f;
	translate.rows.v[1].w = -position.y;
	translate.rows.v[2].z = 1.0f;
	translate.rows.v[2].w = -position.z;
	translate.rows.v[3].w = 1.0f;

	ta_mat4 look_at = mat4_mul(transform, translate);
#if 0
	ta_log_write(tg_debug_log, "transform:\n");
	ta_mat4_print(&transform);
	ta_log_write(tg_debug_log, "translate:\n");
	ta_mat4_print(&translate);
	ta_log_write(tg_debug_log, "look_at:\n");
	ta_mat4_print(&look_at);
#endif
	return look_at;
}