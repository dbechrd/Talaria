#include "ta_viewport.h"
#include "ta_window.h"
#include "misc/gl3w.h"

ta_viewport ta_viewport_init(int left, int top, int width, int height,
	ta_rgba background, ta_camera *camera)
{
	ta_viewport view;
	view.rect.x = left;
	view.rect.y = top;
	view.rect.w = width;
	view.rect.h = height;
	view.background = background;
    view.camera = camera;
	return view;
}

// TODO: Push previously bound viewport onto stack if we want to enable nested
//       viewports.
void ta_viewport_bind(ta_viewport *view, bool stretch_to_fit)
{
    int inv_y = tg_window.rect.h - (view->rect.y + view->rect.h);
	if (stretch_to_fit) {
		glViewport(view->rect.x, inv_y, view->rect.w, view->rect.h);
	}
	glEnable(GL_SCISSOR_TEST);
	glScissor(view->rect.x, inv_y, view->rect.w, view->rect.h);
	glClearColor(view->background.r, view->background.g, view->background.b,
		view->background.a);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void ta_viewport_unbind()
{
	glDisable(GL_SCISSOR_TEST);
	glViewport(0, 0, tg_window.rect.w, tg_window.rect.h);
	glClear(GL_DEPTH_BUFFER_BIT);
}