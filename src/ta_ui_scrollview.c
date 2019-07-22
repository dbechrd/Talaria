#include "ta_ui_scrollview.h"
#include "ta_primitive.h"
#include "ta_window.h"
#include "ta_shader.h"
#include "ta_symbol.h"
#include "ta_mouse.h"
#include "dlb_vector.h"
#include "misc/gl3w.h"

#define CONTENT_PAD 4
#define WIDGET_PAD 1
#define SCROLL_SPEED 20

// TODO: Use arena, no need for stretchy bufs
static ta_ui_control *ui_controls;

void ta_ui_clear()
{
	dlb_vec_clear(ui_controls);
}

static ta_ui_control *ta_ui_control_init(ta_ui_type type, ta_size size)
{
    ta_ui_control *control = dlb_vec_alloc(ui_controls);
    control->type = type;
    control->size = size;
    return control;
}

ta_ui_control *ta_ui_create_image(ta_size size, ta_texture *tex)
{
    ta_size actual_size = { 0 };
    actual_size.w = size.w ? size.w : tex->width;
    actual_size.h = size.h ? size.h : tex->height;
    ta_ui_control *control = ta_ui_control_init(TA_UI_IMAGE, actual_size);
    control->data.image.tex = tex;
	return control;
}

ta_ui_control *ta_ui_create_scrollview(ta_size size, ta_ui_control *content)
{
    ta_ui_control *control = ta_ui_control_init(TA_UI_SCROLLVIEW, size);
    control->data.scrollview.content = content;

#if 0
	int content_w = content->rect.x + content->rect.w + CONTENT_PAD * 2;
	int content_h = content->rect.y + content->rect.h + CONTENT_PAD * 2;

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
#endif

	return control;
}

void ta_ui_scrollview_scroll(ta_ui_scrollview *sv, int scroll)
{
    UNUSED(sv);
    UNUSED(scroll);
#if 0
	if (sv->scrollbar_y.visible) {
		ta_ui_scrollbar *bar = &sv->scrollbar_y;
		int offset = bar->widget.offset + scroll * SCROLL_SPEED;

		if (offset < 0) {
			offset = 0;
		} else if (offset > sv->content->rect.h - bar->rect.h) {
			offset = sv->content->rect.h - bar->rect.h;
		}

		bar->widget.offset = offset;
		bar->widget.rect.y = bar->widget.offset + WIDGET_PAD;
	}
#endif
}

static void ui_draw(ta_ui_control *control, ta_rect parent);
static void ui_draw_scrollview(ta_ui_control *control, ta_rect rect)
{
    ui_draw(control->data.scrollview.content, rect);

#if 0
    if (sv->scrollbar_y.visible) {
        // Scrollbar background
        static ta_rgba scrollbar_color = { 0.5f, 0.5f, 0.5f, 0.5f };
        ta_primitive_push_rect(parent, sv->scrollbar_y.rect, scrollbar_color);

        // Scrollbar widget
        ta_rect widget_parent = parent;
        widget_parent.x += sv->scrollbar_y.rect.x;
        widget_parent.y += sv->scrollbar_y.rect.y;
        static ta_rgba widget_color = { 0.2f, 0.2f, 0.2f, 0.5f };
        ta_primitive_push_rect(widget_parent, sv->scrollbar_y.widget.rect,
            widget_color);
    }
#endif
}

static void ui_draw_image(ta_ui_control *control, ta_rect rect)
{
    ta_shader_set_mat4(tg_shader_quads, SYM_U_PROJ, &MAT4_IDENT);
    ta_shader_set_mat4(tg_shader_quads, SYM_U_VIEW, &MAT4_IDENT);
    ta_shader_set_mat4(tg_shader_quads, SYM_U_MODEL, &MAT4_IDENT);
    ta_shader_set_sampler2d(tg_shader_quads, SYM_U_TEX, control->data.image.tex->gl_id);
    ta_rect img_rect = rect;
    img_rect.w = control->data.image.tex->width;
    img_rect.h = control->data.image.tex->height;
    ta_primitive_push_rect(img_rect, TA_COLOR_INVIS);
    ta_primitive_render();
    ta_shader_set_sampler2d(tg_shader_quads, SYM_U_TEX, 0);
    ta_primitive_clear(true);
}

#define UI_COLOR_CONTROL_NONE  (ta_rgba){ 0.5f, 0.5f, 0.5f, 1.0f }; //TA_COLOR_GRAY2
#define UI_COLOR_CONTROL_HOVER (ta_rgba){ 1.0f, 1.0f, 0.0f, 1.0f }; //TA_COLOR_YELLOW
#define UI_COLOR_CONTROL_CLICK (ta_rgba){ 1.0f, 0.0f, 0.0f, 1.0f }; //TA_COLOR_RED

static void ui_draw(ta_ui_control *control, ta_rect parent)
{
    ta_rect rect = { 0 };
    rect.x = parent.x;
    rect.y = parent.y;
    rect.w = control->size.w;
    rect.h = control->size.h;

    ta_rgba bg_color = UI_COLOR_CONTROL_NONE;

    if (tg_mouse.x >= rect.x && tg_mouse.x < rect.x + rect.w &&
        tg_mouse.y >= rect.y && tg_mouse.y < rect.y + rect.w)
    {
        bg_color = UI_COLOR_CONTROL_HOVER;
        if (ta_button_state_down(&tg_mouse.left)) {
            bg_color = UI_COLOR_CONTROL_CLICK;
        }
    }

    // Background color
    ta_primitive_push_rect(rect, bg_color);
    ta_primitive_render();
    ta_primitive_clear(true);

    ta_rect content_rect = { 0 };
    content_rect.x = rect.x + CONTENT_PAD;
    content_rect.y = rect.y + CONTENT_PAD - 0; //sv->scrollbar_y.widget.offset;
    content_rect.w = rect.w - CONTENT_PAD * 2;
    content_rect.h = rect.h - CONTENT_PAD * 2;

    glEnable(GL_SCISSOR_TEST);
    int inv_y = tg_window.rect.h - (content_rect.y + content_rect.h);
    glScissor(content_rect.x, inv_y, content_rect.w, content_rect.h);
    //glClear(GL_DEPTH_BUFFER_BIT);

    switch (control->type) {
        case TA_UI_IMAGE:
            ui_draw_image(control, content_rect);
            break;
        case TA_UI_SCROLLVIEW:
            ui_draw_scrollview(control, content_rect);
            break;
    };

    glDisable(GL_SCISSOR_TEST);
    //glClear(GL_DEPTH_BUFFER_BIT);
}

void ta_ui_draw(ta_ui_control *control, ta_vec2i position)
{
    glDisable(GL_DEPTH_TEST);

    ta_rect rect = { 0 };
    rect.x = position.x;
    rect.y = position.y;
    rect.w = tg_window.rect.w - position.x;
    rect.h = tg_window.rect.h - position.y;
    ui_draw(control, rect);

    glEnable(GL_DEPTH_TEST);
}