#include "ta_mouse.h"
#include "ta_event.h"
#include "ta_log.h"
#include "ta_timer.h"
#include "ta_button_state.h"
#include "ta_game.h"
#include "ta_window.h"
#include "SDL/SDL.h"

typedef struct ta_mouse {
    int x;
    int y;
    int dx;
    int dy;
    bool captured;  // true when capture, *except* drag_float
    bool dragging;  // captured specifically for drag_float
    int drag_x;     // x position before drag started
    int drag_y;     // y position before drag started
} ta_mouse;

static ta_mouse mouse;

void ta_mouse_init()
{
    SDL_GetMouseState(&mouse.x, &mouse.y);
    //mouse.captured = true;
    //SDL_SetRelativeMouseMode(mouse.captured);
}

void ta_mouse_capture_set(bool capture)
{
    if (mouse.dragging) return;

    mouse.captured = capture;
    SDL_SetRelativeMouseMode(mouse.captured);
}

void ta_mouse_capture_toggle()
{
    if (mouse.dragging) return;

    TOGGLE(mouse.captured);
    SDL_SetRelativeMouseMode(mouse.captured);
}

bool ta_mouse_captured()
{
    return mouse.captured;
}

void ta_mouse_drag_begin()
{
    DLB_ASSERT(!mouse.dragging);
    mouse.dragging = true;
    mouse.drag_x = mouse.x;
    mouse.drag_y = mouse.y;
    SDL_SetRelativeMouseMode(true);
}

void ta_mouse_drag_end()
{
    DLB_ASSERT(mouse.dragging);
    SDL_SetRelativeMouseMode(mouse.captured);
    ta_mouse_move(mouse.drag_x, mouse.drag_y);
    mouse.dragging = false;
    mouse.drag_x = 0;
    mouse.drag_y = 0;
}

bool ta_mouse_dragging()
{
    return mouse.dragging;
}

int ta_mouse_x()
{
    return mouse.x;
}

int ta_mouse_y()
{
    return mouse.y;
}

int ta_mouse_dx()
{
    return mouse.dx;
}

int ta_mouse_dy()
{
    return mouse.dy;
}

void ta_mouse_move(int x, int y)
{
    SDL_Window *window = ta_window_sdl(tg_window);
    SDL_WarpMouseInWindow(window, x, y);
}

void ta_mouse_reset_relative()
{
    mouse.dx = 0;
    mouse.dy = 0;
}

void ta_mouse_event(struct ta_event *event)
{
    switch (event->type) {
        case TA_EVENT_MOUSE_MOVE: {
            mouse.x = event->data.mouse_move.x;
            mouse.y = event->data.mouse_move.y;
            mouse.dx = event->data.mouse_move.dx;
            mouse.dy = event->data.mouse_move.dy;
            break;
        }
    }
}