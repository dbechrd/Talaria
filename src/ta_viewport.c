#include "ta_viewport.h"
#include "ta_window.h"
#include "ta_mouse.h"
#include "misc/gl3w.h"

ta_viewport ta_viewport_init(ta_size size, ta_rgba background)
{
	ta_viewport view;
	view.size = size;
	view.background = background;
	return view;
}

// TODO: Push previously bound viewport onto stack if we want to enable nested
//       viewports.
void ta_viewport_bind(ta_viewport *view, ta_vec2i position, bool relative)
{
    int inv_y = tg_window.rect.h - (position.y + view->size.h);
	if (relative) {
		glViewport(position.x, inv_y, view->size.w, view->size.h);
	}
	glEnable(GL_SCISSOR_TEST);
	glScissor(position.x, inv_y, view->size.w, view->size.h);
	glClearColor(view->background.r, view->background.g, view->background.b,
		view->background.a);
    //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClear(GL_DEPTH_BUFFER_BIT);
}

void ta_viewport_unbind()
{
	glDisable(GL_SCISSOR_TEST);
	glViewport(0, 0, tg_window.rect.w, tg_window.rect.h);
	glClear(GL_DEPTH_BUFFER_BIT);
}