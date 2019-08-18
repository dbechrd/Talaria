#include "ta_camera.h"
#include "ta_log.h"
#include "ta_event.h"
#include "ta_window.h"
#include "ta_game.h"
#include "dlb/dlb_vector.h"
#include "misc/gl3w.h"
#include <math.h>

void ta_camera_init(ta_camera *camera)
{
    if (!camera->position_smooth)     camera->position_smooth = 1.0f;
    if (!camera->position_target_vel) camera->position_target_vel = 0.1f;
    camera->follow_target =           camera->position;

    if (!camera->yaw)                 camera->yaw = 90.0f;
    if (!camera->yaw_smooth)          camera->yaw_smooth = 1.0f;
    camera->yaw_target =              camera->yaw;

    if (!camera->pitch_smooth)        camera->pitch_smooth = 1.0f;
    if (!camera->pitch_min)           camera->pitch_min = -90.0f;
    if (!camera->pitch_max)           camera->pitch_max = 90.0f;
    camera->pitch_target =            camera->pitch;

    if (!camera->fov)                 camera->fov = 90.0f;
    if (!camera->znear)               camera->znear = 0.1f;
    ta_camera_recalc_projection(camera);

    if (vec3_zero(camera->up))        camera->up = VEC3_Y;

    camera->dirty = true;
}

void ta_camera_set_ortho(ta_camera *camera, bool ortho)
{
    camera->ortho = ortho;
    ta_camera_recalc_projection(camera);
}

void ta_camera_set_position(ta_camera *camera, float x, float y, float z)
{
    camera->position.x = x;
    camera->position.y = y;
    camera->position.z = z;
    camera->follow_target = camera->position;
}

void ta_camera_set_rotation(ta_camera *camera, float yaw, float pitch)
{
    camera->yaw = yaw;
    camera->pitch = pitch;
}

#if 0
void ta_camera_set_rotate_accel(ta_camera *camera, float yaw_accel,
    float pitch_accel)
{
    camera->yaw_accel = yaw_accel;
    camera->pitch_accel = pitch_accel;
}
#endif

void ta_camera_set_target_pos_absolute(ta_camera *camera, ta_vec3 follow_target)
{
    camera->follow_target = follow_target;
    camera->dirty = true;
}

void ta_camera_set_target_pos_relative(ta_camera *camera, ta_vec3 delta)
{
    ta_vec3 offset = vec3_scalef(delta, camera->position_target_vel);
    camera->follow_target = vec3_add(camera->follow_target, offset);
    camera->dirty = true;
}

void ta_camera_yaw(ta_camera *camera, float delta)
{
    camera->yaw_target += delta * 0.1f;
    while (camera->yaw_target < 0.0f)    { camera->yaw_target += 360.0f; }
    while (camera->yaw_target >= 360.0f) { camera->yaw_target -= 360.0f; }
    camera->dirty = true;
}

void ta_camera_pitch(ta_camera *camera, float delta)
{
    camera->pitch_target += delta * 0.1f;
    camera->pitch_target = clampf(camera->pitch_target, camera->pitch_min,
        camera->pitch_max);
    camera->dirty = true;
}

void ta_camera_recalc_projection(ta_camera *camera)
{
    if (camera->ortho) {
        camera->projection = mat4_ortho(-1.0f, 1.0f, -1.0f, 1.0f, 0.1f, 10.0f);
    } else {
        camera->projection =
            mat4_perspective_inf(camera->fov, tg_window.aspect, camera->znear);
    }

}

