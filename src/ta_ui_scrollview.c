#include "ta_ui_scrollview.h"
#include "ta_primitive.h"
#include "ta_window.h"
#include "ta_shader_quads.h"
#include "dlb_vector.h"
#include "misc/gl3w.h"

#define CONTENT_PAD 0

ta_ui_scrollview *buffer;

static ta_color4 invis = { 0.0f, 0.0f, 0.0f, 0.0f };
static ta_color4 red =   { 0.7f, 0.1f, 0.1f, 1.0f };
static ta_color4 green = { 0.1f, 0.7f, 0.1f, 1.0f };
static ta_color4 blue =  { 0.1f, 0.1f, 0.7f, 1.0f };
static ta_color4 gray1 = { 0.1f, 0.1f, 0.1f, 1.0f };
static ta_color4 gray2 = { 0.2f, 0.2f, 0.2f, 1.0f };
static ta_color4 gray3 = { 0.3f, 0.3f, 0.3f, 1.0f };
static ta_color4 gray4 = { 0.4f, 0.4f, 0.4f, 1.0f };
static ta_color4 gray5 = { 0.5f, 0.5f, 0.5f, 1.0f };
static ta_color4 gray6 = { 0.6f, 0.6f, 0.6f, 1.0f };
static ta_color4 gray7 = { 0.7f, 0.7f, 0.7f, 1.0f };
static ta_color4 gray8 = { 0.8f, 0.8f, 0.8f, 1.0f };
static ta_color4 gray9 = { 0.9f, 0.9f, 0.9f, 1.0f };

ta_ui_scrollview *ta_ui_scrollview_init(int x, int y, int w, int h, ta_ui_base *content)
{
	ta_ui_scrollview *view = dlb_vec_alloc(buffer);
	view->type = TA_UI_SCROLLVIEW;
	view->rect.x = x;
	view->rect.y = y;
	view->rect.w = w;
	view->rect.h = h;
	view->content = content;
	
	int content_w = content->rect.x + content->rect.w + CONTENT_PAD * 2;
	int content_h = content->rect.y + content->rect.h + CONTENT_PAD * 2;
	UNUSED(content_w);
	UNUSED(content_h);

	ta_ui_scrollbar *bar = &view->scrollbar;
	bar->rect.w = 10;
	bar->rect.h = view->rect.h;
	bar->rect.x = (view->rect.x + view->rect.w) - bar->rect.w;
	bar->rect.y = view->rect.y;
	bar->widget.offset = 0;
	bar->widget.rect.x = bar->rect.x + 1;
	bar->widget.rect.y = bar->rect.y + bar->widget.offset + 1;
	bar->widget.rect.w = bar->rect.w - 2;
	if (content_h > view->rect.h) {
		bar->widget.rect.h = bar->rect.h * view->rect.h / content_h - 2;
	} else {
		bar->widget.rect.h = bar->rect.h - 2;
	}
	
	return view;
}

void ta_ui_scrollview_scroll(ta_ui_scrollview *view, int scroll)
{
	ta_ui_scrollbar *bar = &view->scrollbar;
	int offset = bar->widget.offset + scroll * 20;

	if (offset < 0) {
		offset = 0;
	} else if (offset > bar->rect.h - 2 - bar->widget.rect.h) {
		offset = bar->rect.h - 2 - bar->widget.rect.h;
	}

	bar->widget.offset = offset;
	bar->widget.rect.y = bar->rect.y + 1 + bar->widget.offset;
}

void ta_ui_scrollview_draw(ta_ui_scrollview *view)
{
	// Background
	ta_primitive_push_rect(&view->rect, &gray2);

	ta_primitive_render();
	ta_primitive_clear();

	// Content
	int content_h = view->content->rect.y + view->content->rect.h + CONTENT_PAD;
	ta_rect content_rect = view->content->rect;
	content_rect.x += view->rect.x + CONTENT_PAD;
	content_rect.y += view->rect.y + CONTENT_PAD - (view->scrollbar.widget.offset * content_h / view->rect.h);

	glEnable(GL_SCISSOR_TEST);
	int border = 0;
	glScissor(
		border + view->rect.x,
		border + tg_window.height - (view->rect.y + view->rect.h),
		-border*2 + view->rect.w,
		-border*2 + view->rect.h
	);
	ta_shader_quads_set_texture(0, 1);
	ta_primitive_push_rect(&content_rect, &invis);
	ta_primitive_render();
	ta_shader_quads_set_texture(0, 0);
	ta_primitive_clear();
	glDisable(GL_SCISSOR_TEST);

	// Scrollbar / widget
	static ta_color4 scrollbar_color = { 0.5f, 0.5f, 0.5f, 0.5f };
	static ta_color4 widget_color = { 0.2f, 0.2f, 0.2f, 0.5f };
	ta_primitive_push_rect(&view->scrollbar.rect, &scrollbar_color);
	ta_primitive_push_rect(&view->scrollbar.widget.rect, &widget_color);
}

void ta_ui_clear()
{
	dlb_vec_clear(buffer);
}