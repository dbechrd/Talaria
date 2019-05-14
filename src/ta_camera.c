#include "ta_camera.h"
#include "ta_log.h"
#include "ta_event.h"
#include "misc/gl3w.h"
#include <math.h>

void ta_camera_init(ta_camera *camera)
{
    camera->target_pos = camera->position;
    camera->target_yaw = camera->yaw;
    camera->target_pitch = camera->pitch;
    camera->dirty = true;
}

void ta_camera_toggle_wireframe(ta_camera *camera)
{
    camera->wireframe = !camera->wireframe;
    glPolygonMode(GL_FRONT_AND_BACK, camera->wireframe ? GL_LINE : GL_FILL);
}

void ta_camera_set_position(ta_camera *camera, float x, float y, float z)
{
    camera->position.x = x;
    camera->position.y = y;
    camera->position.z = z;
    camera->target_pos = camera->position;
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

void ta_camera_move_target(ta_camera *camera, ta_vec3 delta)
{
    ta_vec3 offset = vec3_scalef(delta, camera->target_vel);
    camera->target_pos = vec3_add(camera->target_pos, offset);
    camera->dirty = true;
}

void ta_camera_yaw(ta_camera *camera, float delta)
{
    camera->target_yaw += delta * 0.1f;
    while (camera->target_yaw < 0.0f)    { camera->target_yaw += 360.0f; }
    while (camera->target_yaw >= 360.0f) { camera->target_yaw -= 360.0f; }
    camera->dirty = true;
}

void ta_camera_pitch(ta_camera *camera, float delta)
{
    camera->target_pitch += delta * 0.1f;
    camera->target_pitch = clampf(camera->target_pitch, camera->pitch_min,
        camera->pitch_max);
    camera->dirty = true;
}

static void camera_events(ta_camera *camera)
{
    ta_vec3 dir = { 0 };
    ta_event event;
    while (ta_event_pop(&event, TA_EVENT_QUEUE_CAMERA)) {
        switch (event.type) {
            case TA_EVENT_CAMERA_MOVE_FORWARD: {
                dir.x += camera->front.x;
                dir.z += camera->front.z;
                dir = vec3_normalize(dir);
                break;
            } case TA_EVENT_CAMERA_MOVE_BACKWARD: {
                dir.x -= camera->front.x;
                dir.z -= camera->front.z;
                dir = vec3_normalize(dir);
                break;
            } case TA_EVENT_CAMERA_MOVE_RIGHT: {
                dir.x += camera->right.x;
                dir.z += camera->right.z;
                dir = vec3_normalize(dir);
                break;
            } case TA_EVENT_CAMERA_MOVE_LEFT: {
                dir.x -= camera->right.x;
                dir.z -= camera->right.z;
                dir = vec3_normalize(dir);
                break;
            } case TA_EVENT_CAMERA_MOVE_UP: {
                dir.y += 1.0f;
                break;
            } case TA_EVENT_CAMERA_MOVE_DOWN: {
                dir.y -= 1.0f;
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
    if (vec3_len(dir)) {
        ta_camera_move_target(camera, dir);
    }

}

// pitch: -89.0f - 89.0f deg
// yaw: 0.0f - 360.0f deg
static ta_vec3 camera_fps_target(ta_camera *camera)
{
    DLB_ASSERT(camera->yaw >= 0.0f);
    DLB_ASSERT(camera->yaw < 360.0f);
    DLB_ASSERT(camera->pitch > -90.0f);
    DLB_ASSERT(camera->pitch < 90.0f);

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

void ta_camera_update(ta_camera *camera, double dt)
{
    UNUSED(dt);
    camera_events(camera);

    ta_vec3 pos_delta = vec3_sub(camera->target_pos, camera->position);
    if (vec3_len(pos_delta) > TA_EPSILON) {
        camera->position = vec3_add(camera->position,
            vec3_scalef(pos_delta, camera->target_smooth));
        camera->dirty = true;
    }

    float yaw_delta = camera->target_yaw - camera->yaw;
    float yaw_delta_abs = (float)fabs(yaw_delta);
    if (yaw_delta_abs > TA_EPSILON) {
        // NOTE(dlb): Negate delta when wrapping around 0/360 boundary. Dunno if
        // there's a better way to handle this.
        if (yaw_delta_abs > 180.0f) {
            float sign = (yaw_delta > 0.0f ? 1.0f : -1.0f);
            yaw_delta = (360.0f - yaw_delta_abs) * -sign;
        }
        camera->yaw += yaw_delta * camera->yaw_smooth;
        while (camera->yaw < 0.0f)    { camera->yaw += 360.0f; }
        while (camera->yaw >= 360.0f) { camera->yaw -= 360.0f; }
        camera->dirty = true;
    }

    float pitch_delta = camera->target_pitch - camera->pitch;
    if (fabs(pitch_delta) > TA_EPSILON) {
        camera->pitch += pitch_delta * camera->pitch_smooth;
        camera->pitch = clampf(camera->pitch, camera->pitch_min,
            camera->pitch_max);
        camera->dirty = true;
    }

    if (camera->dirty) {
        if (camera->mode == TA_CAMERA_FPS) {
            camera->look_target = camera_fps_target(camera);
        }

        camera->front = vec3_normalize(vec3_sub(camera->look_target,
            camera->position));
        camera->right = vec3_normalize(vec3_cross(camera->front, VEC3_Y));
        camera->up = vec3_cross(camera->right, camera->front);
        camera->look_at = mat4_lookat_fru(camera->position, camera->front,
            camera->right, camera->up);
        camera->dirty = false;
    }
}