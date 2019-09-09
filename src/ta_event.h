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

typedef enum ta_event_type {
    TA_EVENT_NULL,

    // Window events
    TA_EVENT_WINDOW_RESIZE,

    // Input events
    TA_EVENT_MOUSE_MOVE,
    TA_EVENT_MOUSE_SCROLL,
    TA_EVENT_KEY_PRESS,
    TA_EVENT_KEY_RELEASE,
    TA_EVENT_TEXT_INPUT,

    // Camera events
    TA_EVENT_CAMERA_MOVE_FORWARD,
    TA_EVENT_CAMERA_MOVE_BACKWARD,
    TA_EVENT_CAMERA_MOVE_RIGHT,
    TA_EVENT_CAMERA_MOVE_LEFT,
    TA_EVENT_CAMERA_MOVE_UP,
    TA_EVENT_CAMERA_MOVE_DOWN,
    TA_EVENT_CAMERA_ROTATE,

    // Game events
    TA_EVENT_GAME_STARTUP,
    TA_EVENT_GAME_FREE_CAM,
    TA_EVENT_GAME_PLAY,
    TA_EVENT_GAME_EDITOR,
    TA_EVENT_GAME_SHUTDOWN,

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

    // Game events for debugging
    TA_EVENT_DEBUG_TOGGLE_MOUSE_LOCK,
    TA_EVENT_DEBUG_TOGGLE_WIREFRAME,
    TA_EVENT_DEBUG_TOGGLE_NORMALS,
    TA_EVENT_DEBUG_TOGGLE_BBOX,
    TA_EVENT_DEBUG_TOGGLE_MESH,

    // Editor events
    TA_EVENT_EDITOR_CLOSE,
    TA_EVENT_EDITOR_SELECT,
    TA_EVENT_EDITOR_TXT_NEWLINE,
    TA_EVENT_EDITOR_TXT_SUBMIT,
    TA_EVENT_EDITOR_TXT_CANCEL,
    TA_EVENT_EDITOR_TXT_BACKSPACE,
    TA_EVENT_EDITOR_TXT_DELETE,
    TA_EVENT_EDITOR_TXT_CURSOR_RIGHT,
    TA_EVENT_EDITOR_TXT_CURSOR_LEFT,
    TA_EVENT_EDITOR_TXT_CURSOR_DOWN,
    TA_EVENT_EDITOR_TXT_CURSOR_UP,
    TA_EVENT_EDITOR_TXT_CURSOR_BOL,
    TA_EVENT_EDITOR_TXT_CURSOR_EOL,
    TA_EVENT_EDITOR_TXT_CURSOR_BOF,
    TA_EVENT_EDITOR_TXT_CURSOR_EOF,
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
    const char *button_uid;
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

typedef struct ta_event_queue {
    u32 head;  // oldest item
    u32 count;
    ta_event *buffer;
} ta_event_queue;

void ta_event_push(ta_event *event);
bool ta_event_pop(ta_event *event);
bool ta_event_peek(ta_event *event);
void ta_event_events();