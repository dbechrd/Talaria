#pragma once
#include "dlb/dlb_types.h"

#if 0
typedef enum ta_event_queue_type {
    TA_EVENT_QUEUE_GLOBAL,
    TA_EVENT_QUEUE_WINDOW,
    TA_EVENT_QUEUE_GAME,
    TA_EVENT_QUEUE_CAMERA,
    TA_EVENT_QUEUE_TEXT_ENTRY,
    TA_EVENT_QUEUE_COUNT
} ta_event_queue_type;
#endif

// TODO: Add TA_ prefix
typedef enum ta_event_type {
    // Window events
    WINDOW_EVENT_RESIZE,

    // Game events
    GAME_EVENT_SHUTDOWN,
    GAME_EVENT_CAMERA_ROTATE,
    GAME_EVENT_BUTTON_ACTIVATED,
    GAME_EVENT_BUTTON_DEACTIVATED,
    GAME_EVENT_BUTTON_STATE_CHANGED,

    // Input events
    INPUT_EVENT_MOUSE_MOVE,
    INPUT_EVENT_MOUSE_SCROLL,
    INPUT_EVENT_KEY_PRESS,
    INPUT_EVENT_KEY_RELEASE,
    INPUT_EVENT_TEXT_INPUT,

    TA_EVENT_COUNT
} ta_event_type;

typedef struct ta_event_window_resize_event {
    int width;
    int height;
} ta_event_window_resize_event;

typedef struct ta_event_mouse_move_event {
    s32 x;
    s32 y;
    s32 dx;
    s32 dy;
} ta_event_mouse_move_event;

typedef struct ta_event_mouse_scroll_event {
    int x;
    int y;
    bool flipped;
} ta_event_mouse_scroll_event;

typedef struct ta_event_key_press_event {
    s32 scancode;
} ta_event_key_press_event;

typedef struct ta_event_key_release_event {
    s32 scancode;
} ta_event_key_release_event;

typedef struct ta_event_key_text_input_event {
    char chr;
} ta_event_key_text_input_event;

typedef struct ta_event_camera_rotate_event {
    float delta_pitch;
    float delta_yaw;
} ta_event_camera_rotate_event;

typedef struct ta_event_button_event {
    const char *button_name;
} ta_event_button_event;

typedef struct ta_event {
    ta_event_type type;
    bool handled;
    union {
        ta_event_window_resize_event window_resize;
        ta_event_mouse_move_event mouse_move;
        ta_event_mouse_scroll_event mouse_scroll;
        ta_event_key_press_event key_press;
        ta_event_key_release_event key_release;
        ta_event_key_text_input_event text_input;

        // TODO: Move game-specific events out of this struct?
        ta_event_camera_rotate_event camera_rotate;
        ta_event_button_event button;
    } data;
} ta_event;

void ta_event_push(ta_event *event);
bool ta_event_pop(ta_event *event);
bool ta_event_peek(ta_event *event);
void ta_event_events();