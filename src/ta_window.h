#pragma once
#include "dlb/dlb_types.h"

#define WINDOW_W ta_window_width(tg_game.window)
#define WINDOW_H ta_window_height(tg_game.window)
#define WINDOW_ASPECT ta_window_aspect(tg_game.window)

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

#define UI_LAYER_HUD_BG     -0.0001f  // minimap
#define UI_LAYER_HUD        -0.0002f  // crosshair
#define UI_LAYER_EDIT_1_BG  -0.0003f
#define UI_LAYER_EDIT_1     -0.0004f
#define UI_LAYER_EDIT_2_BG  -0.0005f
#define UI_LAYER_EDIT_2     -0.0006f
#define UI_LAYER_TIP_BG     -0.0007f
#define UI_LAYER_TIP        -0.0008f

typedef struct ta_event ta_event;
typedef struct ta_size ta_size;
typedef struct ta_window ta_window;

void ta_window_init(ta_window **ptr, int w, int h, bool fullscreen);
void ta_window_free(ta_window **ptr);
ta_size ta_window_size(ta_window *window);
int ta_window_width(ta_window *window);
int ta_window_height(ta_window *window);
float ta_window_aspect(ta_window *window);
void ta_window_swap(ta_window *window);
void ta_window_event(ta_window *window, ta_event *event);
int ta_window_msgbox(ta_window *window, u32 flags, const char *title,
    const char *message);