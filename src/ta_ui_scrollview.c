#include "ta_ui_scrollview.h"
#include "ta_primitive.h"
#include "ta_window.h"
#include "ta_shader.h"
#include "ta_symbol.h"
#include "ta_mouse.h"
#include "ta_scene.h"
#include "ta_game.h"
#include "ta_buffer.h"
#include "dlb/dlb_vector.h"
#include "misc/gl3w.h"

#define UI_DEBUG_CONTAINERS 0

#define WIDGET_PAD          1
#define SCROLL_SPEED        20

typedef struct control_state {
    bool hover;
    bool down;
    bool pressed;
    bool released;
} control_state;

//static ta_vec2i row_start;
//static ta_vec2i row_current;
//static int row_max_height;
//static bool row_continue;
static control_state last_frame_state;
static const char *status_msg;

typedef enum ui_frame_type {
    UI_ROOT,
    UI_WINDOW,
    UI_PANEL,
    UI_TEXTBOX,
    UI_BUTTON,
    UI_COUNT
} ui_frame_type;

typedef struct ui_frame {
    u32 index;
    ui_frame_type type;

    ta_rect margin;         // external margin
    ta_rect pad;            // internal padding
    ta_rect rect;           // position & size (-margin, +pad)
    ta_vec2i offset;        // dynamic offset for layout

    int row_height;         // height of current layout row
    bool row_continue;      // if true, next element will layout on same row
    ta_size content_size;   // dynamic content size (-margin, +pad)
} ui_frame;

static ui_frame ui_root = {
    .index = (u32)-1,
    .type = UI_ROOT
};
static ui_frame *ui_frames;

typedef enum ui_color_type {
    COLOR_NONE,
    COLOR_HOVER,
    COLOR_DOWN,
    COLOR_ACTIVE,
    COLOR_COUNT
} ui_color_type;

static ta_rgba ui_colors[][COLOR_COUNT] = {
    [UI_ROOT] = {
        [COLOR_NONE]  = { 1.0f, 0.0f, 0.0f, 0.5f }, //TA_COLOR_RED1
        [COLOR_HOVER] = { 1.0f, 0.0f, 0.0f, 0.5f }, //TA_COLOR_RED2
        [COLOR_DOWN]  = { 1.0f, 0.0f, 0.0f, 0.5f }, //TA_COLOR_RED3
    },
    [UI_WINDOW] = {
        [COLOR_NONE]  = { 0.0f, 1.0f, 0.0f, 0.5f }, //TA_COLOR_GREEN1
        [COLOR_HOVER] = { 0.0f, 1.0f, 0.0f, 0.5f }, //TA_COLOR_GREEN2
        [COLOR_DOWN]  = { 0.0f, 1.0f, 0.0f, 0.5f }, //TA_COLOR_GREEN3
    },
    [UI_PANEL] = {
        [COLOR_NONE]  = { 0.0f, 0.0f, 1.0f, 0.5f }, //TA_COLOR_RED
        [COLOR_HOVER] = { 0.0f, 0.0f, 1.0f, 0.5f }, //TA_COLOR_GREEN
        [COLOR_DOWN]  = { 0.0f, 0.0f, 1.0f, 0.5f }, //TA_COLOR_BLUE
    },
    [UI_TEXTBOX] = {
        [COLOR_NONE]  = { 0.5f, 0.5f, 0.5f, 0.5f }, //TA_COLOR_GRAY2
        [COLOR_HOVER] = { 1.0f, 1.0f, 0.0f, 0.5f }, //TA_COLOR_YELLOW
        [COLOR_DOWN]  = { 1.0f, 0.0f, 1.0f, 0.5f }, //TA_COLOR_MAGENTA
    },
    [UI_BUTTON] = {
        [COLOR_NONE]   = { 0.5f, 0.5f, 0.5f, 1.0f }, //TA_COLOR_GRAY2
        [COLOR_HOVER]  = { 1.0f, 1.0f, 0.0f, 1.0f }, //TA_COLOR_YELLOW
        [COLOR_DOWN]   = { 1.0f, 0.0f, 1.0f, 1.0f }, //TA_COLOR_MAGENTA
        [COLOR_ACTIVE] = { 0.0f, 1.0f, 1.0f, 1.0f }, //TA_COLOR_CYAN
    },
};

