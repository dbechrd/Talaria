#include "ta_mouse.h"
#include "ta_event.h"
#include "ta_log.h"
#include "ta_timer.h"
#include "ta_button_state.h"
#include "SDL/SDL.h"

typedef struct ta_mouse {
    int x;
    int y;
    bool captured;
} ta_mouse;

static ta_mouse mouse;

void ta_mouse_init()
{
    ta_log_write(&tg_debug_log, "[Mouse] Initializing mouse\n");
    SDL_GetMouseState(&mouse.x, &mouse.y);
    mouse.captured = true;
    SDL_SetRelativeMouseMode(mouse.captured);
    ta_log_write(&tg_debug_log, "[Mouse] Mouse initialized\n");
}

void ta_mouse_capture_set(bool capture)
{
    mouse.captured = capture;
    SDL_SetRelativeMouseMode(mouse.captured);
}

void ta_mouse_capture_toggle()
{
    TOGGLE(mouse.captured);
    SDL_SetRelativeMouseMode(mouse.captured);
}

bool ta_mouse_captured()
{
    return mouse.captured;
}

int ta_mouse_x()
{
    return mouse.x;
}

int ta_mouse_y()
{
    return mouse.y;
}

void ta_mouse_event(struct ta_event *event)
{
    switch (event->type) {
        case TA_EVENT_MOUSE_MOVE: {
            mouse.x = event->data.mouse_move.x;
            mouse.y = event->data.mouse_move.y;
            break;
        }
    }
}