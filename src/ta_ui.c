#include "ta_ui.h"
#include "ta_primitive.h"
#include "ta_window.h"
#include "ta_shader.h"
#include "ta_symbol.h"
#include "ta_mouse.h"
#include "ta_game.h"
#include "ta_font.h"
#include "ta_text_entry.h"
#include "ta_texture.h"
#include "ta_keybind.h"
#include "dlb/dlb_vector.h"
#include "misc/gl3w.h"

#define UI_DEBUG_MARGIN         1
#define UI_DEBUG_PAD            1
#define UI_DEBUG_CONTAINERS     0
#define UI_DEBUG_NO_TEXTURES    0
#define UI_DEBUG_RANDOM_COLORS  0

#define WIDGET_PAD          1
#define SCROLL_SPEED        20

typedef enum ui_frame_type {
    UI_ROOT,
    UI_WINDOW,
    UI_PANEL,
    UI_BUTTON,
    UI_LABEL,
    UI_TEXTBOX,
    UI_COUNT
} ui_frame_type;

typedef struct ui_style {
    ta_rect margin;
    ta_rect pad;
    ta_rgba bg_color[UI_STATE_COUNT];
    ta_rgba fg_color[UI_STATE_COUNT];
    bool invisible;
} ui_style;

static ui_style ui_default_style[UI_COUNT] = { 0 };

static ta_size next_frame_size;
static ui_style next_frame_style;
static ta_ui_state last_frame_state;

typedef struct ui_frame {
    u32 index;
    ui_frame_type type;
    const char *name;

    ta_rect margin;
    ta_rect pad;
    ta_rgba bg_color;
    ta_rgba fg_color;
    bool invisible;
    ta_rect rect;           // position & size (-margin, +pad)
    ta_vec2i offset;        // dynamic offset for layout

    int row_height;         // height of current layout row
    bool row_continue;      // if true, next element will layout on same row
    ta_size content_size;   // dynamic content size (-margin, +pad)
} ui_frame;

static ui_frame *ui_frames;

