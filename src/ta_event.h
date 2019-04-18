#pragma once
#include "dlb_types.h"

typedef enum {
    TA_EVENT_QUEUE_GLOBAL,
    TA_EVENT_QUEUE_CAMERA,
    TA_EVENT_QUEUE_COUNT
} ta_event_queue_type;

#define TA_EVENT_TYPE_BITS 7
#define TA_EVENT_TYPE_FIRST(queue) ((queue) << TA_EVENT_TYPE_BITS)
#define TA_EVENT_TYPE_QUEUE(type) ((type) >> TA_EVENT_TYPE_BITS)

typedef enum {
    // Global events
    TA_EVENT_GLOBAL_QUIT = TA_EVENT_TYPE_FIRST(TA_EVENT_QUEUE_GLOBAL),
    TA_EVENT_GLOBAL_TOGGLE_MOUSE_LOCK,
    TA_EVENT_GLOBAL_MOUSE_MOVE,
    TA_EVENT_GLOBAL_TOGGLE_WIREFRAME,
    TA_EVENT_GLOBAL_TOGGLE_DEBUG_A,

    // Camera events
    TA_EVENT_CAMERA_MOVE_FORWARD = TA_EVENT_TYPE_FIRST(TA_EVENT_QUEUE_CAMERA),
    TA_EVENT_CAMERA_MOVE_BACKWARD,
    TA_EVENT_CAMERA_MOVE_RIGHT,
    TA_EVENT_CAMERA_MOVE_LEFT,
    TA_EVENT_CAMERA_MOVE_UP,
    TA_EVENT_CAMERA_MOVE_DOWN,
    TA_EVENT_CAMERA_ROTATE,
} ta_event_type;

typedef struct {
    ta_event_type type;
    int dx;
    int dy;
} ta_event_mouse_move;

typedef struct {
    ta_event_type type;
    float delta_pitch;
    float delta_yaw;
} ta_event_camera_rotate;

typedef struct {
    ta_event_type type;
    union {
        ta_event_mouse_move mouse_move;
        ta_event_camera_rotate camera_rotate;
    } data;
} ta_event;

typedef struct {
    ta_event_queue_type type;
    u32 head;  // oldest item
    u32 count;
    u32 capacity;
    ta_event *buffer;
} ta_event_queue;
ta_event_queue tg_event_queues[TA_EVENT_QUEUE_COUNT];

void ta_event_push(ta_event *event);
bool ta_event_pop(ta_event *event, ta_event_queue_type queue_type);
bool ta_event_peek(ta_event *event, ta_event_queue_type queue_type);