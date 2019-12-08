#pragma once
#include "dlb/dlb_types.h"

struct ta_size;
struct ta_event;
typedef struct SDL_Window SDL_Window;

#define WINDOW_W ta_window_width(tg_window)
#define WINDOW_H ta_window_height(tg_window)
#define WINDOW_ASPECT ta_window_aspect(tg_window)

// Converts top-left screen x/y to normalized device coordinates:
//
//          +1.0
//       ___________
//       |    |    |
// -1.0  |____|____|  +1.0
//       |    |    |
//       |____|____|
//
//          -1.0
//
#define NDC_W(x) ((float)(x) / WINDOW_W * 2.0f)
#define NDC_H(y) ((float)(y) / WINDOW_H * 2.0f)
#define NDC_X(x) (NDC_W(x) - 1.0f)
#define NDC_Y(y) (1.0f - NDC_H(y))

// NOTE: Pixel origin is top-left of screen
// NOTE: NDC origin is center of screen
// e.g. [0, 0]     -> [-1.0f, 1.0f]
// e.g. [800, 450] -> [0.0f, 0.0f]

// Calculate relative x/y in pixels (negative values are relative to right and
// bottom edges of screen)
#define SCREEN_WRAP_X(x) ((x) >= 0 ? (x) : (float)((x) + WINDOW_W))
#define SCREEN_WRAP_Y(y) ((y) >= 0 ? (y) : (float)((y) + WINDOW_H))

#define UI_LAYER_EPSILON 0.001f

#define UI_LAYER_HUD_BG          -0.0001f  // minimap
#define UI_LAYER_HUD             -0.0002f  // crosshair
#define UI_LAYER_EDIT_WINDOW_BG  -0.0003f
#define UI_LAYER_EDIT_1_BG       -0.0004f
#define UI_LAYER_EDIT_1          -0.0005f
#define UI_LAYER_EDIT_2_BG       -0.0006f
#define UI_LAYER_EDIT_2          -0.0007f
#define UI_LAYER_TIP_BG          -0.0008f
#define UI_LAYER_TIP             -0.0009f

typedef struct ta_window ta_window;

extern ta_window *tg_window;

void ta_window_init(ta_window *window, int w, int h, bool fullscreen);
void ta_window_free(ta_window *window);
struct ta_size ta_window_size(ta_window *window);
int ta_window_width(ta_window *window);
int ta_window_height(ta_window *window);
float ta_window_aspect(ta_window *window);
SDL_Window *ta_window_sdl(ta_window *window);
void ta_window_set_vsync(bool vsync);
void ta_window_swap(ta_window *window);
void ta_window_event(ta_window *window, struct ta_event *event);
int ta_window_msgbox(ta_window *window, u32 flags, const char *title,
    const char *message);