void ta_ui_init()
{
    // Reserve element zero for UI_ROOT
    dlb_vec_alloc(ui_frames);

    //ui_default_style[UI_ROOT].margin                        = TA_RECT_ZERO;
    //ui_default_style[UI_ROOT].pad                           = TA_RECT_ZERO;
    //ui_default_style[UI_ROOT].bg_color[UI_STATE_NONE]       = TA_COLOR_INVIS;
    ui_default_style[UI_ROOT].bg_color[UI_STATE_HOVER]      = TA_RGBA(1.0f, 0.0f, 0.0f, 0.9f);
    //ui_default_style[UI_ROOT].bg_color[UI_STATE_DOWN]       = TA_COLOR_INVIS;
    //ui_default_style[UI_ROOT].bg_color[UI_STATE_ACTIVE]     = TA_COLOR_INVIS;
    //ui_default_style[UI_ROOT].fg_color[UI_STATE_NONE]       = TA_COLOR_INVIS;
    //ui_default_style[UI_ROOT].fg_color[UI_STATE_HOVER]      = TA_COLOR_INVIS;
    //ui_default_style[UI_ROOT].fg_color[UI_STATE_DOWN]       = TA_COLOR_INVIS;
    //ui_default_style[UI_ROOT].fg_color[UI_STATE_ACTIVE]     = TA_COLOR_INVIS;

    //ui_default_style[UI_WINDOW].margin = TA_RECT_ZERO;
    //ui_default_style[UI_WINDOW].pad    = TA_RECT_ZERO;
    //ui_default_style[UI_WINDOW].bg_color[UI_STATE_NONE]     = TA_COLOR_INVIS;
    ui_default_style[UI_WINDOW].bg_color[UI_STATE_HOVER]    = TA_RGBA(0.0f, 1.0f, 0.0f, 0.9f);
    //ui_default_style[UI_WINDOW].bg_color[UI_STATE_DOWN]     = TA_COLOR_INVIS;
    //ui_default_style[UI_WINDOW].bg_color[UI_STATE_ACTIVE]   = TA_COLOR_INVIS;
    //ui_default_style[UI_WINDOW].fg_color[UI_STATE_NONE]     = TA_COLOR_INVIS;
    //ui_default_style[UI_WINDOW].fg_color[UI_STATE_HOVER]    = TA_COLOR_INVIS;
    //ui_default_style[UI_WINDOW].fg_color[UI_STATE_DOWN]     = TA_COLOR_INVIS;
    //ui_default_style[UI_WINDOW].fg_color[UI_STATE_ACTIVE]   = TA_COLOR_INVIS;

    ui_default_style[UI_PANEL].margin                       = TA_RECT(0, 0, 0, 0);
    ui_default_style[UI_PANEL].pad                          = TA_RECT(0, 0, 0, 0);
    //ui_default_style[UI_PANEL].bg_color[UI_STATE_NONE]      = TA_COLOR_INVIS;
    ui_default_style[UI_PANEL].bg_color[UI_STATE_HOVER]     = TA_RGBA(0.0f, 0.0f, 1.0f, 0.9f);
    //ui_default_style[UI_PANEL].bg_color[UI_STATE_DOWN]      = TA_COLOR_INVIS;
    //ui_default_style[UI_PANEL].bg_color[UI_STATE_ACTIVE]    = TA_COLOR_INVIS;
    //ui_default_style[UI_PANEL].fg_color[UI_STATE_NONE]      = TA_COLOR_INVIS;
    //ui_default_style[UI_PANEL].fg_color[UI_STATE_HOVER]     = TA_COLOR_INVIS;
    //ui_default_style[UI_PANEL].fg_color[UI_STATE_DOWN]      = TA_COLOR_INVIS;
    //ui_default_style[UI_PANEL].fg_color[UI_STATE_ACTIVE]    = TA_COLOR_INVIS;

    ui_default_style[UI_BUTTON].margin                      = TA_RECT(2, 1, 0, 1);
    ui_default_style[UI_BUTTON].pad                         = TA_RECT(4, 4, 4, 4); //TA_RECT(4, 1, 4, 1);
    ui_default_style[UI_BUTTON].bg_color[UI_STATE_NONE]     = TA_RGBA(1.0f, 1.0f, 1.0f, 0.9f);
    ui_default_style[UI_BUTTON].bg_color[UI_STATE_HOVER]    = TA_RGBA(1.0f, 1.0f, 0.0f, 0.9f);
    ui_default_style[UI_BUTTON].bg_color[UI_STATE_DOWN]     = TA_RGBA(1.0f, 1.0f, 0.0f, 0.9f);
    ui_default_style[UI_BUTTON].bg_color[UI_STATE_ACTIVE]   = TA_RGBA(0.0f, 1.0f, 1.0f, 0.9f);
    //ui_default_style[UI_BUTTON].fg_color[UI_STATE_NONE]     = TA_COLOR_INVIS;
    //ui_default_style[UI_BUTTON].fg_color[UI_STATE_HOVER]    = TA_COLOR_INVIS;
    //ui_default_style[UI_BUTTON].fg_color[UI_STATE_DOWN]     = TA_COLOR_INVIS;
    //ui_default_style[UI_BUTTON].fg_color[UI_STATE_ACTIVE]   = TA_COLOR_INVIS;

    ui_default_style[UI_LABEL].margin                       = TA_RECT(2, 1, 0, 1);
    ui_default_style[UI_LABEL].pad                          = TA_RECT(4, 1, 4, 1);
    ui_default_style[UI_LABEL].bg_color[UI_STATE_NONE]      = TA_RGBA(0.2f, 0.2f, 0.4f, 0.9f);
    ui_default_style[UI_LABEL].bg_color[UI_STATE_HOVER]     = TA_RGBA(0.2f, 0.2f, 0.4f, 0.9f);
    ui_default_style[UI_LABEL].bg_color[UI_STATE_DOWN]      = TA_RGBA(0.2f, 0.2f, 0.4f, 0.9f);
    ui_default_style[UI_LABEL].bg_color[UI_STATE_ACTIVE]    = TA_RGBA(0.2f, 0.2f, 0.4f, 0.9f);
    //ui_default_style[UI_LABEL].fg_color[UI_STATE_NONE]      = TA_COLOR_INVIS;
    //ui_default_style[UI_LABEL].fg_color[UI_STATE_HOVER]     = TA_COLOR_INVIS;
    //ui_default_style[UI_LABEL].fg_color[UI_STATE_DOWN]      = TA_COLOR_INVIS;
    //ui_default_style[UI_LABEL].fg_color[UI_STATE_ACTIVE]    = TA_COLOR_INVIS;

    ui_default_style[UI_TEXTBOX].margin                     = TA_RECT(2, 1, 0, 1);
    ui_default_style[UI_TEXTBOX].pad                        = TA_RECT(4, 1, 4, 1);
    ui_default_style[UI_TEXTBOX].bg_color[UI_STATE_NONE]    = TA_RGBA(1.0f, 1.0f, 1.0f, 0.9f);
    ui_default_style[UI_TEXTBOX].bg_color[UI_STATE_HOVER]   = TA_RGBA(1.0f, 1.0f, 0.0f, 0.9f);
    ui_default_style[UI_TEXTBOX].bg_color[UI_STATE_DOWN]    = TA_RGBA(1.0f, 0.0f, 1.0f, 0.9f);
    ui_default_style[UI_TEXTBOX].bg_color[UI_STATE_ACTIVE]  = TA_RGBA(0.0f, 1.0f, 1.0f, 0.9f);
    //ui_default_style[UI_TEXTBOX].fg_color[UI_STATE_NONE]    = TA_COLOR_INVIS;
    //ui_default_style[UI_TEXTBOX].fg_color[UI_STATE_HOVER]   = TA_COLOR_INVIS;
    //ui_default_style[UI_TEXTBOX].fg_color[UI_STATE_DOWN]    = TA_COLOR_INVIS;
    //ui_default_style[UI_TEXTBOX].fg_color[UI_STATE_ACTIVE]  = TA_COLOR_INVIS;
}