static bool type_is_container(ui_frame_type type)
{
    return (type == UI_WINDOW || type == UI_PANEL);
}

// returns closest parent container index
static ui_frame *ui_container(u32 frame_idx)
{
    DLB_ASSERT(frame_idx <= dlb_vec_len(ui_frames));

    // TODO: Could just keep track of most recent container? Idk.. this most
    // likely only runs for 1-2 iterations worst case right now.
    int i = frame_idx - 1;
    for (; i >= 0; i--) {
        if (type_is_container(ui_frames[i].type)) {
            break;
        }
    }

    ui_frame *frame = &ui_root;
    if (i >= 0) {
        DLB_ASSERT(type_is_container(ui_frames[i].type));
        frame = &ui_frames[i];
    }
    return frame;
}

static ui_frame *ui_container_last()
{
    return ui_container(dlb_vec_len(ui_frames));
}

void ta_ui_pad(int x, int y)
{
    ui_frame *container = ui_container_last();
    container->offset.x += x;
    container->offset.y += y;
}

static void ui_pop(u32 index)
{
    DLB_ASSERT(dlb_vec_len(ui_frames));
    ui_frame *frame = dlb_vec_last(ui_frames);
    while (frame != ui_frames && frame->index != index) {
        dlb_vec_popz(ui_frames);
        frame--;
    }
    DLB_ASSERT(frame->index == index);
    dlb_vec_popz(ui_frames);
}

static ta_rect rect_shrink(ta_rect orig, ta_rect shrink)
{
    ta_rect result = { 0 };
    result.x = orig.x + shrink.x;
    result.y = orig.y + shrink.y;
    result.w = orig.w - (shrink.x + shrink.w);
    result.h = orig.h - (shrink.w + shrink.h);
    return result;
}

static bool rect_contains_mouse(ta_rect rect)
{
    if (tg_mouse.x >= rect.x && tg_mouse.x < rect.x + rect.w &&
        tg_mouse.y >= rect.y && tg_mouse.y < rect.y + rect.h)
    {
        return true;
    }
    return false;
}

// returns frame index
static u32 ui_frame_start(ui_frame_type type, const char *name,
    const ta_size *size, const ta_rect *margin, const ta_rect *pad)
{
    // TODO: This probably shouldn't be here...
    glDisable(GL_SCISSOR_TEST);

    // Allocate frame
    ui_frame *frame = dlb_vec_alloc(ui_frames);
    frame->index = dlb_vec_len(ui_frames) - 1;
    frame->type = type;
    if (margin) frame->margin = *margin;
    if (pad)    frame->pad = *pad;
    ui_frame *container = ui_container(frame->index);
    frame->rect.x = container->rect.x + container->offset.x + frame->margin.x;
    frame->rect.y = container->rect.y + container->offset.y + frame->margin.y;
    frame->rect.w = size->w;
    frame->rect.h = size->h;
    frame->content_size.w = frame->pad.x + frame->pad.w;
    frame->content_size.h = frame->pad.y + frame->pad.h;

    // Margin
    //ta_ui_pad(frame->margin.x, frame->margin.y);

    // Container padding
    if (type_is_container(frame->type)) {
        ta_ui_pad(frame->pad.x, frame->pad.y);
    } else {
        last_frame_state.hover = false;
        last_frame_state.down = false;
        last_frame_state.pressed = false;
        last_frame_state.released = false;

        ta_rgba bg_color = ui_colors[frame->type][COLOR_NONE];
        if (rect_contains_mouse(frame->rect))
        {
            bg_color = ui_colors[frame->type][COLOR_HOVER];
            last_frame_state.hover = true;
            if (last_frame_state.hover) {
                status_msg = name;
            }
            if (ta_button_state_down(&tg_mouse.left)) {
                bg_color = ui_colors[frame->type][COLOR_DOWN];
                last_frame_state.down = true;
                last_frame_state.pressed = ta_button_state_pressed(&tg_mouse.left);
            } else {
                last_frame_state.released = ta_button_state_released(&tg_mouse.left);
            }
        }
    }

    // TODO: If we're going to render containers we need to defer *all*
    //       rendering to e.g. container->queue until the container pops, then
    //       render everything starting at the container for proper ordering.
    //if (UI_DEBUG_CONTAINERS || !type_is_container(frame->type)) {
    //    ta_primitive_push_rect(bg_rect, bg_color, UI_LAYER_EDIT_1_BG);
    //    ta_primitive_render(true, true);
    //}

    if (frame->type != UI_TEXTBOX) {
        // TODO: Should clip rect contain pad or not? Not sure.. see how it looks.
        ta_rect clip_rect = rect_shrink(frame->rect, frame->pad);
        glEnable(GL_SCISSOR_TEST);
        int inv_y = tg_window.rect.h - (clip_rect.y + clip_rect.h);
        glScissor(clip_rect.x, inv_y, clip_rect.w, clip_rect.h);
    }

    return frame->index;
}

