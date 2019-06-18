#include "ta_ui_scrollview.h"
#include "ta_primitive.h"
#include "ta_window.h"
#include "ta_shader.h"
#include "ta_symbol.h"
#include "dlb_vector.h"
#include "misc/gl3w.h"

#define CONTENT_PAD 0
#define WIDGET_PAD 1
#define SCROLL_SPEED 20

// TODO: Use arena, no need for stretchy bufs
typedef struct {
	ta_ui_scrollview *scrollviews;
	ta_ui_image *images;
} ta_ui_buffers;

static ta_ui_buffers ui_arena;

void ta_ui_draw(int x, int y, ta_ui_base *ui)
{
	switch (ui->type) {
	case TA_UI_IMAGE:
		ta_ui_image_draw(x, y, (ta_ui_image *)ui);
		break;
	case TA_UI_SCROLLVIEW:
		ta_ui_scrollview_draw(x, y, (ta_ui_scrollview *)ui);
		break;
	};
}

void ta_ui_clear()
{
	dlb_vec_clear(ui_arena.images);
	dlb_vec_clear(ui_arena.scrollviews);
}

ta_ui_image *ta_ui_image_init(int x, int y, int w, int h, ta_texture *tex)
{
	ta_ui_image *image = dlb_vec_alloc(ui_arena.images);
	image->type = TA_UI_IMAGE;
	image->rect.x = x;
	image->rect.y = y;
	image->rect.w = w ? w : tex->width;
	image->rect.h = h ? h : tex->height;
	image->tex = tex;
	return image;
}

void ta_ui_image_draw(int x, int y, ta_ui_image *image)
{
    ta_shader_set_mat4(tg_shader_quads, SYM_U_PROJ, &MAT4_IDENT);
    ta_shader_set_mat4(tg_shader_quads, SYM_U_VIEW, &MAT4_IDENT);
    ta_shader_set_mat4(tg_shader_quads, SYM_U_MODEL, &MAT4_IDENT);
    ta_shader_set_sampler2d(tg_shader_quads, SYM_U_TEX, image->tex->gl_id);
    ta_rect parent;
    parent.x = x;
    parent.y = y;
    parent.w = 2;
    parent.h = 2;
	ta_primitive_push_rect(parent, image->rect, TA_COLOR_INVIS);
	ta_primitive_render();
    ta_shader_set_sampler2d(tg_shader_quads, SYM_U_TEX, 0);
    ta_primitive_clear();
}

ta_ui_scrollview *ta_ui_scrollview_init(int x, int y, int w, int h,
	ta_ui_base *content)
{
	ta_ui_scrollview *view = dlb_vec_alloc(ui_arena.scrollviews);
	view->type = TA_UI_SCROLLVIEW;
	view->rect.x = x;
	view->rect.y = y;
	view->rect.w = w;
	view->rect.h = h;
	int border = 0;
	view->viewport = ta_viewport_init(
		border + view->rect.x,
		border + view->rect.y,
		-border*2 + view->rect.w,
		-border*2 + view->rect.h,
		TA_COLOR_GREEN,
        0
	);
	view->content = content;

	int content_w = content->rect.x + content->rect.w + CONTENT_PAD * 2;
	int content_h = content->rect.y + content->rect.h + CONTENT_PAD * 2;
	UNUSED(content_w);
	UNUSED(content_h);

	ta_ui_scrollbar *bar = &view->scrollbar_y;
	if (content_h > view->rect.h) {
		bar->rect.w = 10;
		bar->rect.h = view->rect.h;
		bar->rect.x = (view->rect.x + view->rect.w) - bar->rect.w;
		bar->rect.y = view->rect.y;
		bar->widget.offset = 0;
		bar->widget.rect.x = WIDGET_PAD;
		bar->widget.rect.y = bar->widget.offset + WIDGET_PAD;
		bar->widget.rect.w = bar->rect.w - WIDGET_PAD*2;
		bar->widget.rect.h = bar->rect.h*2 - view->content->rect.h - WIDGET_PAD*2;
		bar->visible = true;
	}

	return view;
}

void ta_ui_scrollview_scroll(ta_ui_scrollview *view, int scroll)
{
	if (view->scrollbar_y.visible) {
		ta_ui_scrollbar *bar = &view->scrollbar_y;
		int offset = bar->widget.offset + scroll * SCROLL_SPEED;

		if (offset < 0) {
			offset = 0;
		} else if (offset > view->content->rect.h - bar->rect.h) {
			offset = view->content->rect.h - bar->rect.h;
		}

		bar->widget.offset = offset;
		bar->widget.rect.y = bar->widget.offset + WIDGET_PAD;
	}
}

void ta_ui_scrollview_draw(int x, int y, ta_ui_scrollview *view)
{
	// Background
    ta_rect parent;
    parent.x = x;
    parent.y = y;
    parent.w = 2;
    parent.h = 2;
	ta_primitive_push_rect(parent, view->rect, TA_COLOR_GRAY2);

	// TODO: Should clearing prim buffer be part of viewport_bind, or some other
	//       stack of prims? Not sure...
	ta_primitive_render();
	ta_primitive_clear();

	ta_viewport_bind(&view->viewport, false);
	{
		int cx = x + view->rect.x + CONTENT_PAD;
		int cy = y + view->rect.y + CONTENT_PAD - view->scrollbar_y.widget.offset;
		ta_ui_draw(cx, cy, view->content);
	}
	ta_viewport_unbind();

	if (view->scrollbar_y.visible) {
		// Scrollbar background
		static ta_rgba scrollbar_color = { 0.5f, 0.5f, 0.5f, 0.5f };
		ta_primitive_push_rect(parent, view->scrollbar_y.rect, scrollbar_color);

		// Scrollbar widget
        ta_rect widget_parent = parent;
		widget_parent.x += view->scrollbar_y.rect.x;
		widget_parent.y += view->scrollbar_y.rect.y;
		static ta_rgba widget_color = { 0.2f, 0.2f, 0.2f, 0.5f };
		ta_primitive_push_rect(widget_parent, view->scrollbar_y.widget.rect,
            widget_color);
	}
}