#if 0
static ta_rgba ui_random_color(ui_frame *frame, ui_state_type state)
{
    // HACK: Generate random colors based on name of control
    ta_rgba color;
    u32 hash = dlb_hash_code(SYM(frame->name));
    color.r = ((hash >> (state     )) & 255) / 255.0f;
    color.g = ((hash >> (state +  8)) & 255) / 255.0f;
    color.b = ((hash >> (state + 16)) & 255) / 255.0f;
    color.a = 1.0f;
    return color;
}
#endif
static bool type_is_container(ui_frame_type type)
{
    return (type == UI_ROOT || type == UI_WINDOW || type == UI_PANEL);
}
// returns closest parent container index
static ui_frame *ui_container(u32 frame_idx)
{
    DLB_ASSERT(dlb_vec_len(ui_frames));
    DLB_ASSERT(frame_idx <= dlb_vec_len(ui_frames));

    // note: UI_ROOT is its own parent
    if (!frame_idx) {
        return &ui_frames[0];
    }

    // TODO: Could just keep track of most recent container? Idk.. this most
    // likely only runs for 1-2 iterations worst case right now.
    ui_frame *frame = 0;
    for (int i = frame_idx - 1; i >= 0; i--) {
        if (type_is_container(ui_frames[i].type)) {
            frame = &ui_frames[i];
            break;
        }
    }
    DLB_ASSERT(frame);
    DLB_ASSERT(type_is_container(frame->type));
    return frame;
}
static ui_frame *ui_container_last()
{
    return ui_container(dlb_vec_len(ui_frames));
}
static void ui_pop(u32 frame_idx)
{
    DLB_ASSERT(dlb_vec_len(ui_frames));
    DLB_ASSERT(frame_idx <= dlb_vec_len(ui_frames));

    ui_frame *frame = dlb_vec_last(ui_frames);
    u8 found = 0;
    while (frame->index != frame_idx) {
        dlb_vec_popz(ui_frames);
        frame--;
    }
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
    if (MOUSE_X >= rect.x && MOUSE_X < rect.x + rect.w &&
        MOUSE_Y >= rect.y && MOUSE_Y < rect.y + rect.h)
    {
        return true;
    }
    return false;
}
// returns frame index
static u32 ui_frame_start(ui_frame_type type, const char *name)
{
    glDisable(GL_SCISSOR_TEST);

    // Allocate frame
    ui_frame *frame = dlb_vec_alloc(ui_frames);
    frame->index = dlb_vec_len(ui_frames) - 1;
    frame->type = type;
    frame->name = name;

    frame->margin = next_frame_style.margin;
    if (!(frame->margin.x || frame->margin.y ||
          frame->margin.w || frame->margin.h))
    {
        frame->margin = ui_default_style[type].margin;
    }
    frame->pad = next_frame_style.pad;
    if (!(frame->pad.x || frame->pad.y ||
          frame->pad.w || frame->pad.h))
    {
        frame->pad = ui_default_style[type].pad;
    }

    ui_frame *container = ui_container(frame->index);
    frame->content_size.w = frame->pad.x + next_frame_size.w + frame->pad.w;
    frame->content_size.h = frame->pad.y + next_frame_size.h + frame->pad.h;
    frame->rect.x = container->rect.x + container->offset.x + frame->margin.x;
    frame->rect.y = container->rect.y + container->offset.y + frame->margin.y;
    frame->rect.w = frame->content_size.w;
    frame->rect.h = frame->content_size.h;

    ui_state_type state = UI_STATE_NONE;

    if (type_is_container(frame->type)) {
        // Grow container
        frame->offset.x += frame->pad.x;
        frame->offset.y += frame->pad.y;
    } else {
        // Updated frame states
        last_frame_state.hover = false;
        last_frame_state.down = false;
        last_frame_state.pressed = false;
        last_frame_state.released = false;
        last_frame_state.unfocused = false;

        if (!ta_mouse_captured() && rect_contains_mouse(frame->rect))
        {
            state = UI_STATE_HOVER;
            last_frame_state.hover = true;
            if (ta_key_down(SDL_SCANCODE_MOUSE_LEFT)) {
                state = UI_STATE_DOWN;
                last_frame_state.down = true;
                last_frame_state.pressed = ta_key_pressed(SDL_SCANCODE_MOUSE_LEFT);
            } else {
                last_frame_state.released = ta_key_released(SDL_SCANCODE_MOUSE_LEFT);
            }
        } else {
            if (ta_key_down(SDL_SCANCODE_MOUSE_LEFT)) {
                last_frame_state.unfocused = true;
            }
        }
    }

    frame->bg_color = next_frame_style.bg_color[state];
    if (frame->bg_color.a == 0.0f) {
        frame->bg_color = ui_default_style[type].bg_color[state];
    }
    frame->fg_color = next_frame_style.fg_color[state];
    if (frame->fg_color.a == 0.0f) {
        frame->fg_color = ui_default_style[type].fg_color[state];
    }
    frame->invisible = next_frame_style.invisible;

    // TODO: If we're going to render containers we need to defer *all*
    //       rendering to e.g. container->queue until the container pops, then
    //       render everything starting at the container for proper ordering.
    //if (UI_DEBUG_CONTAINERS || !type_is_container(frame->type)) {
    //    ta_primitive_push_rect(bg_rect, bg_color, UI_LAYER_EDIT_1_BG);
    //    ta_primitive_render(true, true);
    //}

    if (frame->type != UI_TEXTBOX && frame->type != UI_LABEL) {
        // TODO: Should clip rect contain pad or not? Not sure.. see how it looks.
        //ta_rect clip_rect = rect_shrink(frame->rect, frame->pad);
        ta_rect clip_rect = frame->rect;
        glEnable(GL_SCISSOR_TEST);
        int inv_y = WINDOW_H - (clip_rect.y + clip_rect.h);
        glScissor(clip_rect.x, inv_y, clip_rect.w, clip_rect.h);
    }

    next_frame_size = TA_SIZE_ZERO;
    dlb_memset(&next_frame_style, 0, sizeof(next_frame_style));

    return frame->index;
}
static void ui_frame_end(u32 frame_idx)
{
    glDisable(GL_SCISSOR_TEST);

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

ta_ui_state ta_ui_last_frame_state()
{
    return last_frame_state;
}
#if 0
ta_vec2i ta_ui_container_absolute_cursor()
{
    ui_frame *container = ui_container_last();
    DLB_ASSERT(container);
    ta_vec2i result = { 0 };
    result.x = container->rect.x + container->offset.x;
    result.y = container->rect.y + container->offset.y;
    return result;
}
#endif
// TODO: Replace these with ta_ui_push_style that persists until pop
void ta_ui_next_margin(int left, int top, int right, int bottom)
{
#if UI_DEBUG_MARGIN
    next_frame_style.margin.x = left;
    next_frame_style.margin.y = top;
    next_frame_style.margin.w = right;
    next_frame_style.margin.h = bottom;
#endif
}
void ta_ui_next_pad(int left, int top, int right, int bottom)
{
#if UI_DEBUG_PAD
    next_frame_style.pad.x = left;
    next_frame_style.pad.y = top;
    next_frame_style.pad.w = right;
    next_frame_style.pad.h = bottom;
#endif
}
void ta_ui_next_size(int w, int h)
{
    next_frame_size.w = w;
    next_frame_size.h = h;
}
void ta_ui_next_invisible()
{
    next_frame_style.invisible = true;
}
void ta_ui_next_bg_color(ui_state_type state, float r, float g, float b, float a)
{
    if (state < UI_STATE_COUNT) {
        next_frame_style.bg_color[state].r = r;
        next_frame_style.bg_color[state].g = g;
        next_frame_style.bg_color[state].b = b;
        next_frame_style.bg_color[state].a = a;
    } else {
        for (int i = 0; i < UI_STATE_COUNT; i++) {
            next_frame_style.bg_color[i].r = r;
            next_frame_style.bg_color[i].g = g;
            next_frame_style.bg_color[i].b = b;
            next_frame_style.bg_color[i].a = a;
        }
    }
}
void ta_ui_next_fg_color(ui_state_type state, float r, float g, float b, float a)
{
    if (state < UI_STATE_COUNT) {
        next_frame_style.fg_color[state].r = r;
        next_frame_style.fg_color[state].g = g;
        next_frame_style.fg_color[state].b = b;
        next_frame_style.fg_color[state].a = a;
    } else {
        for (int i = 0; i < UI_STATE_COUNT; i++) {
            next_frame_style.fg_color[i].r = r;
            next_frame_style.fg_color[i].g = g;
            next_frame_style.fg_color[i].b = b;
            next_frame_style.fg_color[i].a = a;
        }
    }
}

void ta_ui_window_begin(const char *name, int *scroll_v)
{
    glClear(GL_DEPTH_BUFFER_BIT);

    u32 frame_idx = ui_frame_start(UI_WINDOW, name);
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

    dlb_vec_zero(ui_frames);
    // reserve UI_ROOT
    dlb_vec_alloc(ui_frames);
}

void ta_ui_panel_begin(const char *name, u32 *index)
{
    u32 frame_idx = ui_frame_start(UI_PANEL, name);
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

void ta_ui_spacer(int w, int h)
{
    ui_frame *container = ui_container(dlb_vec_len(ui_frames));
    container->offset.x += w;
    container->offset.y += h;
}
void ta_ui_tooltip(const char *text, u32 text_len)
{
    // TODO: Cache tooltips
    ta_rect_uv *text_rects = 0;
    ta_rectf text_rect = ta_font_push_text(&text_rects, tg_game.font,
        text, text_len, true, 0, 0, 0, 0);

    float offset_x = MOUSE_X + 10.0f;
    float offset_y = MOUSE_Y + 20.0f;

    ta_rect_uv tooltip_bg = { 0 };
    tooltip_bg.rect.x = offset_x - 10.0f;
    tooltip_bg.rect.y = offset_y;
    tooltip_bg.rect.w = text_rect.w + 20.0f;
    tooltip_bg.rect.h = text_rect.h;
    ta_primitive_push_rect_uv(&tooltip_bg_queue, tooltip_bg, TA_COLOR_GRAY3A,
        UI_LAYER_TIP_BG, true, false);

    dlb_vec_each(ta_rect_uv *, rect, text_rects) {
        ta_rect_uv offset_rect = *rect;
        offset_rect.rect.x += offset_x;
        offset_rect.rect.y += offset_y;
        ta_primitive_push_rect_uv(&tooltip_fg_queue, offset_rect, TA_COLOR_WHITE,
            UI_LAYER_TIP, true, false);
    }
    dlb_vec_zero(text_rects);
}
void ta_ui_statusbar()
//void ta_ui_statusbar(int x, int y)
{
    const float statusbar_pad = 4;
    ta_rect_uv status_bg = { 0 };
    status_bg.rect.x = statusbar_pad;
    status_bg.rect.y = -(float)(tg_game.font->line_height + statusbar_pad);
    status_bg.rect.w = (float)(WINDOW_W - statusbar_pad * 2);
    status_bg.rect.h = (float)tg_game.font->line_height;
    ta_primitive_push_rect_uv(&tooltip_bg_queue, status_bg, TA_COLOR_GRAY3A,
        UI_LAYER_TIP_BG, true, false);
}
bool ta_ui_button(const char *name, const ta_texture *tex, int face)
{
#if UI_DEBUG_NO_TEXTURES
    tex = 0;
#endif

    if (tex) {
        if (!next_frame_size.w && !next_frame_size.h) {
            ta_ui_next_size(tex->width, tex->height);
        }
    }

    u32 frame_idx = ui_frame_start(UI_BUTTON, name);
    ui_frame *frame = &ui_frames[frame_idx];

    if (!frame->invisible) {
        ta_primitive_push_rect(frame->rect, frame->bg_color, UI_LAYER_EDIT_1_BG);
        ta_primitive_render_quads(quads_queue, tg_shader_quads, true, true);

        if (tex) {
            if (tex->cubemap) {
                DLB_ASSERT(face >= 0 && face <= 6);
                ta_shader_set_sampler_cube(tg_shader_cubemap, SYM_U_TEX, tex->gl_id);
                ta_shader_set_int(tg_shader_cubemap, SYM_U_FACE, face);
                ta_rect img_rect = rect_shrink(frame->rect, frame->pad);
                ta_primitive_push_rect(img_rect, TA_COLOR_INVIS, UI_LAYER_EDIT_1);
                ta_primitive_render_quads(quads_queue, tg_shader_cubemap, true, true);
                ta_shader_set_sampler_cube(tg_shader_cubemap, SYM_U_TEX, 0);
            } else {
                ta_shader_set_mat4(tg_shader_quads, SYM_U_PROJ, &MAT4_IDENT);
                ta_shader_set_mat4(tg_shader_quads, SYM_U_VIEW, &MAT4_IDENT);
                ta_shader_set_mat4(tg_shader_quads, SYM_U_MODEL, &MAT4_IDENT);
                ta_shader_set_sampler2d(tg_shader_quads, SYM_U_TEX, tex->gl_id);
                ta_rect img_rect = rect_shrink(frame->rect, frame->pad);
                ta_primitive_push_rect(img_rect, TA_COLOR_INVIS, UI_LAYER_EDIT_1);
                ta_primitive_render_quads(quads_queue, tg_shader_quads, true, true);
                ta_shader_set_sampler2d(tg_shader_quads, SYM_U_TEX, 0);
            }
        }
    }
    ui_frame_end(frame_idx);

    return last_frame_state.pressed;
}
bool ta_ui_button_toggle(const char *name, const ta_texture *tex, bool *active)
{
#if UI_DEBUG_NO_TEXTURES
    tex = 0;
#endif

    if (tex) {
        if (!next_frame_size.w && !next_frame_size.h) {
            ta_ui_next_size(tex->width, tex->height);
        }
    }

    u32 frame_idx = ui_frame_start(UI_BUTTON, name);
    ui_frame *frame = &ui_frames[frame_idx];

    if (last_frame_state.pressed) {
        *active = !*active;
    }
    ta_rgba bg_color = frame->bg_color;
    if (*active) {
        bg_color = ui_default_style[frame->type].bg_color[UI_STATE_ACTIVE];
    }

    ta_primitive_push_rect(frame->rect, bg_color, UI_LAYER_EDIT_1_BG);
    ta_primitive_render_quads(quads_queue, tg_shader_quads, true, true);

    if (tex) {
        ta_shader_set_mat4(tg_shader_quads, SYM_U_PROJ, &MAT4_IDENT);
        ta_shader_set_mat4(tg_shader_quads, SYM_U_VIEW, &MAT4_IDENT);
        ta_shader_set_mat4(tg_shader_quads, SYM_U_MODEL, &MAT4_IDENT);
        ta_shader_set_sampler2d(tg_shader_quads, SYM_U_TEX, tex->gl_id);
        ta_rect img_rect = rect_shrink(frame->rect, frame->pad);
        ta_primitive_push_rect(img_rect, TA_COLOR_INVIS, UI_LAYER_EDIT_1);
        ta_primitive_render_quads(quads_queue, tg_shader_quads, true, true);
        ta_shader_set_sampler2d(tg_shader_quads, SYM_U_TEX, 0);
    }
    ui_frame_end(frame_idx);

    return *active;
}
bool ta_ui_label(const char *name, const char *text)
{
    DLB_ASSERT(text);

    static ta_rect_uv *text_rects = 0;
    ta_rectf text_rect = ta_font_push_text(&text_rects, tg_game.font, text, 0,
        true, 0, 0, 0, 0);

    // Auto-expand frame based on contents
    next_frame_size.w = MAX(next_frame_size.w, (int)text_rect.w);
    next_frame_size.h = MAX(next_frame_size.h, (int)text_rect.h);

    u32 frame_idx = ui_frame_start(UI_LABEL, name);
    ui_frame *frame = &ui_frames[frame_idx];

    // Render background
    if (frame->bg_color.a == 0.0f) {
        frame->bg_color.a = TA_EPSILON;
    }
    ta_rect bg_rect = frame->rect;
    ta_primitive_push_rect(bg_rect, frame->bg_color, UI_LAYER_EDIT_1_BG);
    ta_primitive_render_quads(quads_queue, tg_shader_quads, true, true);

    float text_left = (float)frame->rect.x + frame->pad.x;
    float text_top = (float)frame->rect.y + frame->pad.y;

    // Render text
    dlb_vec_each(ta_rect_uv *, rect, text_rects) {
        ta_primitive_push_rect_uv(&quads_queue, *rect, TA_COLOR_WHITE, 0, true,
            false);
    }
    dlb_vec_zero(text_rects);
    ta_font_render(quads_queue, tg_game.font, text_left, text_top,
        UI_LAYER_EDIT_1, true, true);

    ui_frame_end(frame_idx);
    return last_frame_state.pressed;
}
bool ta_ui_textbox(const char *name, ta_text_entry *text_entry)
{
    DLB_ASSERT(text_entry);

    static ta_rect_uv *text_rects = 0;
    ta_vec2 cursor = { 0 };
    ta_rectf bounds = ta_text_entry_draw(text_entry, &text_rects, &cursor);

    // Auto-expand frame based on contents
    next_frame_size.w = MAX(next_frame_size.w, (int)bounds.w);
    next_frame_size.h = MAX(next_frame_size.h, (int)bounds.h);

    u32 frame_idx = ui_frame_start(UI_TEXTBOX, name);
    ui_frame *frame = &ui_frames[frame_idx];

    if (last_frame_state.down) {
        // Activate textbox when clicked
        ta_text_entry_focus(text_entry);
    } else if (ta_key_pressed(SDL_SCANCODE_MOUSE_LEFT)) {
        // Deactivate textbox when elsewhere clicked
        ta_text_entry_unfocus(text_entry);
    }

    // Render background
    ta_rgba bg_color = ta_text_entry_focused(text_entry) ? TA_COLOR_BLUE3 : TA_COLOR_BLUE2;
    ta_rect bg_rect = frame->rect;
    ta_primitive_push_rect(bg_rect, bg_color, UI_LAYER_EDIT_1_BG);
    ta_primitive_render(true, false);

    int text_left = frame->rect.x + frame->pad.x;
    int text_top = frame->rect.y + frame->pad.y;

    // Render text
    if (dlb_vec_len(text_rects)) {
        dlb_vec_each(ta_rect_uv *, rect, text_rects) {
            ta_primitive_push_rect_uv(&quads_queue, *rect, TA_COLOR_WHITE, 0,
                true, false);
        }
        dlb_vec_zero(text_rects);
        ta_font_render(quads_queue, tg_game.font, (float)text_left,
            (float)text_top, UI_LAYER_EDIT_1, true, true);
    }

    // If active, render cursor
    if (ta_text_entry_focused(text_entry)) {
        ta_rect cursor_rect = { 0 };
        cursor_rect.x = text_left + (int)cursor.x;
        cursor_rect.y = text_top + (int)cursor.y + 1;
        cursor_rect.w = 1;
        cursor_rect.h = tg_game.font->line_height - 2;
        ta_primitive_push_rect(cursor_rect, TA_COLOR_GRAY8, UI_LAYER_EDIT_2);
        ta_primitive_render(true, false);
    }
    ui_frame_end(frame_idx);
    bool valid = ta_text_entry_valid(text_entry);
    return valid;
}