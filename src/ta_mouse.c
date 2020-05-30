#include "ta_mouse.h"
#include "ta_event.h"
#include "ta_log.h"
#include "ta_timer.h"
#include "ta_game.h"
#include "ta_window.h"

typedef struct ta_mouse {
    int x;
    int y;
    int dx;
    int dy;
    int scroll_dx;
    int scroll_dy;
    bool captured;  // true when captured, *except* drag_float
    int capture_x;
    int capture_y;
    bool dragging;  // captured specifically for drag_float
    int drag_x;     // x position before drag started
    int drag_y;     // y position before drag started
} ta_mouse;

static ta_mouse mouse;

void ta_mouse_init()
{
    ta_window_get_cursor_pos(tg_window, &mouse.x, &mouse.y);
    //mouse.captured = true;
    //SDL_SetRelativeMouseMode(mouse.captured);
}

void ta_mouse_capture_set(bool capture)
{
    if (mouse.dragging) return;
    if (mouse.captured == capture) return;

    if (capture) {
        ta_window_set_mouse_captured(tg_window, true);
        mouse.captured = true;
        mouse.capture_x = mouse.x;
        mouse.capture_y = mouse.y;
    } else {
        ta_window_set_mouse_captured(tg_window, false);
        ta_mouse_move(mouse.capture_x, mouse.capture_y);
        mouse.captured = false;
        mouse.capture_x = 0;
        mouse.capture_y = 0;
    }
}

void ta_mouse_capture_toggle()
{
    ta_mouse_capture_set(!mouse.captured);
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
    ta_window_set_mouse_captured(tg_window, true);
}

void ta_mouse_drag_end()
{
    DLB_ASSERT(mouse.dragging);
    ta_window_set_mouse_captured(tg_window, mouse.captured);
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

int ta_mouse_scroll_dx()
{
    return mouse.scroll_dx;
}

int ta_mouse_scroll_dy()
{
    return mouse.scroll_dy;
}

void ta_mouse_move(int x, int y)
{
    ta_window_set_cursor_pos(tg_window, x, y);
}

void ta_mouse_reset_relative()
{
    mouse.dx = 0;
    mouse.dy = 0;
    mouse.scroll_dx = 0;
    mouse.scroll_dy = 0;
}

void ta_mouse_event(ta_event *event)
{
    switch (event->type) {
        case INPUT_EVENT_MOUSE_MOVE: {
            mouse.x = event->data.mouse_move.x;
            mouse.y = event->data.mouse_move.y;
            mouse.dx += event->data.mouse_move.dx;
            mouse.dy += event->data.mouse_move.dy;
            break;
        } case INPUT_EVENT_MOUSE_SCROLL: {
            mouse.scroll_dx += event->data.mouse_scroll.x;
            mouse.scroll_dy += event->data.mouse_scroll.y;
            if (!event->data.mouse_scroll.flipped) {
                mouse.scroll_dy *= -1;
            }
        }
    }
}