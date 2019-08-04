#pragma once
#include "ta_primitive.h"
#include "SDL/SDL_video.h"

#define WINDOW_ASPECT ((float)tg_window.rect.w / tg_window.rect.h)
#define X_SCREEN(x) ((float)(x) / (tg_window.rect.w / 2.0f))
#define Y_SCREEN(y) (-(float)(y) / (tg_window.rect.h / 2.0f))
#define X_TO_NDC(x) (X_SCREEN(x) - 1.0f)
#define Y_TO_NDC(y) (Y_SCREEN(y) + 1.0f)

// NOTE: Pixel origin is top-left of screen
// NOTE: NDC origin is center of screen
// e.g. [0, 0]     -> [-1.0f, 1.0f]
// e.g. [800, 450] -> [0.0f, 0.0f]

// Calculate relative x/y in pixels, returns x/y in normalized device
// coordinates (negative values are relative to right and bottom edges of
// screen)
#define NDC_X(x) (x >= 0.0f ? X_TO_NDC(x) : X_TO_NDC(x + tg_window.rect.w))
#define NDC_Y(y) (y >= 0.0f ? Y_TO_NDC(y) : Y_TO_NDC(y + tg_window.rect.h))

// Takes width/height in pixels, returns width/height in NDC
#define NDC_W(x) ((float)(x) / tg_window.rect.w * 2.0f)
#define NDC_H(y) (-(float)(y) / tg_window.rect.h * 2.0f)

#define UI_LAYER_BG     0.001f
#define UI_LAYER_SHADOW 0.002f
#define UI_LAYER_1      0.003f
#define UI_LAYER_2      0.004f
#define UI_LAYER_3      0.005f
#define UI_LAYER_4      0.006f
#define UI_LAYER_TIP    0.007f

typedef struct ta_window {
	ta_rect rect;
    float aspect;
} ta_window;

extern ta_window tg_window;

void ta_window_init(int w, int h, bool fullscreen);
void ta_window_free();
void ta_window_swap();
void ta_window_events();