static void ui_frame_end(u32 frame_idx)
{
    ui_frame *container = ui_container(frame_idx);
    ui_frame *frame = &ui_frames[frame_idx];

    int frame_w = 0;
    int frame_h = 0;
    if (type_is_container(frame->type)) {
        // assume all containers auto-expand for now
        frame_w = frame->margin.x + frame->content_size.w + frame->margin.w;
        frame_h = frame->margin.y + frame->content_size.h + frame->margin.h;
    } else {
        frame_w = frame->margin.x + frame->rect.w + frame->margin.w;
        frame_h = frame->margin.y + frame->rect.h + frame->margin.h;
    }

    container->offset.x += frame_w;
    container->row_height = MAX(container->row_height, frame_h);
    container->content_size.w = MAX(container->content_size.w, container->offset.x);
    container->content_size.h += container->row_height;

    if (!container->row_continue) {
        container->offset.x = container->pad.x;
        container->offset.y += container->row_height;
        container->row_height = 0;
    }

    // Pop container (currently required for container search to work)
    if (type_is_container(frame->type)) {
        ui_pop(frame->index);
    }
}

void ta_ui_window_begin(const char *name, const ta_size *size,
    const ta_rect *pad, int *scroll_v)
{
    glClear(GL_DEPTH_BUFFER_BIT);

    u32 frame_idx = ui_frame_start(UI_WINDOW, name, size, 0, pad);
    UNUSED(frame_idx);
    UNUSED(scroll_v);

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
}

void ta_ui_panel_begin(const char *name, const ta_size *size,
    const ta_rect *margin, const ta_rect *pad, u32 *index)
{
    u32 frame_idx = ui_frame_start(UI_PANEL, name, size, margin, pad);
    if (index) *index = frame_idx;
}

void ta_ui_panel_end(u32 index)
{
    ui_frame_end(index);
}

void ta_ui_row_end()
{
    ui_frame *container = ui_container_last();
    if (container->row_continue) {
        container->offset.x = container->pad.x;
        container->offset.y += container->row_height;
        container->row_height = 0;
        container->row_continue = false;
    }
}

void ta_ui_row_begin()
{
    ui_frame *container = ui_container_last();
    ta_ui_row_end();
    container->row_continue = true;
}

