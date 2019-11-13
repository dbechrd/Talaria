#include "ta_viewport.h"
#include "ta_window.h"
#include "ta_mouse.h"
#include "ta_game.h"
#include "misc/gl3w.h"

typedef struct viewport {
    ta_rect viewport_rect;
    bool scissor_enabled;
    ta_rect scissor_rect;
    ta_rgba clear_color;
} viewport;

static viewport viewports[16];
static int next = 0;

// TODO: Push previously bound viewport onto stack if we want to enable nested
//       viewports.
void ta_viewport_bind(ta_rect rect, ta_rgba background, bool relative)
{
    DLB_ASSERT(next < ARRAY_COUNT(viewports));

    // Save previous state
    glGetIntegerv(GL_VIEWPORT, (int *)&viewports[next].viewport_rect);
    glGetBooleanv(GL_SCISSOR_TEST, (GLboolean *)&viewports[next].scissor_enabled);
    glGetIntegerv(GL_SCISSOR_BOX, (int *)&viewports[next].scissor_rect);
    glGetFloatv(GL_COLOR_CLEAR_VALUE, (float *)&viewports[next].clear_color);
    next++;

    // Set new state
    int inv_y = WINDOW_H - (rect.y + rect.h);
    if (relative) {
        glViewport(rect.x, inv_y, rect.w, rect.h);
    }
    glEnable(GL_SCISSOR_TEST);
    glScissor(rect.x, inv_y, rect.w, rect.h);
    glClearColor(background.r, background.g, background.b, background.a);
    //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClear(GL_DEPTH_BUFFER_BIT);
}

void ta_viewport_unbind()
{
    DLB_ASSERT(next > 0);
    next--;

    glViewport(
        viewports[next].viewport_rect.x,
        viewports[next].viewport_rect.y,
        viewports[next].viewport_rect.w,
        viewports[next].viewport_rect.h
    );
    if (viewports[next].scissor_enabled) {
        glEnable(GL_SCISSOR_TEST);
    } else {
        glDisable(GL_SCISSOR_TEST);
    }
    glScissor(
        viewports[next].scissor_rect.x,
        viewports[next].scissor_rect.y,
        viewports[next].scissor_rect.w,
        viewports[next].scissor_rect.h
    );
    glClearColor(
        viewports[next].clear_color.r,
        viewports[next].clear_color.g,
        viewports[next].clear_color.b,
        viewports[next].clear_color.a
    );
    glClear(GL_DEPTH_BUFFER_BIT);
}