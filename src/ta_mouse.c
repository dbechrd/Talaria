#include "ta_mouse.h"
#include "ta_event.h"
#include "ta_log.h"
#include "SDL/SDL.h"

ta_mouse tg_mouse;

void ta_mouse_init()
{
    ta_log_write(tg_debug_log, "[Mouse] Initializing mouse\n");
    tg_mouse.captured = true;
    SDL_SetRelativeMouseMode(tg_mouse.captured);
    //SDL_GetMouseState(&tg_mouse.x, &tg_mouse.y);
    ta_log_write(tg_debug_log, "[Mouse] Mouse initialized\n");
}

void ta_mouse_toggle_capture()
{
    tg_mouse.captured = !tg_mouse.captured;
    SDL_SetRelativeMouseMode(tg_mouse.captured);
}

void ta_mouse_events()
{
    int dx, dy;
    u32 buttons = SDL_GetRelativeMouseState(&dx, &dy);
    tg_mouse.left   = (buttons & SDL_BUTTON_LMASK) > 0;
    tg_mouse.middle = (buttons & SDL_BUTTON_MMASK) > 0;
    tg_mouse.right  = (buttons & SDL_BUTTON_RMASK) > 0;

    // Mouse move events
    if (dx || dy) {
        tg_mouse.x += dx;
        tg_mouse.y += dy;

        ta_event mouse_move_evt = { 0 };
        mouse_move_evt.type = TA_EVENT_GLOBAL_MOUSE_MOVE;
        mouse_move_evt.data.mouse_move.dx = dx;
        mouse_move_evt.data.mouse_move.dy = dy;
        ta_event_push(&mouse_move_evt);
    }

    // Mouse click events
    if (tg_mouse.left || tg_mouse.middle || tg_mouse.right) {

        ta_event mouse_move_evt = { 0 };
        mouse_move_evt.type = TA_EVENT_GLOBAL_MOUSE_CLICK;
        mouse_move_evt.data.mouse_click.left = tg_mouse.left;
        mouse_move_evt.data.mouse_click.middle = tg_mouse.middle;
        mouse_move_evt.data.mouse_click.right = tg_mouse.right;
        ta_event_push(&mouse_move_evt);
    }
}