void ta_ui_tooltip(const char *text, u32 text_len)
{
    ta_rect_uv *text_rects = 0;
    ta_rectf text_rect = ta_font_push_text(&text_rects, tg_game.font,
        text, text_len, true, 0, 0);

    float offset_x = tg_mouse.x + 10.0f;
    float offset_y = tg_mouse.y + 20.0f;

    ta_rect_uv tooltip_bg = { 0 };
    tooltip_bg.rect.x = offset_x - 10.0f;
    tooltip_bg.rect.y = offset_y;
    tooltip_bg.rect.w = text_rect.w + 20.0f;
    tooltip_bg.rect.h = text_rect.h;
    ta_primitive_push_rect_uv(&tooltip_bg_queue, tooltip_bg, TA_COLOR_GRAY3A,
        UI_LAYER_TIP_BG, true);

    dlb_vec_each(ta_rect_uv *, rect, text_rects) {
        ta_rect_uv offset_rect = *rect;
        offset_rect.rect.x += offset_x;
        offset_rect.rect.y += offset_y;
        ta_primitive_push_rect_uv(&tooltip_fg_queue, offset_rect, TA_COLOR_WHITE,
            UI_LAYER_TIP, true);
    }
    dlb_vec_clearz(text_rects);
}

void ta_ui_statusbar()
//void ta_ui_statusbar(int x, int y)
{
    const float statusbar_pad = 4;
    ta_rect_uv status_bg = { 0 };
    status_bg.rect.x = statusbar_pad;
    status_bg.rect.y = -(float)(tg_game.font->line_height + statusbar_pad);
    status_bg.rect.w = (float)(tg_window.rect.w - statusbar_pad * 2);
    status_bg.rect.h = (float)tg_game.font->line_height;
    ta_primitive_push_rect_uv(&tooltip_bg_queue, status_bg, TA_COLOR_GRAY3A,
        UI_LAYER_TIP_BG, true);
}

bool ta_ui_button(const char *name, const ta_size *size, const ta_rect *margin,
    const ta_rect *pad, const ta_texture *tex)
{
    u32 frame_idx = ui_frame_start(UI_BUTTON, name, size, margin, pad);
    ui_frame *frame = &ui_frames[frame_idx];

    ta_rgba bg_color = ui_colors[frame->type][COLOR_NONE];
    if (last_frame_state.down) {
        bg_color = ui_colors[frame->type][COLOR_DOWN];
    } else if (last_frame_state.hover) {
        bg_color = ui_colors[frame->type][COLOR_HOVER];
    }

    ta_primitive_push_rect(frame->rect, bg_color, UI_LAYER_EDIT_1_BG);
    ta_primitive_render(true, true);

    if (tex) {
        ta_shader_set_mat4(tg_shader_quads, SYM_U_PROJ, &MAT4_IDENT);
        ta_shader_set_mat4(tg_shader_quads, SYM_U_VIEW, &MAT4_IDENT);
        ta_shader_set_mat4(tg_shader_quads, SYM_U_MODEL, &MAT4_IDENT);
        ta_shader_set_sampler2d(tg_shader_quads, SYM_U_TEX, tex->gl_id);
        ta_rect img_rect = rect_shrink(frame->rect, frame->pad);
        ta_primitive_push_rect(img_rect, TA_COLOR_INVIS, UI_LAYER_EDIT_1);
        ta_primitive_render(true, true);
        ta_shader_set_sampler2d(tg_shader_quads, SYM_U_TEX, 0);
    }
    ui_frame_end(frame_idx);

    return last_frame_state.pressed;
}