void ta_camera_events()
{
    ta_camera *camera = tg_game.camera;
    ta_vec3 dir = { 0 };
    ta_event event;
    while (ta_event_pop(&event, TA_EVENT_QUEUE_CAMERA)) {
        switch (event.type) {
            case TA_EVENT_CAMERA_ASPECT_CHANGE: {
                // Update all cameras to new aspect ratio
                dlb_vec_each(ta_camera *, cam, tg_game.scene->pools[TA_CAMERA]) {
                    if (!cam->ortho) {
                        ta_camera_recalc_projection(cam);
                    }
                }
                break;
            } case TA_EVENT_CAMERA_MOVE_FORWARD: {
                dir = vec3_add(dir, camera->front);
                break;
            } case TA_EVENT_CAMERA_MOVE_BACKWARD: {
                dir = vec3_sub(dir, camera->front);
                break;
            } case TA_EVENT_CAMERA_MOVE_RIGHT: {
                dir = vec3_add(dir, camera->right);
                break;
            } case TA_EVENT_CAMERA_MOVE_LEFT: {
                dir = vec3_sub(dir, camera->right);
                break;
            } case TA_EVENT_CAMERA_MOVE_UP: {
                dir = vec3_add(dir, camera->up);
                break;
            } case TA_EVENT_CAMERA_MOVE_DOWN: {
                dir = vec3_sub(dir, camera->up);
                break;
            } case TA_EVENT_CAMERA_ROTATE: {
                if (event.data.camera_rotate.delta_yaw) {
                    ta_camera_yaw(camera, event.data.camera_rotate.delta_yaw);
                }
                if (event.data.camera_rotate.delta_pitch) {
                    ta_camera_pitch(camera, event.data.camera_rotate.delta_pitch);
                }
                break;
            } default: {
                DLB_ASSERT(!"Unhandled event type");
            }
        }
    }
    if (!vec3_zero(dir)) {
        dir = vec3_normalize(dir);
        ta_camera_set_target_pos_relative(camera, dir);
    }
}

static ta_vec3 camera_fps_target(ta_camera *camera)
{
    // Prevent pathological orientations
    DLB_ASSERT(camera->yaw >= 0.0f);
    DLB_ASSERT(camera->yaw < 360.0f);
    DLB_ASSERT(camera->pitch > -90.0f);
    DLB_ASSERT(camera->pitch < 90.0f);

    ta_vec3 result = { 0 };
    float pitch_rads = DEG_TO_RADF(camera->pitch);
    float yaw_rads = DEG_TO_RADF(camera->yaw);
    result.x = cosf(pitch_rads) * cosf(yaw_rads);
    result.y = sinf(pitch_rads);
    result.z = cosf(pitch_rads) * -sinf(yaw_rads);
    result = vec3_normalize(result);
    return result;
}

void ta_camera_update(ta_camera *camera, double dt)
{
    UNUSED(dt);

    // Update position
    ta_vec3 pos_delta = vec3_sub(camera->follow_target, camera->position);
    if (vec3_len(pos_delta) > camera->follow_distance) {
        camera->position = vec3_add(camera->position,
            vec3_scalef(pos_delta, camera->position_smooth));
        camera->dirty = true;
    }

    if (vec3_zero(camera->focal_point)) {
        // Update yaw
        float yaw_delta = camera->yaw_target - camera->yaw;
        float yaw_delta_abs = (float)fabs(yaw_delta);
        if (yaw_delta_abs > TA_EPSILON) {
            // NOTE(dlb): Negate delta when wrapping around 0/360 boundary.
            // Dunno if there's a better way to handle this.
            if (yaw_delta_abs > 180.0f) {
                float sign = (yaw_delta > 0.0f ? 1.0f : -1.0f);
                yaw_delta = (360.0f - yaw_delta_abs) * -sign;
            }
            camera->yaw += yaw_delta * camera->yaw_smooth;
            while (camera->yaw < 0.0f)    { camera->yaw += 360.0f; }
            while (camera->yaw >= 360.0f) { camera->yaw -= 360.0f; }
            camera->dirty = true;
        }

        // Update pitch
        float pitch_delta = camera->pitch_target - camera->pitch;
        if (fabs(pitch_delta) > TA_EPSILON) {
            camera->pitch += pitch_delta * camera->pitch_smooth;
            camera->pitch = clampf(camera->pitch, camera->pitch_min,
                camera->pitch_max);
            camera->dirty = true;
        }

        if (camera->dirty) {
            camera->front = camera_fps_target(camera);
            camera->right = vec3_normalize(vec3_cross(camera->front, VEC3_Y));
            camera->up = vec3_cross(camera->right, camera->front);
        }
    } else {
        if (camera->dirty) {
            camera->front = vec3_normalize(vec3_sub(camera->focal_point,
                camera->position));
            camera->right = vec3_normalize(vec3_cross(camera->front, camera->up));
        }
    }

    // Recalculate look_at
    if (camera->dirty) {
        camera->look_at = mat4_lookat_fru(camera->position, camera->front,
            camera->right, camera->up);
        camera->dirty = false;
    }
}