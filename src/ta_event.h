#pragma once
#include "ta_camera.h"

typedef enum ta_event_queue_type {
    TA_EVENT_QUEUE_GLOBAL,
    TA_EVENT_QUEUE_WINDOW,
    TA_EVENT_QUEUE_GAME,
    TA_EVENT_QUEUE_CAMERA,
    TA_EVENT_QUEUE_TEXT_ENTRY,
    TA_EVENT_QUEUE_COUNT
} ta_event_queue_type;

#define TA_EVENT_TYPE_BITS TA_EVENT_QUEUE_COUNT
#define TA_EVENT_TYPE_FIRST(queue) ((queue) << TA_EVENT_TYPE_BITS)
#define TA_EVENT_TYPE_QUEUE(type) ((type) >> TA_EVENT_TYPE_BITS)

typedef enum ta_event_type {
    // Global events
    TA_EVENT_GLOBAL_QUIT = TA_EVENT_TYPE_FIRST(TA_EVENT_QUEUE_GLOBAL),
    TA_EVENT_GLOBAL_WINDOW_RESIZE,
    TA_EVENT_GLOBAL_MOUSE_MOVE,
    //TA_EVENT_GLOBAL_MOUSE_CLICK,
    TA_EVENT_GLOBAL_MOUSE_SCROLL,

    // Window events
    TA_EVENT_WINDOW_RESIZE = TA_EVENT_TYPE_FIRST(TA_EVENT_QUEUE_WINDOW),

    // Game events
    TA_EVENT_GAME_QUIT = TA_EVENT_TYPE_FIRST(TA_EVENT_QUEUE_GAME),
    TA_EVENT_GAME_INIT,
    TA_EVENT_GAME_FREE_CAM,
    TA_EVENT_GAME_PLAY,
    TA_EVENT_GAME_MOUSE_MOVE,
    //TA_EVENT_GAME_MOUSE_CLICK,
    //TA_EVENT_GAME_MOUSE_SCROLL,

    // Player events
    TA_EVENT_GAME_PLAYER_MOVE_FORWARD,
    TA_EVENT_GAME_PLAYER_MOVE_BACKWARD,
    TA_EVENT_GAME_PLAYER_MOVE_RIGHT,
    TA_EVENT_GAME_PLAYER_MOVE_LEFT,
    TA_EVENT_GAME_PLAYER_JUMP,
    TA_EVENT_GAME_PLAYER_SHOOT,

    // Big red button events
    TA_EVENT_GAME_BUTTON_ACTIVATED,
    TA_EVENT_GAME_BUTTON_DEACTIVATED,
    TA_EVENT_GAME_BUTTON_STATE_CHANGED,

    TA_EVENT_EDITOR_SELECT,

    // Game events for debugging
    TA_EVENT_DEBUG_TOGGLE_MOUSE_LOCK,
    TA_EVENT_DEBUG_TOGGLE_WIREFRAME,
    TA_EVENT_DEBUG_TOGGLE_NORMALS,
    TA_EVENT_DEBUG_TOGGLE_BBOX,
    TA_EVENT_DEBUG_TOGGLE_MESH,

    // Camera events
    TA_EVENT_CAMERA_ASPECT_CHANGE = TA_EVENT_TYPE_FIRST(TA_EVENT_QUEUE_CAMERA),
    TA_EVENT_CAMERA_MOVE_FORWARD,
    TA_EVENT_CAMERA_MOVE_BACKWARD,
    TA_EVENT_CAMERA_MOVE_RIGHT,
    TA_EVENT_CAMERA_MOVE_LEFT,
    TA_EVENT_CAMERA_MOVE_UP,
    TA_EVENT_CAMERA_MOVE_DOWN,
    TA_EVENT_CAMERA_ROTATE,

    // Text entry events
    TA_EVENT_TEXT_ENTRY_KEYDOWN = TA_EVENT_TYPE_FIRST(TA_EVENT_QUEUE_TEXT_ENTRY),
} ta_event_type;

typedef struct ta_event_window_resize_event {
    int width;
    int height;
} ta_event_window_resize_event;

typedef struct ta_event_mouse_move_event {
    int dx;
    int dy;
} ta_event_mouse_move_event;

typedef struct ta_event_mouse_scroll_event {
    int x;
    int y;
    bool flipped;
} ta_event_mouse_scroll_event;

typedef struct ta_event_camera_rotate_event {
    float delta_pitch;
    float delta_yaw;
} ta_event_camera_rotate_event;

typedef struct ta_event_button_event {
    const char *button_uid;
} ta_event_button_event;

typedef struct ta_event {
    ta_event_type type;
    union {
        ta_event_window_resize_event window_resize;
        ta_event_mouse_move_event mouse_move;
        ta_event_mouse_scroll_event mouse_scroll;

        // TODO: Move game-specific events out of this struct?
        ta_event_camera_rotate_event camera_rotate;
        ta_event_button_event button;
    } data;
} ta_event;

typedef struct ta_event_queue {
    u32 head;  // oldest item
    u32 count;
    u32 capacity;
    ta_event *buffer;
} ta_event_queue;
ta_event_queue tg_event_queues[TA_EVENT_QUEUE_COUNT];

void ta_event_push(ta_event *event);
bool ta_event_pop(ta_event *event, ta_event_queue_type queue_type);
bool ta_event_peek(ta_event *event, ta_event_queue_type queue_type);
void ta_event_events();