bool ta_ui_button_toggle(const char *name, const ta_size *size,
    const ta_rect *margin, const ta_rect *pad, const ta_texture *tex,
    bool *active)
{
    u32 frame_idx = ui_frame_start(UI_BUTTON, name, size, margin, pad);
    ui_frame *frame = &ui_frames[frame_idx];

    ta_rgba bg_color = ui_colors[frame->type][COLOR_NONE];
    if (last_frame_state.pressed) {
        *active = !*active;
    }
    if (*active) {
        bg_color = ui_colors[frame->type][COLOR_ACTIVE];
    } else if (last_frame_state.down) {
        bg_color = ui_colors[frame->type][COLOR_DOWN];
    } else if (last_frame_state.hover) {
        bg_color = ui_colors[frame->type][COLOR_HOVER];
    }

    ta_primitive_push_rect(frame->rect, bg_color, UI_LAYER_EDIT_1_BG);
    ta_primitive_render(true, true);

    if (tex) {
        ta_shader_set_mat4(tg_shader_quads, SYM_U_PROJ, &MAT4_IDENT);
        ta_shader_set_mat4(tg_shader_quads, SYM_U_VIEW, &MAT4_IDENT);
        ta_shader_set_mat4(tg_shader_quads, SYM_U_MODEL, &MAT4_IDENT);
        ta_shader_set_sampler2d(tg_shader_quads, SYM_U_TEX, tex->gl_id);
        ta_rect img_rect = rect_shrink(frame->rect, frame->pad);
        ta_primitive_push_rect(img_rect, TA_COLOR_INVIS, UI_LAYER_EDIT_1);
        ta_primitive_render(true, true);
        ta_shader_set_sampler2d(tg_shader_quads, SYM_U_TEX, 0);
    }
    ui_frame_end(frame_idx);

    return *active;
}

bool ta_ui_label(const char *name, const ta_size *size, const ta_rect *margin,
    const ta_rect *pad, const char *text)
{
    DLB_ASSERT(text);

    u32 frame_idx = ui_frame_start(UI_TEXTBOX, name, size, margin, pad);
    ui_frame *frame = &ui_frames[frame_idx];

    // Calculate text bg_rect
    static ta_rect_uv *label_rects = 0;
    ta_rectf label_rect = ta_font_push_text(&label_rects, tg_game.font, text, 0,
        true, 0, 0);

    frame->content_size.w += (int)label_rect.w;
    frame->content_size.h += (int)label_rect.h;

    // Auto-resize frame based on contents
    frame->rect.w = MAX(frame->rect.w, frame->content_size.w);
    frame->rect.h = MAX(frame->rect.h, frame->content_size.h);

    // Render background
    ta_rgba bg_color = TA_COLOR_CYAN;
    ta_rect bg_rect = frame->rect;
    ta_primitive_push_rect(bg_rect, bg_color, UI_LAYER_EDIT_1_BG);
    ta_primitive_render(true, false);

    float text_left = (float)frame->rect.x + frame->pad.x;
    float text_top = (float)frame->rect.y + frame->pad.y;

    // Render text
    dlb_vec_each(ta_rect_uv *, rect, label_rects) {
        ta_primitive_push_rect_uv(&quads_queue, *rect, TA_COLOR_WHITE, 0, true);
    }
    dlb_vec_clearz(label_rects);
    ta_font_render(quads_queue, tg_game.font, text_left, text_top,
        UI_LAYER_EDIT_1, true, true);

    ui_frame_end(frame_idx);
    return last_frame_state.pressed;
}

bool ui_textbox_filter(char c) {
    if ((c >= tg_game.font->first_char && c <= tg_game.font->last_char) ||
        c == '\n')
    {
        return true;
    }
    return false;
}

