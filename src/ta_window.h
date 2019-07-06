#pragma once
#include "SDL/SDL_video.h"
#include "dlb_types.h"
#include "ta_primitive.h"

#define WINDOW_ASPECT ((float)tg_window.rect.w / tg_window.rect.h)
#define X_TO_NDC(x) ((float)(x) / (tg_window.rect.w / 2.0f) - 1.0f)
#define Y_TO_NDC(y) (-(float)(y) / (tg_window.rect.h / 2.0f) + 1.0f)

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

typedef struct {
	ta_rect rect;
    float aspect;
} ta_window;

extern ta_window tg_window;

void ta_window_init(int w, int h, bool fullscreen);
void ta_window_swap();
void ta_window_free();