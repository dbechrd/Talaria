#pragma once
#include "dlb_types.h"
#include "ta_camera.h"

typedef enum {
    TA_EVENT_QUEUE_GLOBAL,
    TA_EVENT_QUEUE_GAME,
    TA_EVENT_QUEUE_CAMERA,
    TA_EVENT_QUEUE_COUNT
} ta_event_queue_type;

#define TA_EVENT_TYPE_BITS 7
#define TA_EVENT_TYPE_FIRST(queue) ((queue) << TA_EVENT_TYPE_BITS)
#define TA_EVENT_TYPE_QUEUE(type) ((type) >> TA_EVENT_TYPE_BITS)

typedef enum {
    // Global events
    TA_EVENT_GLOBAL_QUIT = TA_EVENT_TYPE_FIRST(TA_EVENT_QUEUE_GLOBAL),
    TA_EVENT_GLOBAL_MOUSE_MOVE,
    TA_EVENT_GLOBAL_MOUSE_CLICK,
    TA_EVENT_GLOBAL_MOUSE_SCROLL,

    // Game events
    TA_EVENT_GAME_QUIT = TA_EVENT_TYPE_FIRST(TA_EVENT_QUEUE_GAME),
    TA_EVENT_GAME_INIT,
    TA_EVENT_GAME_FREE_CAM,
    TA_EVENT_GAME_PLAY,
    TA_EVENT_GAME_MOUSE_MOVE,
    TA_EVENT_GAME_MOUSE_CLICK,
    TA_EVENT_GAME_MOUSE_SCROLL,

    // Player events
    TA_EVENT_GAME_PLAYER_MOVE_FORWARD,
    TA_EVENT_GAME_PLAYER_MOVE_BACKWARD,
    TA_EVENT_GAME_PLAYER_MOVE_RIGHT,
    TA_EVENT_GAME_PLAYER_MOVE_LEFT,
    TA_EVENT_GAME_PLAYER_MOVE_JUMP,

    // Game events for debugging
    TA_EVENT_DEBUG_TOGGLE_MOUSE_LOCK,
    TA_EVENT_DEBUG_TOGGLE_WIREFRAME,
    TA_EVENT_DEBUG_TOGGLE_NORMALS,
    TA_EVENT_DEBUG_TOGGLE_BBOX,
    TA_EVENT_DEBUG_TOGGLE_MESH,
    TA_EVENT_DEBUG_BOOST_PINKY,

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
    int dx;
    int dy;
} ta_event_mouse_move;

typedef struct {
    bool left;
    bool middle;
    bool right;
} ta_event_mouse_click;

typedef struct {
    int x;
    int y;
    bool flipped;
} ta_event_mouse_scroll;

typedef struct {
    float delta_pitch;
    float delta_yaw;
} ta_event_camera_rotate;

typedef struct {
    ta_event_type type;
    union {
        ta_event_mouse_move mouse_move;
        ta_event_mouse_click mouse_click;
        ta_event_mouse_scroll mouse_scroll;
        ta_event_camera_rotate camera_rotate;
    } data;
} ta_event;

typedef struct {
    u32 head;  // oldest item
    u32 count;
    u32 capacity;
    ta_event *buffer;
} ta_event_queue;
ta_event_queue tg_event_queues[TA_EVENT_QUEUE_COUNT];

void ta_event_push(ta_event *event);
bool ta_event_pop(ta_event *event, ta_event_queue_type queue_type);
bool ta_event_peek(ta_event *event, ta_event_queue_type queue_type);
void ta_event_sdl_poll();
void ta_event_update();