bool ta_ui_textbox(const char *name, const ta_size *size, const ta_rect *margin,
    const ta_rect *pad, text_entry_settings *text_entry)
{
    DLB_ASSERT(text_entry);

    u32 frame_idx = ui_frame_start(UI_TEXTBOX, name, size, margin, pad);
    ui_frame *frame = &ui_frames[frame_idx];

    static ta_rect_uv *text_rects = 0;

    ta_vec2 cursor_offset = { 0 };
    if (dlb_vec_len(text_entry->buffer)) {
        // Calculate text bg_rect
        ta_rectf text_rect = ta_font_push_text(&text_rects, tg_game.font,
            text_entry->buffer, dlb_vec_len(text_entry->buffer), true,
            text_entry->cursor, &cursor_offset);

        frame->content_size.w += (int)text_rect.w;
        frame->content_size.h += (int)text_rect.h;

        // Auto-resize frame based on contents
        frame->rect.w = MAX(frame->rect.w, frame->content_size.w);
        frame->rect.h = MAX(frame->rect.h, frame->content_size.h);
    }

    ta_rgba bg_color = TA_COLOR_BLACK;
    if (tg_game.text_entry.entry == text_entry) {
        bg_color = TA_COLOR_BLUE;
    }

    // Render background
    ta_rect bg_rect = frame->rect;
    ta_primitive_push_rect(bg_rect, bg_color, UI_LAYER_EDIT_1_BG);
    ta_primitive_render(true, false);

    int text_left = frame->rect.x + frame->pad.x;
    int text_top = frame->rect.y + frame->pad.y;

    // Render text
    if (dlb_vec_len(text_rects)) {
        dlb_vec_each(ta_rect_uv *, rect, text_rects) {
            ta_primitive_push_rect_uv(&quads_queue, *rect, TA_COLOR_WHITE, 0, true);
        }
        dlb_vec_clearz(text_rects);
        ta_font_render(quads_queue, tg_game.font, (float)text_left,
            (float)text_top, UI_LAYER_EDIT_1, true, true);
    }

    // Render cursor
    const int cursor_vert_pad = 0;
    ta_rect cursor_rect = { 0 };
    cursor_rect.x = text_left + (int)cursor_offset.x;
    cursor_rect.y = text_top + (int)cursor_offset.y + cursor_vert_pad;
    cursor_rect.w = 1;
    cursor_rect.h = tg_game.font->line_height - (frame->pad.y + frame->pad.h) -
        (cursor_vert_pad * 2);
    ta_primitive_push_rect(cursor_rect, TA_COLOR_RED, UI_LAYER_EDIT_2);
    ta_primitive_render(true, false);

    ui_frame_end(frame_idx);
    return last_frame_state.pressed;
}

void ta_ui_window_end()
{
    glDisable(GL_SCISSOR_TEST);

    // TODO: Render scrollbar on top of content, don't try to pad beforehand
    //       since we don't yet know the size.
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
    dlb_vec_clearz(ui_frames);
}

void ta_ui_hud()
{
    //ta_ui_pad(&TA_SIZE(10, 10));

    // TODO: Remove x,y coords from init() methods and only store size. Pass x,y
    //       at render time (make sure to update viewport correctly).
    ta_ui_window_begin(INTERN("hud"), &TA_SIZE(200, 40), 0, 0);
    ta_ui_row_begin();
    for (int i = 0; i < tg_game.player_ammo_max; i++) {
        if (i < tg_game.player_ammo) {
            ta_ui_button(INTERN("clip_slot_full"), &TA_SIZE(20, 20), 0, 0, tg_game.tex_orange);
        } else {
            ta_ui_button(INTERN("clip_slot_empty"), &TA_SIZE(20, 20), 0, 0, tg_game.tex_red);
        }
    }
    ta_ui_pad(0, 4);
    ta_ui_row_begin();
    for (int i = 0; i < tg_game.player_clip_max; i++) {
        if (i < tg_game.player_clip) {
            ta_ui_button(INTERN("ammo_slot_full"), &TA_SIZE(20, 20), 0, 0, tg_game.tex_orange);
        } else {
            ta_ui_button(INTERN("ammo_slot_empty"), &TA_SIZE(20, 20), 0, 0, tg_game.tex_red);
        }
    }
    ta_ui_window_end();

    glDisable(GL_SCISSOR_TEST);
}

static const ta_rect grid_pad = { 1, 1, 1, 1 };

void ui_4x4_grid(int rows, ta_texture *tex)
{
    for (int r = 0; r < rows; r++) {
        ta_ui_row_begin();
        for (int c = 0; c < 4; c++) {
            ta_ui_button(INTERN("4x4_cell"), &TA_SIZE(20, 20), &grid_pad, &grid_pad, tex);
        }
    }
}

void text_entry_update(text_entry_settings *text_entry)
{
    if (!text_entry->dirty)
        return;

    dlb_vec_clearz(text_entry->buffer);
    int llen = dlb_vec_len(text_entry->lbuffer);
    int rlen = dlb_vec_len(text_entry->rbuffer);
    for (int i = 0; i < llen; ++i) {
        dlb_vec_push(text_entry->buffer, text_entry->lbuffer[i]);
    }
    for (int i = rlen - 1; i >= 0; --i) {
        dlb_vec_push(text_entry->buffer, text_entry->rbuffer[i]);
    }

    text_entry->dirty = false;
}

