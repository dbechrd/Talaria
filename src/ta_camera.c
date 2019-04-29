#include "ta_camera.h"
#include "ta_log.h"
#include "ta_event.h"
#include "misc/gl3w.h"
#include <math.h>

ta_camera tg_camera;

void ta_camera_toggle_wireframe(ta_camera *camera)
{
    camera->wireframe = !camera->wireframe;
    glPolygonMode(GL_FRONT_AND_BACK, camera->wireframe ? GL_LINE : GL_FILL);
}

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

void ta_camera_set_rotate_accel(ta_camera *camera, float yaw_accel,
    float pitch_accel)
{
    camera->yaw_accel = yaw_accel;
    camera->pitch_accel = pitch_accel;
}
#endif

void ta_camera_move(ta_camera *camera, ta_camera_direction direction)
{
    ta_vec3 dir = { 0 };
    switch (direction) {
        case TA_CAMERA_FORWARD: {
            dir.x = camera->front.x;
            dir.z = camera->front.z;
            dir = vec3_normalize(dir);
            break;
        } case TA_CAMERA_BACKWARD: {
            dir.x = -camera->front.x;
            dir.z = -camera->front.z;
            dir = vec3_normalize(dir);
            break;
        } case TA_CAMERA_RIGHT: {
            dir.x = camera->right.x;
            dir.z = camera->right.z;
            dir = vec3_normalize(dir);
            break;
        } case TA_CAMERA_LEFT: {
            dir.x = -camera->right.x;
            dir.z = -camera->right.z;
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

    ta_vec3 offset = vec3_scalef(dir, camera->velocity);
    camera->position = vec3_add(camera->position, offset);
    camera->dirty = true;
}

void ta_camera_yaw(ta_camera *camera, float delta)
{
    camera->yaw += delta;
    while (camera->yaw < 0.0f) { camera->yaw += 360.0f; }
    while (camera->yaw >= 360.0f) { camera->yaw -= 360.0f; }
    camera->dirty = true;
}

void ta_camera_pitch(ta_camera *camera, float delta)
{
    camera->pitch += delta;
    camera->pitch = clampf(camera->pitch, -75.0f, 75.0f);
    camera->dirty = true;
}

static void camera_events(ta_camera *camera)
{
    ta_event event;
    while (ta_event_pop(&event, TA_EVENT_QUEUE_CAMERA)) {
        switch (event.type) {
            case TA_EVENT_CAMERA_MOVE_FORWARD:
            case TA_EVENT_CAMERA_MOVE_BACKWARD:
            case TA_EVENT_CAMERA_MOVE_LEFT:
            case TA_EVENT_CAMERA_MOVE_RIGHT:
            case TA_EVENT_CAMERA_MOVE_UP:
            case TA_EVENT_CAMERA_MOVE_DOWN:
            {
                // TODO: Normalize multiple camera move events to prevent fast
                //       diagonal movement
                ta_camera_move(camera, event.type - TA_EVENT_CAMERA_MOVE_FORWARD);
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
}

// pitch: -89.0f - 89.0f deg
// yaw: 0.0f - 360.0f deg
static ta_vec3 camera_fps_target(ta_camera *camera)
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
    camera_events(camera);

    if (camera->dirty) {
        if (camera->mode == TA_CAMERA_FPS) {
            camera->target = camera_fps_target(camera);
        }

        ta_vec3 dir = vec3_normalize(vec3_sub(camera->position, camera->target));
        camera->front = vec3_negate(dir);
        camera->right = vec3_normalize(vec3_cross(VEC3_Y, dir));
        camera->up = vec3_cross(dir, camera->right);
        // NOTE(perf): lookat duplicates all of the work above
        camera->look_at = mat4_lookat(camera->position, camera->target, VEC3_Y);
        camera->dirty = false;
    }
}