void ta_ui_test()
{
    // TODO: Remove x,y coords from init() methods and only store size. Pass x,y
    //       at render time (make sure to update viewport correctly).
    ta_ui_window_begin(INTERN("test_window"), &TA_SIZE(300, 400), &TA_RECT1(2), 0);

    static text_entry_settings text_entry[2];
#if 0
    // TODO: Initialize text_entry_settings to w/e editable should be
    if (!text_entry[i].lbuffer) {
        const char default_text[] = "Text: "; //|`_,_`|";
        const char *c = default_text;
        while (*c) {
            dlb_vec_push(text_entry[i].lbuffer, *c);
            c++;
        }
        text_entry[i].dirty = true;
        text_entry[i].cursor = dlb_vec_len(text_entry[i].lbuffer);
    }
#endif

    for (int i = 0; i < ARRAY_COUNT(text_entry); i++) {
        ta_ui_row_begin();
        ta_ui_label(INTERN("test_label"), &TA_SIZE(30, 17), &TA_RECT(0, 0, 2, 0), 0, "Text: ");
        text_entry_update(&text_entry[i]);
        if (ta_ui_textbox(INTERN("test_textbox"), &TA_SIZE(300, 17), &TA_RECT(0, 0, 0, 2), 0,
            &text_entry[i]))
        {
            tg_game.text_entry.entry = &text_entry[i];
            tg_game.text_entry.filter = &ui_textbox_filter;
            ta_game_state_set(TA_GAME_STATE_TEXT_ENTRY);
        } else if (tg_game.text_entry.entry == &text_entry[i] &&
            ta_button_state_pressed(&tg_mouse.left))
        {
            ta_game_state_set(tg_game.state_prev);
            tg_game.text_entry.entry = 0;
            tg_game.text_entry.filter = 0;
        }
    }

    enum {
        CATEGORY_AUDIO,
        CATEGORY_NOT_USED_1,
        CATEGORY_NOT_USED_2,
        CATEGORY_NOT_USED_3,
        CATEGORY_COUNT
    };
    const char *category_names[CATEGORY_COUNT] = { 0 };
    category_names[CATEGORY_AUDIO]      = INTERN(STRING(CATEGORY_AUDIO));
    category_names[CATEGORY_NOT_USED_1] = INTERN(STRING(CATEGORY_NOT_USED_1));
    category_names[CATEGORY_NOT_USED_2] = INTERN(STRING(CATEGORY_NOT_USED_2));
    category_names[CATEGORY_NOT_USED_3] = INTERN(STRING(CATEGORY_NOT_USED_3));
    static int category_selected = -1;

    ta_ui_row_begin();
    u32 category_panel_id = (u32)-1;
    ta_ui_panel_begin(INTERN("category_panel"), &TA_SIZE(50, 50), 0, &TA_RECT1(2), &category_panel_id);
    for (int i = 0; i < CATEGORY_COUNT; i++) {
        ta_ui_row_begin();
        if (ta_ui_button(INTERN("category_button"), &TA_SIZE(50, 50), 0, &TA_RECT1(2), tg_game.tex_orange)) {
            category_selected = (i == category_selected ? -1 : i);
        }
        if (last_frame_state.hover) {
            ta_ui_tooltip(SYM(category_names[i]));
        }
    }
    ta_ui_panel_end(category_panel_id);

    switch (category_selected) {
        case -1: {
            break;
        } case CATEGORY_AUDIO: {
            // Audio buffers
            static const char *audio_playing_uid = 0;
            ta_audio_buffer *audio_buffers = tg_game.scene->pools[TYP_AUDIO_BUFFER];
            u32 buf_count = dlb_vec_len(audio_buffers);

            int audio_playing_idx = -1;
            int audio_request_idx = -1;

            u32 audio_panel_id = (u32)-1;
            ta_ui_panel_begin(INTERN("sound_panel"), &TA_SIZE(50, 50), 0, &TA_RECT1(2), &audio_panel_id);

            ta_ui_row_begin();
            for (u32 i = 0; i < buf_count; i++) {
#if 0
                int panel_id = -1;
                ta_ui_panel_begin(&TA_SIZE(60 * buf_count, 60), &panel_id);
                DLB_ASSERT(panel_id >= 0);

                ta_ui_label(audio_buffers[i].uid.uid);
                ta_ui_row_begin();
                ta_ui_button("Play");
                ta_ui_button("Loop");

                ta_ui_pad(&TA_SIZE(0, 4));
                ta_ui_panel_end(panel_id);
#endif
                bool active = audio_buffers[i].uid.uid == audio_playing_uid;
                if (active) {
                    audio_playing_idx = i;
                }
                ta_ui_button_toggle(INTERN("sound_button"), &TA_SIZE(36, 36), 0, &TA_RECT1(2), tg_game.tex_audio_icon, &active);
                if (last_frame_state.pressed) {
                    audio_request_idx = i;
                }
                if (last_frame_state.hover) {
                    ta_ui_tooltip(SYM(audio_buffers[i].uid.uid));
                }
            }

            if (audio_request_idx >= 0) {
                ta_audio_source_stop(tg_game.background_music);
                audio_playing_uid = 0;
                if (audio_request_idx != audio_playing_idx) {
                    ta_audio_source_set_buffer(tg_game.background_music, &audio_buffers[audio_request_idx]);
                    ta_audio_source_play_loop(tg_game.background_music);
                    audio_playing_uid = audio_buffers[audio_request_idx].uid.uid;
                }
            }

            ta_ui_panel_end(audio_panel_id);
            break;
        } default: {
            u32 category_details_id = (u32)-1;
            ta_ui_panel_begin(INTERN("category_details_panel"), &TA_SIZE(20, 20), 0, &grid_pad, &category_details_id);
            ui_4x4_grid(category_selected + 1, tg_game.tex_orange);
            ta_ui_panel_end(category_details_id);
            break;
        }
    }

    ta_ui_window_end();

    if (status_msg) {
        ta_ui_statusbar();

        static ta_rect_uv *status_rects = 0;
        ta_rectf status_rect = ta_font_push_text(&status_rects, tg_game.font,
            SYM(status_msg), true, 0, 0);
        dlb_vec_each(ta_rect_uv *, rect, status_rects) {
            ta_primitive_push_rect_uv(&quads_queue, *rect, TA_COLOR_WHITE, 0,
                true);
        }
        dlb_vec_clearz(status_rects);

        int status_halfw = tg_window.rect.w / 2 - (int)status_rect.w / 2;
        const int status_pad_bottom = 20;
        ta_font_render(quads_queue, tg_game.font, (float)status_halfw,
            (float)(tg_window.rect.h - (tg_game.font->ascent + status_pad_bottom)),
            UI_LAYER_TIP, true, true);

        status_msg = 0;
    }

    // Render tooltips
    ta_primitive_render_quads(tooltip_bg_queue, tg_shader_quads, true, true);
    ta_font_render(tooltip_fg_queue, tg_game.font, 0, 0, UI_LAYER_TIP, true, true);

#if 0
    // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
    if (show_demo_window)
        ImGui::ShowDemoWindow(&show_demo_window);

    // 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
    {
        static float f = 0.0f;
        static int counter = 0;

        ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

        ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
        ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
        ImGui::Checkbox("Another Window", &show_another_window);

        ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
        ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

        if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
            counter++;
        ImGui::SameLine();
        ImGui::Text("counter = %d", counter);

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::End();
    }

    // 3. Show another simple window.
    if (show_another_window)
    {
        ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
        ImGui::Text("Hello from another window!");
        if (ImGui::Button("Close Me"))
            show_another_window = false;
        ImGui::End();
    }

#endif
}