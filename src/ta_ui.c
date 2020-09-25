#include "ta_ui.h"
#include "ta_primitive.h"
#include "ta_window.h"
#include "ta_shader.h"
#include "ta_symbol.h"
#include "ta_mouse.h"
#include "ta_font.h"
#include "ta_texture.h"
#include "ta_keybind.h"
#include "ta_log.h"
#include "ta_scene.h"
#include "ta_event.h"
#include "ta_keybind.h"
#include "ta_schema.h"
#include "ta_timer.h"
#include "ta_parse.h"
#include "dlb/dlb_vector.h"
#include "dlb/dlb_murmur3.h"
#include "misc/glad.h"

#define UI_DEBUG_PANEL          0
#define UI_DEBUG_NO_TEXTURES    0
#define UI_DEBUG_RANDOM_COLORS  0

#define SCROLL_WIDGET_PAD       1
#define SCROLL_WIDGET_THICKNESS 8
#define SCROLL_WIDGET_H_MIN     4
#define SCROLL_THICKNESS        (SCROLL_WIDGET_THICKNESS + SCROLL_WIDGET_PAD * 2)
#define SCROLL_WHEEL_SPEED      17

// internal flags
#define TA_UI_INVISIBLE         0x20000000  // takes up space but doesn't render (display: hidden)
#define TA_UI_CONTAINER         0x40000000  // [internal] will always be set on containers
#define TA_UI_CONTAINER_ENDED   0x80000000  // [internal] will be set when container ends

#define UI_TEXTBOX_MIN_BUFFER_LEN 128  // minimum buffer to reserve for text editing (to avoid frequent resizes)

static const double key_repeat_delay_ms = 300;
static const double key_repeat_interval_ms = 40;
static const double double_click_interval_ms = 500;

typedef struct ui_style {
    ta_rect margin;
    ta_rect pad;
    ta_rgba bg_color[UI_STATE_COUNT];
    ta_rgba fg_color[UI_STATE_COUNT];
    bool invisible;
} ui_style;

// Flags for each override to make it easier to determine if they were set
static struct {
    bool pos_relative;
    bool size;
    bool margin_x;
    bool margin_y;
    bool margin_w;
    bool margin_h;
    bool pad_x;
    bool pad_y;
    bool pad_w;
    bool pad_h;
    bool bg_color[UI_STATE_COUNT];
    bool fg_color[UI_STATE_COUNT];
    bool invisible;
} next_frame_dirty;

typedef struct ui_frame {
    size_t index;
    size_t container_idx;
    ui_frame_type type;

    ta_rect margin;
    ta_rect pad;
    ta_rgba bg_color[UI_STATE_COUNT];
    ta_rgba fg_color[UI_STATE_COUNT];
    ta_rect rect;              // position & size (-margin, +pad)
    ta_vec2i offset;           // dynamic offset for layout
    bool skip_flow;            // if true, doesn't affect flow of parent container

    int row_height;            // height of current layout row
    bool row_continue;         // if true, next element will layout on same row
    ta_size content_size;      // dynamic content size (-margin, +pad)
    ta_rect clip_rect;

    ta_rect_uv *text_rects;    // vector
    bool text_rects_internal;  // if true, text_rects wasn't passed in and must be freed
    const char *texture;
    ta_vec2i cursor;           // cursor location for textboxes

    // Consolidate this and all bools into a single flags bitmap
    ta_ui_state state;
    ui_state_type state_type;

    // TA_UI_INVISIBLE
    u32 internal_flags;

    union {
        void *ptr;
        ta_ui_window_state *window;
        ta_ui_panel_state *panel;
        ta_ui_textbox_state *textbox;
    } data;
} ui_frame;

// Internal state
static ta_font *ui_font;
static ta_ui_textbox_state **ui_textbox_editing;
static ta_ui_textbox_state **ui_textbox_dragging;
static bool ui_hovered = false;

static ui_style ui_default_style[UI_COUNT] = { 0 };
static ta_vec2i next_frame_pos_relative;
static ta_size next_frame_size;
static ui_style next_frame_style;
static ta_ui_state *last_frame_state;

static ui_frame *ui_frames;

static void ui_row_end(ui_frame *container);

// active_textbox is an external pointer that the ui code will keep updated
// for you automatically when focus changes.
void ta_ui_init(ta_font *font, ta_ui_textbox_state **textbox_editing, ta_ui_textbox_state **textbox_dragging)
{
    DLB_ASSERT(font);
    DLB_ASSERT(textbox_editing);
    DLB_ASSERT(textbox_dragging);

    ui_font = font;
    ui_textbox_editing = textbox_editing;
    ui_textbox_dragging = textbox_dragging;

    // Reserve element zero for UI_ROOT
    dlb_vec_alloc(ui_frames);

    //ui_default_style[UI_WINDOW].margin                      = TA_RECT_ZERO;
    ui_default_style[UI_WINDOW].pad                         = TA_RECT(4, 4, 4, 4);
    ui_default_style[UI_WINDOW].bg_color[UI_STATE_NONE]     = TA_RGBA(0.7f, 0.7f, 0.7f, 0.4f);
    ui_default_style[UI_WINDOW].bg_color[UI_STATE_HOVER]    = TA_RGBA(0.7f, 0.7f, 0.7f, 0.4f);
    ui_default_style[UI_WINDOW].bg_color[UI_STATE_DOWN]     = TA_RGBA(0.7f, 0.7f, 0.7f, 0.4f);
    ui_default_style[UI_WINDOW].bg_color[UI_STATE_ACTIVE]   = TA_RGBA(0.7f, 0.7f, 0.7f, 0.4f);
    //ui_default_style[UI_WINDOW].fg_color[UI_STATE_NONE]     = TA_COLOR_INVIS;
    //ui_default_style[UI_WINDOW].fg_color[UI_STATE_HOVER]    = TA_COLOR_INVIS;
    //ui_default_style[UI_WINDOW].fg_color[UI_STATE_DOWN]     = TA_COLOR_INVIS;
    //ui_default_style[UI_WINDOW].fg_color[UI_STATE_ACTIVE]   = TA_COLOR_INVIS;

    //ui_default_style[UI_PANEL].margin                     = TA_RECT(0, 0, 0, 0);
    ui_default_style[UI_PANEL].pad                          = TA_RECT(4, 4, 4, 4);
    ui_default_style[UI_PANEL].bg_color[UI_STATE_NONE]      = TA_RGBA(0.15f, 0.15f, 0.15f, 1.0f); //TA_COLOR_GRAY2; //TA_RGBA(0.7f, 0.7f, 0.7f, 0.4f);
    ui_default_style[UI_PANEL].bg_color[UI_STATE_HOVER]     = TA_RGBA(0.15f, 0.15f, 0.15f, 1.0f); //TA_COLOR_GRAY2; //TA_RGBA(0.7f, 0.7f, 0.7f, 0.4f);
    ui_default_style[UI_PANEL].bg_color[UI_STATE_DOWN]      = TA_RGBA(0.15f, 0.15f, 0.15f, 1.0f); //TA_COLOR_GRAY2; //TA_RGBA(0.7f, 0.7f, 0.7f, 0.4f);
    ui_default_style[UI_PANEL].bg_color[UI_STATE_ACTIVE]    = TA_RGBA(0.15f, 0.15f, 0.15f, 1.0f); //TA_COLOR_GRAY2; //TA_RGBA(0.7f, 0.7f, 0.7f, 0.4f);
    //ui_default_style[UI_PANEL].fg_color[UI_STATE_NONE]      = TA_COLOR_INVIS;
    //ui_default_style[UI_PANEL].fg_color[UI_STATE_HOVER]     = TA_COLOR_INVIS;
    //ui_default_style[UI_PANEL].fg_color[UI_STATE_DOWN]      = TA_COLOR_INVIS;
    //ui_default_style[UI_PANEL].fg_color[UI_STATE_ACTIVE]    = TA_COLOR_INVIS;

    ui_default_style[UI_BUTTON].margin                      = TA_RECT(2, 1, 0, 1);
    ui_default_style[UI_BUTTON].pad                         = TA_RECT(4, 1, 4, 1);
    ui_default_style[UI_BUTTON].bg_color[UI_STATE_NONE]     = TA_RGBA(0.2f, 0.6f, 1.0f, 0.9f);
    ui_default_style[UI_BUTTON].bg_color[UI_STATE_HOVER]    = TA_RGBA(1.0f, 1.0f, 0.0f, 0.9f);
    ui_default_style[UI_BUTTON].bg_color[UI_STATE_DOWN]     = TA_RGBA(0.5f, 0.5f, 0.0f, 0.9f);
    ui_default_style[UI_BUTTON].bg_color[UI_STATE_ACTIVE]   = TA_RGBA(0.0f, 1.0f, 1.0f, 0.9f);
    //ui_default_style[UI_BUTTON].fg_color[UI_STATE_NONE]     = TA_COLOR_INVIS;
    //ui_default_style[UI_BUTTON].fg_color[UI_STATE_HOVER]    = TA_COLOR_INVIS;
    //ui_default_style[UI_BUTTON].fg_color[UI_STATE_DOWN]     = TA_COLOR_INVIS;
    //ui_default_style[UI_BUTTON].fg_color[UI_STATE_ACTIVE]   = TA_COLOR_INVIS;

    ui_default_style[UI_TOGGLE_BUTTON].margin                      = TA_RECT(2, 1, 0, 1);
    ui_default_style[UI_TOGGLE_BUTTON].pad                         = TA_RECT(4, 1, 4, 1);
    ui_default_style[UI_TOGGLE_BUTTON].bg_color[UI_STATE_NONE]     = TA_RGBA(0.4f, 0.2f, 1.0f, 0.9f);
    ui_default_style[UI_TOGGLE_BUTTON].bg_color[UI_STATE_HOVER]    = TA_RGBA(1.0f, 1.0f, 0.0f, 0.9f);
    ui_default_style[UI_TOGGLE_BUTTON].bg_color[UI_STATE_DOWN]     = TA_RGBA(0.5f, 0.5f, 0.0f, 0.9f);
    ui_default_style[UI_TOGGLE_BUTTON].bg_color[UI_STATE_ACTIVE]   = TA_RGBA(0.8f, 0.5f, 1.0f, 0.9f);
    //ui_default_style[UI_TOGGLE_BUTTON].fg_color[UI_STATE_NONE]     = TA_COLOR_INVIS;
    //ui_default_style[UI_TOGGLE_BUTTON].fg_color[UI_STATE_HOVER]    = TA_COLOR_INVIS;
    //ui_default_style[UI_TOGGLE_BUTTON].fg_color[UI_STATE_DOWN]     = TA_COLOR_INVIS;
    //ui_default_style[UI_TOGGLE_BUTTON].fg_color[UI_STATE_ACTIVE]   = TA_COLOR_INVIS;

    //ui_default_style[UI_IMAGE].margin                      = TA_RECT(0, 0, 0, 0);
    //ui_default_style[UI_IMAGE].pad                         = TA_RECT(0, 0, 0, 0);
    //ui_default_style[UI_IMAGE].bg_color[UI_STATE_NONE]     = TA_COLOR_INVIS;
    //ui_default_style[UI_IMAGE].bg_color[UI_STATE_HOVER]    = TA_COLOR_INVIS;
    //ui_default_style[UI_IMAGE].bg_color[UI_STATE_DOWN]     = TA_COLOR_INVIS;
    //ui_default_style[UI_IMAGE].bg_color[UI_STATE_ACTIVE]   = TA_COLOR_INVIS;
    //ui_default_style[UI_IMAGE].fg_color[UI_STATE_NONE]     = TA_COLOR_INVIS;
    //ui_default_style[UI_IMAGE].fg_color[UI_STATE_HOVER]    = TA_COLOR_INVIS;
    //ui_default_style[UI_IMAGE].fg_color[UI_STATE_DOWN]     = TA_COLOR_INVIS;
    //ui_default_style[UI_IMAGE].fg_color[UI_STATE_ACTIVE]   = TA_COLOR_INVIS;

    // dark blue color: TA_RGBA(0.2f, 0.2f, 0.4f, 0.9f)

    ui_default_style[UI_LABEL].margin                       = TA_RECT(2, 1, 0, 1);
    ui_default_style[UI_LABEL].pad                          = TA_RECT(4, 1, 4, 1);
    ui_default_style[UI_LABEL].bg_color[UI_STATE_NONE]      = TA_COLOR_INVIS; //TA_COLOR_BLUE5;
    ui_default_style[UI_LABEL].bg_color[UI_STATE_HOVER]     = TA_COLOR_INVIS; //TA_COLOR_BLUE5;
    ui_default_style[UI_LABEL].bg_color[UI_STATE_DOWN]      = TA_COLOR_INVIS; //TA_COLOR_BLUE5;
    ui_default_style[UI_LABEL].bg_color[UI_STATE_ACTIVE]    = TA_COLOR_INVIS; //TA_COLOR_BLUE5;
    //ui_default_style[UI_LABEL].fg_color[UI_STATE_NONE]      = TA_COLOR_INVIS;
    //ui_default_style[UI_LABEL].fg_color[UI_STATE_HOVER]     = TA_COLOR_INVIS;
    //ui_default_style[UI_LABEL].fg_color[UI_STATE_DOWN]      = TA_COLOR_INVIS;
    //ui_default_style[UI_LABEL].fg_color[UI_STATE_ACTIVE]    = TA_COLOR_INVIS;

    ui_default_style[UI_TEXTBOX].margin                     = TA_RECT(2, 1, 0, 1);
    ui_default_style[UI_TEXTBOX].pad                        = TA_RECT(4, 1, 4, 1);
    ui_default_style[UI_TEXTBOX].bg_color[UI_STATE_NONE]    = TA_COLOR_GRAY4;
    ui_default_style[UI_TEXTBOX].bg_color[UI_STATE_HOVER]   = TA_COLOR_ORANGE;
    ui_default_style[UI_TEXTBOX].bg_color[UI_STATE_DOWN]    = TA_COLOR_ORANGE;
    ui_default_style[UI_TEXTBOX].bg_color[UI_STATE_ACTIVE]  = TA_RGBA(0.0f, 0.5f, 0.45f, 0.9f);
    //ui_default_style[UI_TEXTBOX].fg_color[UI_STATE_NONE]    = TA_COLOR_INVIS;
    //ui_default_style[UI_TEXTBOX].fg_color[UI_STATE_HOVER]   = TA_COLOR_INVIS;
    //ui_default_style[UI_TEXTBOX].fg_color[UI_STATE_DOWN]    = TA_COLOR_INVIS;
    //ui_default_style[UI_TEXTBOX].fg_color[UI_STATE_ACTIVE]  = TA_COLOR_INVIS;
}
void ta_ui_set_font(ta_font *font)
{
    DLB_ASSERT(font);
    ui_font = font;
}
void ta_ui_set_cursor(ta_cursor_type cursor_type)
{
    ta_window_request_cursor(tg_window, cursor_type);
}
void ta_ui_flags_reset()
{
    ui_hovered = false;
}
bool ta_ui_flag_hovered()
{
    return ui_hovered;
}
#if 1
static ta_rgba ui_random_color(size_t frame_idx, ui_state_type state)
{
#if 1
    int seed = state;
#else
    int seed = ui_frames[frame_idx].type;
    if (seed == UI_PANEL) {
        seed += UI_COUNT * state;
    }
#endif
    // HACK: Generate random colors based on index of control
    ta_rgba color;
    u32 hash = dlb_murmur3(&frame_idx, (u32)sizeof(frame_idx));
    color.r = ((hash >> (seed     )) & 255) / 255.0f;
    color.g = ((hash >> (seed +  8)) & 255) / 255.0f;
    color.b = ((hash >> (seed + 16)) & 255) / 255.0f;
    color.a = 1.0f;
    return color;
}
#endif
// returns closest parent container index
static size_t ui_container(size_t frame_idx)
{
    DLB_ASSERT(dlb_vec_len(ui_frames));
    DLB_ASSERT(frame_idx <= dlb_vec_len(ui_frames));

    size_t idx = 0;

    // note: UI_ROOT is its own parent
    if (frame_idx) {
        // TODO: Could just keep track of most recent container? Idk.. this most
        // likely only runs for 1-2 iterations worst case right now.
        for (size_t i = frame_idx; i > 0;) {
            i--;
            if (ui_frames[i].internal_flags & TA_UI_CONTAINER &&
                !(ui_frames[i].internal_flags & TA_UI_CONTAINER_ENDED))
            {
                idx = i;
                break;
            }
        }
    }

    DLB_ASSERT(idx < dlb_vec_len(ui_frames));
    return idx;
}
static inline ui_frame *ui_container_last()
{
    size_t container_idx = ui_container(dlb_vec_len(ui_frames));
    return &ui_frames[container_idx];
}
static void ui_pop(size_t frame_idx)
{
    DLB_ASSERT(dlb_vec_len(ui_frames));
    DLB_ASSERT(frame_idx <= dlb_vec_len(ui_frames));

    ui_frame *frame = dlb_vec_last(ui_frames);
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
static ta_rect rect_intersection(ta_rect a, ta_rect b)
{
    ta_rect result = { 0 };
    result.x = MAX(a.x, b.x);
    result.y = MAX(a.y, b.y);
    result.w = MAX(0, MIN(a.x + a.w, b.x + b.w) - result.x);
    result.h = MAX(0, MIN(a.y + a.h, b.y + b.h) - result.y);
    return result;
}
static bool rect_intersects(ta_rect a, ta_rect b)
{
    bool result = false;
    if (a.x + a.w > b.x &&
        a.y + a.h > b.y &&
        b.x + b.w > a.x &&
        b.y + b.h > a.y)
    {
        result = true;
    }
    return result;
}
static bool rect_contains_mouse(ta_rect rect)
{
    int x = ta_mouse_x();
    int y = ta_mouse_y();
    if (!ta_mouse_captured() && !ta_mouse_dragging() &&
        x >= rect.x && x < rect.x + rect.w &&
        y >= rect.y && y < rect.y + rect.h)
    {
        return true;
    }
    return false;
}
static ta_ui_scroll_state *ui_scroll_state(ui_frame *frame)
{
    ta_ui_scroll_state *scroll = 0;
    switch (frame->type) {
        case UI_WINDOW: {
            scroll = &frame->data.window->scroll;
            break;
        } case UI_PANEL: {
            scroll = &frame->data.panel->scroll;
            break;
        } case UI_TEXTBOX: {
            scroll = &frame->data.textbox->scroll;
            break;
        } default: {
            break;
        }
    }
    return scroll;
}
static void ui_frame_begin(ui_frame_type type, void *data, u32 flags)
{
    // Allocate frame
    ui_frame *frame = dlb_vec_alloc(ui_frames);
    frame->index = dlb_vec_len(ui_frames) - 1;
    frame->type = type;
    frame->data.ptr = data;
    frame->internal_flags = flags;

    frame->margin.x = next_frame_dirty.margin_x ? next_frame_style.margin.x : ui_default_style[type].margin.x;
    frame->margin.y = next_frame_dirty.margin_y ? next_frame_style.margin.y : ui_default_style[type].margin.y;
    frame->margin.w = next_frame_dirty.margin_w ? next_frame_style.margin.w : ui_default_style[type].margin.w;
    frame->margin.h = next_frame_dirty.margin_h ? next_frame_style.margin.h : ui_default_style[type].margin.h;
    frame->pad.x = next_frame_dirty.pad_x ? next_frame_style.pad.x : ui_default_style[type].pad.x;
    frame->pad.y = next_frame_dirty.pad_y ? next_frame_style.pad.y : ui_default_style[type].pad.y;
    frame->pad.w = next_frame_dirty.pad_w ? next_frame_style.pad.w : ui_default_style[type].pad.w;
    frame->pad.h = next_frame_dirty.pad_h ? next_frame_style.pad.h : ui_default_style[type].pad.h;

    for (ui_state_type state = 0; state < UI_STATE_COUNT; state++) {
        frame->bg_color[state] = next_frame_dirty.bg_color[state]
            ? next_frame_style.bg_color[state]
            : ui_default_style[type].bg_color[state];
        frame->fg_color[state] = next_frame_dirty.fg_color[state]
            ? next_frame_style.fg_color[state]
            : ui_default_style[type].fg_color[state];
    }

    frame->container_idx = ui_container(frame->index);
    ui_frame *container = &ui_frames[frame->container_idx];

    ta_vec2i offset = container->offset;
    if (next_frame_dirty.pos_relative) {
        offset = next_frame_pos_relative;
        frame->skip_flow = next_frame_dirty.pos_relative;
    }

    frame->rect.x = container->rect.x + offset.x + frame->margin.x;
    frame->rect.y = container->rect.y + offset.y + frame->margin.y;
    frame->rect.w = frame->pad.x + frame->pad.w;
    frame->rect.h = frame->pad.y + frame->pad.h;
    frame->content_size.w = frame->rect.w;
    frame->content_size.h = frame->rect.h;

    ta_ui_scroll_state *scroll = ui_scroll_state(container);
    if (scroll) {
        frame->rect.y -= scroll->pixels.y;
    }

    if (next_frame_dirty.size) {
        frame->rect.w = frame->pad.x + next_frame_size.w + frame->pad.w;
        frame->rect.h = frame->pad.y + next_frame_size.h + frame->pad.h;
    }

    // Pad container offsets
    if (frame->internal_flags & TA_UI_CONTAINER) {
        frame->offset.x += frame->pad.x;
        frame->offset.y += frame->pad.y;
    }

    if (next_frame_dirty.invisible) {
        frame->internal_flags |= TA_UI_INVISIBLE;
    }

    next_frame_pos_relative = TA_VEC2I_ZERO;
    next_frame_size = TA_SIZE_ZERO;
    dlb_memset(&next_frame_style, 0, sizeof(next_frame_style));
    dlb_memset(&next_frame_dirty, 0, sizeof(next_frame_dirty));
}
static ui_frame *ui_frame_end(ui_frame_type type)
{
    ui_frame *frame = 0;
    for (ui_frame *frm = dlb_vec_last(ui_frames); frm != ui_frames; --frm) {
        if (frm->type == type &&
            !(frm->internal_flags & TA_UI_CONTAINER_ENDED))
        {
            frame = frm;
            break;
        }
    }
    DLB_ASSERT(frame);  // If someone calls _end before _begin this will assert

    // Finalize containers
    if (frame->internal_flags & TA_UI_CONTAINER) {
        ui_row_end(frame);
        if (frame->internal_flags & TA_UI_AUTOSIZE_W) {
            frame->rect.w = MAX(frame->rect.w, frame->content_size.w);
        }
        if (frame->internal_flags & TA_UI_AUTOSIZE_H) {
            frame->rect.h = MAX(frame->rect.h, frame->content_size.h);
        }
        frame->internal_flags |= TA_UI_CONTAINER_ENDED;

        // Reserve room for scrollbar(s)
        // NOTE: This will grow containers without auto-size as well.. oh well
        // for now. Fix if it's ever actually a problem.
        if (frame->content_size.h > frame->rect.h) {
            frame->rect.w += SCROLL_THICKNESS;
        }
        if (frame->content_size.w > frame->rect.w) {
            frame->rect.h += SCROLL_THICKNESS;
        }

        if (frame->type == UI_WINDOW) {
            frame->rect.w = MIN(frame->rect.w, WINDOW_W - frame->rect.x);
            frame->rect.h = MIN(frame->rect.h, WINDOW_H - frame->rect.y);
        }
    }

    // NOTE: Setting offset manually is like position: relative. May want to
    // have a position: absolute that also doesn't affect flow.
    if (!frame->skip_flow) {
        ui_frame *container = &ui_frames[frame->container_idx];
        int frame_w = frame->margin.x + frame->rect.w + frame->margin.w;
        int frame_h = frame->margin.y + frame->rect.h + frame->margin.h;
        container->offset.x += frame_w;
        container->row_height = MAX(container->row_height, frame_h);

        if (!container->row_continue) {
            ui_row_end(container);
        }
    }

    // Updated frame states
    frame->state_type = UI_STATE_NONE;
    frame->state.hover = false;
    frame->state.down = false;
    frame->state.pressed = false;
    frame->state.released = false;
    if (rect_contains_mouse(frame->rect))
    {
        frame->state_type = UI_STATE_HOVER;
        frame->state.hover = true;
        ui_hovered = true;
        if (ta_key_down(SDL_SCANCODE_MOUSE_LEFT)) {
            frame->state_type = UI_STATE_DOWN;
            frame->state.down = true;
            frame->state.pressed = ta_key_pressed(SDL_SCANCODE_MOUSE_LEFT);
        } else {
            frame->state.released = ta_key_released(SDL_SCANCODE_MOUSE_LEFT);
        }
    }
    last_frame_state = &frame->state;

#if UI_DEBUG_RANDOM_COLORS
    frame->bg_color = ui_random_color(frame->index, state);
#endif

    return frame;
}
static ta_rect ui_frame_client_area(ui_frame *frame)
{
    DLB_ASSERT(frame);
    ta_rect client_area = { 0 };
    client_area.x = frame->rect.x + frame->pad.x;
    client_area.y = frame->rect.y + frame->pad.y;
    client_area.w = frame->rect.w - frame->pad.x - frame->pad.w;
    client_area.h = frame->rect.h - frame->pad.y - frame->pad.h;
    return client_area;
}

// TODO: Replace these with ta_ui_push_style that persists until pop
void ta_ui_next_margin_left(int margin)
{
    next_frame_style.margin.x = margin;
    next_frame_dirty.margin_x = true;
}
void ta_ui_next_margin_top(int margin)
{
    next_frame_style.margin.y = margin;
    next_frame_dirty.margin_y = true;
}
void ta_ui_next_margin_right(int margin)
{
    next_frame_style.margin.w = margin;
    next_frame_dirty.margin_w = true;
}
void ta_ui_next_margin_bottom(int margin)
{
    next_frame_style.margin.h = margin;
    next_frame_dirty.margin_h = true;
}
void ta_ui_next_margin(int left, int top, int right, int bottom)
{
    next_frame_style.margin.x = left;
    next_frame_style.margin.y = top;
    next_frame_style.margin.w = right;
    next_frame_style.margin.h = bottom;
    next_frame_dirty.margin_x = true;
    next_frame_dirty.margin_y = true;
    next_frame_dirty.margin_w = true;
    next_frame_dirty.margin_h = true;
}
void ta_ui_next_pad_left(int pad)
{
    next_frame_style.pad.x = pad;
    next_frame_dirty.pad_x = true;
}
void ta_ui_next_pad_top(int pad)
{
    next_frame_style.pad.y = pad;
    next_frame_dirty.pad_y = true;
}
void ta_ui_next_pad_right(int pad)
{
    next_frame_style.pad.w = pad;
    next_frame_dirty.pad_w = true;
}
void ta_ui_next_pad_bottom(int pad)
{
    next_frame_style.pad.h = pad;
    next_frame_dirty.pad_h = true;
}
void ta_ui_next_pad(int left, int top, int right, int bottom)
{
    next_frame_style.pad.x = left;
    next_frame_style.pad.y = top;
    next_frame_style.pad.w = right;
    next_frame_style.pad.h = bottom;
    next_frame_dirty.pad_x = true;
    next_frame_dirty.pad_y = true;
    next_frame_dirty.pad_w = true;
    next_frame_dirty.pad_h = true;
}
void ta_ui_next_offset(int x, int y)
{
    next_frame_pos_relative.x = x;
    next_frame_pos_relative.y = y;
    next_frame_dirty.pos_relative = true;
}
void ta_ui_next_size(int w, int h)
{
    next_frame_size.w = w;
    next_frame_size.h = h;
    next_frame_dirty.size = true;
}
void ta_ui_next_invisible()
{
    next_frame_style.invisible = true;
    next_frame_dirty.invisible = true;
}
void ta_ui_next_bg_color(ui_state_type state, float r, float g, float b, float a)
{
    for (int i = 0; i < UI_STATE_COUNT; i++) {
        if ((i == state) ||
            (state == UI_STATE_INTERACT && i != UI_STATE_NONE) ||
            (state == UI_STATE_ALL))
        {
            next_frame_style.bg_color[i].r = r;
            next_frame_style.bg_color[i].g = g;
            next_frame_style.bg_color[i].b = b;
            next_frame_style.bg_color[i].a = a;
            next_frame_dirty.bg_color[i] = true;
        }
    }
}
void ta_ui_next_fg_color(ui_state_type state, float r, float g, float b, float a)
{
    for (int i = 0; i < UI_STATE_COUNT; i++) {
        if ((i == state) ||
            (state == UI_STATE_INTERACT && i != UI_STATE_NONE) ||
            (state == UI_STATE_ALL))
        {
            next_frame_style.fg_color[i].r = r;
            next_frame_style.fg_color[i].g = g;
            next_frame_style.fg_color[i].b = b;
            next_frame_style.fg_color[i].a = a;
            next_frame_dirty.fg_color[i] = true;
        }
    }
}

ta_ui_state ta_ui_last_state()
{
    ta_ui_state state = { 0 };
    if (last_frame_state) {
        state = *last_frame_state;
    }
    return state;
}
static void ui_row_end(ui_frame *container)
{
    // Update content size
    container->offset.y += container->row_height;
    container->content_size.w = MAX(container->content_size.w, container->offset.x + container->pad.w);
    container->content_size.h = MAX(container->content_size.h, container->offset.y + container->pad.h);

    // Start a new row
    container->offset.x = container->pad.x;
    container->row_height = 0;
    container->row_continue = false;
}
void ta_ui_row_begin()
{
    ui_frame *container = ui_container_last();
    ui_row_end(container);
    container->row_continue = true;
}
void ta_ui_row_end()
{
    ui_frame *container = ui_container_last();
    ui_row_end(container);
}
void ta_ui_spacer(int w, int h)
{
    ui_frame *container = ui_container_last();
    container->offset.x += w;
    container->offset.y += h;
}
void ta_ui_window_begin(ta_ui_window_state *window, u32 flags)
{
    DLB_ASSERT(window);
    ui_frame_begin(UI_WINDOW, window, flags | TA_UI_CONTAINER);
}
void ta_ui_window_end()
{
    ui_frame_end(UI_WINDOW);
}
void ta_ui_panel_begin(ta_ui_panel_state *panel, u32 flags)
{
    DLB_ASSERT(panel);
    ui_frame_begin(UI_PANEL, panel, flags | TA_UI_CONTAINER);
}
void ta_ui_panel_end()
{
#if UI_DEBUG_PANEL
    ui_frame *frame = ui_frame_end(UI_PANEL);
    if (frame->state.hover) {
        char tex_buf[512] = { 0 };
        size_t len = snprintf(tex_buf, sizeof(tex_buf),
            "\n"
            "name: %s\n"
            "            %4d\n"
            "margin: %4d    %4d\n"
            "            %4d\n"
            "\n"
            "            %4d\n"
            "   pad: %4d    %4d\n"
            "            %4d\n"
            "\n"
            "            %4d\n"
            "  rect: %4d    %4d\n"
            "            %4d\n"
            "content_size: %4d, %4d",
            frame->name,
            frame->margin.y, frame->margin.x, frame->margin.w, frame->margin.h,
            frame->pad.y, frame->pad.x, frame->pad.w, frame->pad.h,
            frame->rect.x, frame->rect.y, frame->rect.w, frame->rect.h,
            frame->content_size.w, frame->content_size.h
        );
        DLB_ASSERT(len < sizeof(tex_buf));
        ta_ui_tooltip(tex_buf, len);
    }
#else
    ui_frame_end(UI_PANEL);
#endif
}
void ta_ui_button_begin(u32 flags)
{
    ui_frame_begin(UI_BUTTON, 0, flags | TA_UI_CONTAINER);
}
bool ta_ui_button_end()
{
    ui_frame *frame = ui_frame_end(UI_BUTTON);
    return frame->state.pressed;
}
bool ta_ui_button(const char *text, size_t text_len)
{
    ta_ui_next_pad(0, 0, 0, 0);
    ta_ui_button_begin(TA_UI_AUTOSIZE);
    if (text) {
        ta_ui_next_margin(0, 0, 0, 0);
        ta_ui_label(text, text_len, 0);
    }
    return ta_ui_button_end();
}
bool ta_ui_reset_button()
{
    ta_rgba c = TA_COLOR_DARK_RED;
    ta_ui_next_bg_color(UI_STATE_NONE, c.r, c.g, c.b, c.a);
    ta_ui_next_bg_color(UI_STATE_INTERACT, 0.8f, 0.0f, 0.0f, 0.9f);
    //ta_ui_next_bg_color(UI_STATE_NONE, 0.5f, 0.0f, 0.0f, 1.0f);
    //ta_ui_next_bg_color(UI_STATE_INTERACT, 0.7f, 0.0f, 0.0f, 1.0f);
    return ta_ui_button(CSTR("Reset"));
}
void ta_ui_toggle_button_begin(u32 flags)
{
    ui_frame_begin(UI_TOGGLE_BUTTON, 0, flags | TA_UI_CONTAINER);
}
bool ta_ui_toggle_button_end(bool *checked)
{
    DLB_ASSERT(checked);
    ui_frame *frame = ui_frame_end(UI_TOGGLE_BUTTON);
    if (frame->state.pressed) {
        *checked = !*checked;
    }
    if (*checked) {
        frame->state_type = UI_STATE_ACTIVE;
    }
    return frame->state.pressed;
}
bool ta_ui_toggle_button(const char *false_text, size_t false_text_len, const char *true_text, size_t true_text_len,
    bool *checked)
{
    //ta_ui_next_pad(0, 0, 0, 0);
    ta_ui_toggle_button_begin(TA_UI_AUTOSIZE);
    ta_ui_next_margin(0, 0, 0, 0);
    ta_ui_next_pad(0, 0, 0, 0);
    // NOTE: 1 frame delay because label size is required to do "pressed" hit test
    if (*checked) {
        ta_ui_label(true_text, true_text_len, 0);
    } else {
        ta_ui_label(false_text, false_text_len, 0);
    }
    return ta_ui_toggle_button_end(checked);
}
bool ta_ui_image(const char *texture)
{
#if UI_DEBUG_NO_TEXTURES
    tex = 0;
#endif

    if (texture) {
        if (!next_frame_size.w && !next_frame_size.h) {
            ta_texture *tex = ta_game_by_sym(RES_TEXTURE, texture);
            ta_ui_next_size(tex->width, tex->height);
        }
    }

    ui_frame_begin(UI_IMAGE, 0, false);
    ui_frame *frame = ui_frame_end(UI_IMAGE);
    frame->texture = texture;
    return frame->state.pressed;
}
void ta_ui_label(const char *text, size_t text_len, ta_rect_uv **text_rects)
{
    if (!text || !text_len) return;

    ta_rect_uv *text_rects_internal = 0;
    if (text_rects) {
        text_rects_internal = *text_rects;
    }

    ta_rect text_rect = ta_font_push_text(ui_font, text, text_len, true, 0, 0, 0, &text_rects_internal);

    // Auto-expand frame based on contents
    ta_ui_next_size(MAX(next_frame_size.w, (int)text_rect.w),
                    MAX(next_frame_size.h, (int)text_rect.h));

    ui_frame_begin(UI_LABEL, 0, false);
    ui_frame *frame = ui_frame_end(UI_LABEL);
    frame->text_rects = text_rects_internal;
    frame->text_rects_internal = !text_rects;

    if (text_rects) {
        *text_rects = text_rects_internal;
    }
}
void ta_ui_label_float(float value, ta_rect_uv **text_rects)
{
    char text[16] = { 0 };
    size_t text_len = snprintf(CSTR(text), "%.3f", value);
    DLB_ASSERT(text_len < sizeof(text));
    ta_ui_label(text, text_len, text_rects);
}

// TODO: Move this to the keybind and support it more generally
#if 0
static bool textbox_repeat_valid(double *last_time, int *repeat, ta_command command)
{
    DLB_ASSERT(last_time);
    DLB_ASSERT(repeat);

    UNUSED(command);

    double timer_ms = ta_timer_elapsed_ms();
    double delta_ms = timer_ms - *last_time;

    // HACK: Figure this out properly..
    //bool first = keybind->triggered && keybind->changed;
    if (*repeat > 1 && delta_ms > key_repeat_interval_ms * 2) {
        *repeat = 0;
    }

    if ((*repeat == 0) ||
        (*repeat == 1 && delta_ms >= key_repeat_delay_ms) ||
        (*repeat > 1 && delta_ms >= key_repeat_interval_ms))
    {
        *last_time = timer_ms;
        if (*repeat <= 1) {
            (*repeat)++;
        }
        return true;
    }

    return false;
}
#endif
static bool textbox_filter_default(char c)
{
    if ((c >= ui_font->first_char && c <= ui_font->last_char) || c == '\n') {
        return true;
    }
    return false;
}
void textbox_command_cursor_right()
{
    DLB_ASSERT(ui_textbox_editing && *ui_textbox_editing);
    ta_ui_textbox_state *textbox = *ui_textbox_editing;

    size_t len = dlb_vec_len(textbox->buffer);
    if (textbox->cursor < len) {
        textbox->cursor++;
    }
}
void textbox_command_cursor_left()
{
    DLB_ASSERT(ui_textbox_editing && *ui_textbox_editing);
    ta_ui_textbox_state *textbox = *ui_textbox_editing;

    if (textbox->cursor) {
        textbox->cursor--;
    }
}
void textbox_command_cursor_down()
{
    DLB_ASSERT(ui_textbox_editing && *ui_textbox_editing);
    ta_ui_textbox_state *textbox = *ui_textbox_editing;

    //TODO: Move cursor up
    UNUSED(textbox);
}
void textbox_command_cursor_up()
{
    DLB_ASSERT(ui_textbox_editing && *ui_textbox_editing);
    ta_ui_textbox_state *textbox = *ui_textbox_editing;

    //TODO: Move cursor down
    UNUSED(textbox);
}
void textbox_command_cursor_bol()
{
    DLB_ASSERT(ui_textbox_editing && *ui_textbox_editing);
    ta_ui_textbox_state *textbox = *ui_textbox_editing;

    while (textbox->cursor && textbox->buffer[textbox->cursor - 1] != '\n') {
        textbox->cursor--;
    }
}
void textbox_command_cursor_eol()
{
    DLB_ASSERT(ui_textbox_editing && *ui_textbox_editing);
    ta_ui_textbox_state *textbox = *ui_textbox_editing;

    size_t len = dlb_vec_len(textbox->buffer);
    while (textbox->cursor < len && textbox->buffer[textbox->cursor + 1] != '\n') {
        textbox->cursor++;
    }
}
void textbox_command_cursor_bof()
{
    DLB_ASSERT(ui_textbox_editing && *ui_textbox_editing);
    ta_ui_textbox_state *textbox = *ui_textbox_editing;

    textbox->cursor = 0;
}
void textbox_command_cursor_eof()
{
    DLB_ASSERT(ui_textbox_editing && *ui_textbox_editing);
    ta_ui_textbox_state *textbox = *ui_textbox_editing;

    size_t len = dlb_vec_len(textbox->buffer);
    textbox->cursor = len;
}
static void textbox_delete(ta_ui_textbox_state *textbox)
{
    // TODO: dlb_vec_remove_at
    size_t len = dlb_vec_len(textbox->buffer);
    if (textbox->cursor < len) {
        dlb_memcpy(
            textbox->buffer + textbox->cursor,
            textbox->buffer + textbox->cursor + 1,
            len - 1 - textbox->cursor
        );
        textbox->buffer[len - 1] = 0;
        dlb_vec_hdr(textbox->buffer)->len--;
    }
}
void textbox_command_delete()
{
    DLB_ASSERT(ui_textbox_editing && *ui_textbox_editing);
    ta_ui_textbox_state *textbox = *ui_textbox_editing;

    textbox_delete(textbox);
}
void textbox_command_backspace()
{
    DLB_ASSERT(ui_textbox_editing && *ui_textbox_editing);
    ta_ui_textbox_state *textbox = *ui_textbox_editing;

    if (textbox->cursor) {
        textbox->cursor--;
        textbox_delete(textbox);
    }
}
static void textbox_focus(ta_ui_textbox_state *textbox)
{
    *ui_textbox_editing = textbox;
    textbox->focus_changed = !textbox->focused;
    textbox->focused = true;
}
static void textbox_unfocus(ta_ui_textbox_state *textbox)
{
    if (*ui_textbox_editing == textbox) {
        *ui_textbox_editing = 0;
    }
    textbox->focus_changed = textbox->focused;
    textbox->focused = false;  // User clicked elsewhere
    textbox->cursor = 0;
    textbox->selection_start = 0;
    textbox->selection_len = 0;
}
static void textbox_clear(ta_ui_textbox_state *textbox)
{
    textbox->submit = false;
    dlb_vec_free(textbox->buffer);
    textbox->cursor = 0;
    textbox->selection_start = 0;
    textbox->selection_len = 0;
}
static void textbox_submit(ta_ui_textbox_state *textbox)
{
    textbox->submit = true;
    // TODO: Unfocus and free on client's request, after they've been able to
    // use the buffer contents for whatever they need.
    // NOTE: Don't want to unfocus console after entering a command!
    //textbox_unfocus(textbox);
}
void textbox_command_submit()
{
    DLB_ASSERT(ui_textbox_editing && *ui_textbox_editing);
    ta_ui_textbox_state *textbox = *ui_textbox_editing;

    textbox_submit(textbox);
}
static void textbox_cancel(ta_ui_textbox_state *textbox)
{
    textbox->submit = false;
    dlb_vec_free(textbox->buffer);
    textbox_unfocus(textbox);
}
void textbox_command_cancel()
{
    DLB_ASSERT(ui_textbox_editing && *ui_textbox_editing);
    ta_ui_textbox_state *textbox = *ui_textbox_editing;
    textbox_cancel(textbox);
}

ta_textbox_filter *ta_textbox_filter_default = &textbox_filter_default;

// TODO: Run filter on input string.. maybe?
static void textbox_set_text(ta_ui_textbox_state *textbox, const char *text, size_t text_len)
{
    if (textbox->buffer) {
        dlb_vec_zero(textbox->buffer);
    }
    dlb_vec_reserve(textbox->buffer, MIN(UI_TEXTBOX_MIN_BUFFER_LEN, text_len + 1));  // reserve 1 extra for nil
    dlb_memcpy(textbox->buffer, text, text_len);
    dlb_vec_hdr(textbox->buffer)->len = text_len;

    // Ensure buffer is nil-terminated
    size_t new_len = dlb_vec_len(textbox->buffer);
    DLB_ASSERT(textbox->buffer[new_len] == '\0');

    // TODO: Set cursor (and selection?) based on where user clicked
    //textbox->cursor = text_len;
    //textbox->selection_start = 0;
    //textbox->selection_len = text_len;
}

static void textbox_mouse_down(ui_frame *frame)
{
    DLB_ASSERT(frame->data.textbox);

    ta_rect client_area = ui_frame_client_area(frame);
    int mouse_x = ta_mouse_x() - client_area.x;
    int mouse_y = ta_mouse_y() - client_area.y;
    if (mouse_x >= 0 && mouse_x <= client_area.w &&
        mouse_y >= 0 && mouse_y <= client_area.h)
    {
        frame->data.textbox->mouse_coords.x = mouse_x;
        frame->data.textbox->mouse_coords.y = mouse_y;
        frame->data.textbox->mouse_down = true;

        if (frame->state.pressed) {
            double now_ms = ta_timer_elapsed_ms();
            double delta_ms = now_ms - frame->data.textbox->last_clicked_ms;
            if (delta_ms < double_click_interval_ms) {
                // TODO: Select current word
                frame->data.textbox->double_clicked = true;
            }
            frame->data.textbox->last_clicked_ms = now_ms;
        }
    }
}

bool ta_ui_textbox(const char *text, size_t text_len, ta_ui_textbox_state *textbox, u32 flags)
{
    //DLB_ASSERT(text);
    //DLB_ASSERT(text_len);
    DLB_ASSERT(textbox);

    ta_rect_uv *text_rects = 0;
    ta_vec2i cursor = { 0 };
    ta_rect text_rect = { 0 };

    if (textbox->buffer) {
        ta_vec2i *mouse_coords = 0;
        if (textbox->mouse_down) {
            mouse_coords = &textbox->mouse_coords;
            textbox->mouse_down = false;
        }

        // If still editing, render buffer
        size_t buffer_len = dlb_vec_len(textbox->buffer);
        text_rect = ta_font_push_text(ui_font, textbox->buffer, buffer_len, true, &textbox->cursor, &cursor,
            mouse_coords, &text_rects);
    } else if (text) {
        // If not editing (or just canceled), render text
        text_rect = ta_font_push_text(ui_font, text, text_len, true, 0, 0, 0, &text_rects);
    }

    // Auto-expand frame based on contents
    ta_ui_next_size(MAX(next_frame_size.w, (int)text_rect.w),
                    MAX(next_frame_size.h, (int)text_rect.h));

    ui_frame_begin(UI_TEXTBOX, textbox, flags);
    ui_frame *frame = ui_frame_end(UI_TEXTBOX);

    if (textbox->focused) {
        frame->state_type = UI_STATE_ACTIVE;
        textbox->focus_changed = false;
    }

    if (frame->state.pressed) {
        if (!textbox->buffer) {
            textbox_set_text(textbox, text, text_len);
        }
        textbox_focus(textbox);
    }

    if (textbox->buffer) {
        if (frame->state.down) {
            textbox_mouse_down(frame);
        } else if (ta_key_pressed(SDL_SCANCODE_MOUSE_LEFT)) {
            // NOTE: But for console window, I want focus lost with any button to
            // mean cancel.. the correct behavior is based on the use-case. This
            // code can't be globally shared.
            //textbox_submit(textbox);
            textbox_unfocus(textbox);
        } else if (ta_key_pressed(SDL_SCANCODE_MOUSE_RIGHT)) {
            // TODO(cleanup): Right click usually means cancel, but in the case
            // of search box I want to right click to rotate camera without
            // losing my search results, sooo...
            //textbox_submit(textbox);
            textbox_unfocus(textbox);
            //textbox_command_cancel(textbox);
        }
    }

    frame->text_rects = text_rects;
    frame->cursor = cursor;
    return frame->data.textbox->submit;
}

typedef struct drag_float_state {
    float *value;     // pointer to float being dragged
    float value_orig; // value when drag started (for cancel)
    bool changed;     // true if float has been dragged at all
} drag_float_state;
static drag_float_state drag_float;

static void drag_float_begin(ta_ui_textbox_state *textbox, float *f)
{
    drag_float.value = f;
    drag_float.value_orig = *f;
    drag_float.changed = false;
    *ui_textbox_dragging = textbox;
    ta_mouse_drag_begin();
}
static void drag_float_update(float delta)
{
    if (!drag_float.value) return;

    int mouse_dx = ta_mouse_dx();
    if (mouse_dx) {
        *drag_float.value += mouse_dx * delta;
        drag_float.changed = true;
    }
}
static bool drag_float_end()
{
    DLB_ASSERT(drag_float.value);
    bool changed = drag_float.changed;
    drag_float.value = 0;
    drag_float.changed = false;
    *ui_textbox_dragging = 0;
    ta_mouse_drag_end();
    return changed;
}
static void drag_float_cancel()
{
    DLB_ASSERT(drag_float.value);
    *drag_float.value = drag_float.value_orig;
    drag_float_end();
}

bool ta_ui_textbox_float(float *value, ta_ui_textbox_state *textbox, u32 flags)
{
    DLB_ASSERT(value);
    DLB_ASSERT(textbox);

    if (!next_frame_dirty.size) {
        // TODO: Add UI_TEXT_ALIGN_RIGHT flag to make this useful
        ta_ui_next_size(60, 0);
    }

    //if (textbox->submit) {
    //    ta_ui_textbox_clear(textbox);
    //}

    ta_rect_uv *text_rects = 0;
    ta_vec2i cursor = { 0 };
    ta_rect text_rect = { 0 };

    if (textbox->buffer) {
        ta_vec2i *mouse_coords = 0;
        if (textbox->mouse_down) {
            mouse_coords = &textbox->mouse_coords;
            textbox->mouse_down = false;
        }

        // If still editing, render buffer
        size_t buffer_len = dlb_vec_len(textbox->buffer);
        text_rect = ta_font_push_text(ui_font, textbox->buffer, buffer_len, true, &textbox->cursor, &cursor,
            mouse_coords, &text_rects);
    } else {
        // If not editing (or just canceled), render text
        char text[16] = { 0 };
        size_t text_len = snprintf(CSTR(text), "%.3f", *value);
        DLB_ASSERT(text_len < sizeof(text));
        text_rect = ta_font_push_text(ui_font, text, text_len, true, 0, 0, 0, &text_rects);
    }

    // Auto-expand frame based on contents
    ta_ui_next_size(MAX(next_frame_size.w, (int)text_rect.w),
        MAX(next_frame_size.h, (int)text_rect.h));

    ui_frame_begin(UI_TEXTBOX, textbox, flags);
    ui_frame *frame = ui_frame_end(UI_TEXTBOX);

    if (textbox->focused) {
        frame->state_type = UI_STATE_ACTIVE;
        textbox->focus_changed = false;
    }

    if (textbox->buffer) {
        // Do edit mode things
        if (frame->state.down) {
            if (frame->state.pressed) {
                textbox_focus(textbox);
            }
            textbox_mouse_down(frame);
        } else if (ta_key_pressed(SDL_SCANCODE_MOUSE_LEFT)) {
            textbox_submit(textbox);
        } else if (ta_key_pressed(SDL_SCANCODE_MOUSE_RIGHT)) {
            textbox_cancel(textbox);
        }
    } else {
        // Do drag float things
        if (frame->state.pressed) {
            drag_float_begin(textbox, value);
        } else if (drag_float.value == value) {
            drag_float_update(0.01f);
            if (ta_key_released(SDL_SCANCODE_MOUSE_LEFT)) {
                // If drag ended and value didn't change, start edit mode
                if (!drag_float_end()) {
                    char text[16] = { 0 };
                    size_t text_len = snprintf(CSTR(text), "%.3f", *value);
                    DLB_ASSERT(text_len < sizeof(text));
                    textbox_set_text(textbox, text, text_len);
                    textbox_focus(textbox);
                    textbox_mouse_down(frame);
                }
            // TODO: I want right-click to cancel drag, but it's also bound to
            // rotate camera right now and cancel drag works but it rotates the
            // camera a huge amount which is annoying and gross.
            //} else if (ta_key_pressed(SDL_SCANCODE_MOUSE_RIGHT)) {
            //    drag_float_cancel();
            } else {
#if 0
                // TODO: fix this
                ta_keybind_update(&textbox_keybinds[TEXTBOX_COMMAND_CANCEL], "ta_ui_textbox_float_2");
                if (ta_keybind_triggered(&textbox_keybinds[TEXTBOX_COMMAND_CANCEL])) {
                    drag_float_cancel();
                } else {
                    frame->state_type = UI_STATE_ACTIVE;
                }
#endif
            }
        }
    }

    if (frame->data.textbox->submit) {
        *value = parse_float(textbox->buffer);
        ta_ui_textbox_clear(textbox);
        textbox_unfocus(textbox);
    } else if (ta_ui_last_state().hover && !textbox->focused) {
        ta_ui_set_cursor(TA_CURSOR_HRESIZE);
    }

    frame->text_rects = text_rects;
    frame->cursor = cursor;
    return frame->data.textbox->submit;
}
bool ta_ui_textbox_float_reset(float *value, ta_ui_textbox_state *textbox, u32 flags, float reset_value)
{
    ta_ui_row_begin();
    bool result = ta_ui_textbox_float(value, textbox, flags);

    ta_ui_next_margin(6, 1, 0, 1);
    if (ta_ui_reset_button()) {
        *value = reset_value;
    }

    if (ta_ui_last_state().hover) {
        char text[32] = { 0 };
        size_t text_len = snprintf(CSTR(text), "Reset value to %.3f", reset_value);
        DLB_ASSERT(text_len < sizeof(text));
        ta_ui_tooltip(text, text_len);
    }

    ta_ui_row_end();
    return result;
}
void ta_ui_textbox_vec2(ta_vec2 *vec, ta_ui_textbox_vec2_state* vec_state)
{
    DLB_ASSERT(vec);
    DLB_ASSERT(vec_state);

    // NOTE: Assuming length == 1 for ta_ui_label() below
    static const char *labels[2] = { "x", "y" };
    float *components = (float *)vec;
    for (int i = 0; i < 2; ++i) {
        ta_ui_next_margin(0, 1, 0, 1);
        ta_ui_label(labels[i], 1, 0);
        ta_ui_next_bg_color(UI_STATE_NONE,
            0.3f * (i == 0) + ui_default_style[UI_TEXTBOX].bg_color[UI_STATE_NONE].r * (i == 3),
            0.3f * (i == 1) + ui_default_style[UI_TEXTBOX].bg_color[UI_STATE_NONE].g * (i == 3),
            0.3f * (i == 2) + ui_default_style[UI_TEXTBOX].bg_color[UI_STATE_NONE].b * (i == 3), 1.0f);
        ta_ui_textbox_state *state = &vec_state->textbox_states[i];
        ta_ui_textbox_float(&components[i], state, 0);
    }
}
void ta_ui_textbox_vec2_normalize(ta_vec2 *vec, ta_ui_textbox_vec2_state* vec_state)
{
    ta_ui_textbox_vec2(vec, vec_state);
    if (!vec2_zero(*vec)) {
        *vec = vec2_normalize(*vec);
    }
}
void ta_ui_textbox_vec2_reset(ta_vec2 *vec, ta_ui_textbox_vec2_state* vec_state, ta_vec2 reset_value)
{
    ta_ui_row_begin();
    ta_ui_textbox_vec2(vec, vec_state);
    ta_ui_next_margin(6, 1, 0, 1);
    if (ta_ui_reset_button()) {
        *vec = reset_value;
    }
    ta_ui_row_end();
}
void ta_ui_textbox_vec2_reset_normalize(ta_vec2 *vec, ta_ui_textbox_vec2_state* vec_state, ta_vec2 reset_value)
{
    ta_ui_textbox_vec2_reset(vec, vec_state, reset_value);
    if (!vec2_zero(*vec)) {
        *vec = vec2_normalize(*vec);
    }
}

void ta_ui_textbox_vec3(ta_vec3 *vec, ta_ui_textbox_vec3_state* vec_state, bool normalize, bool reset_button)
{
    DLB_ASSERT(vec);
    DLB_ASSERT(vec_state);

    ta_ui_next_pad(0, 0, 0, 0);
    ta_ui_panel_begin(&vec_state->panel_state, TA_UI_AUTOSIZE);
    ta_ui_row_begin();

    // NOTE: Assuming length == 1 for ta_ui_label() below
    static const char *labels[3] = { "x", "y", "z" };
    float *components = (float *)vec;
    for (int i = 0; i < 3; ++i) {
        ta_ui_next_margin(0, 1, 0, 1);
        ta_ui_label(labels[i], 1, 0);
        ta_ui_next_bg_color(UI_STATE_NONE,
            0.3f * (i == 0) + ui_default_style[UI_TEXTBOX].bg_color[UI_STATE_NONE].r * (i == 3),
            0.3f * (i == 1) + ui_default_style[UI_TEXTBOX].bg_color[UI_STATE_NONE].g * (i == 3),
            0.3f * (i == 2) + ui_default_style[UI_TEXTBOX].bg_color[UI_STATE_NONE].b * (i == 3), 1.0f);
        ta_ui_textbox_state *state = &vec_state->textbox_states[i];
        ta_ui_textbox_float(&components[i], state, 0);
    }
    if (reset_button) {
        ta_ui_next_margin(6, 1, 0, 1);
        if (ta_ui_reset_button()) {
            *vec = VEC3_ZERO;
        }
    }
    if (normalize) {
        *vec = vec3_normalize(*vec);
    }

    ta_ui_row_end();
    ta_ui_panel_end();
}
void ta_ui_textbox_vec4(ta_vec4 *vec, ta_ui_textbox_vec4_state* vec_state, bool normalize, bool reset_button)
{
    DLB_ASSERT(vec);
    DLB_ASSERT(vec_state);

    ta_ui_next_pad(0, 0, 0, 0);
    ta_ui_panel_begin(&vec_state->panel_state, TA_UI_AUTOSIZE);
    ta_ui_row_begin();

    // NOTE: Assuming length == 1 for ta_ui_label() below
    static const char *labels[4] = { "x", "y", "z", "w" };
    float *components = (float *)vec;
    for (int i = 0; i < 4; ++i) {
        ta_ui_next_margin(0, 1, 0, 1);
        ta_ui_label(labels[i], 1, 0);
        ta_ui_next_bg_color(UI_STATE_NONE,
            0.3f * (i == 0) + ui_default_style[UI_TEXTBOX].bg_color[UI_STATE_NONE].r * (i == 3),
            0.3f * (i == 1) + ui_default_style[UI_TEXTBOX].bg_color[UI_STATE_NONE].g * (i == 3),
            0.3f * (i == 2) + ui_default_style[UI_TEXTBOX].bg_color[UI_STATE_NONE].b * (i == 3), 1.0f);
        ta_ui_textbox_state *state = &vec_state->textbox_states[i];
        ta_ui_textbox_float(&components[i], state, 0);
    }
    if (reset_button) {
        ta_ui_next_margin(6, 1, 0, 1);
        if (ta_ui_reset_button()) {
            *vec = QUAT_IDENT;
        }
    }
    if (normalize) {
        *vec = quat_normalize(*vec);
    }

    ta_ui_row_end();
    ta_ui_panel_end();
}
void ta_ui_textbox_focus(ta_ui_textbox_state *textbox)
{
    // TODO: Would be nice to focus console textbox when console is clicked
    DLB_ASSERT(!"Doesn't work for some reason, probably related to mouse_down");
    DLB_ASSERT(textbox);
    textbox_focus(textbox);
}
bool ta_ui_textbox_insert(ta_ui_textbox_state *textbox, char c)
{
    DLB_ASSERT(textbox);
    //DLB_ASSERT(textbox->buffer);

    // Cleanup: This doesn't work because we don't know if the codepoint event is a repeat event, and if I try to
    // auto-detect it based on the interval it fails for e.g. press A, release A, press A within the interval.
    //static double last_repeat_ms = 0;
    //static int repeat = 0;
    //static char c_prev;
    //if (c != c_prev) {
    //    repeat = 0;
    //    c_prev = c;
    //}
    //if (!textbox_repeat_valid(&last_repeat_ms, &repeat, 0)) {
    //    return false;
    //}

    if (!textbox->filter) {
        textbox->filter = ta_textbox_filter_default;
    }
    if (!textbox->filter(c)) {
        return false;
    }

    // TODO: dlb_vec_insert_at
    size_t len = dlb_vec_len(textbox->buffer);
    dlb_vec_reserve(textbox->buffer, len + 2);  // reserve 2, for c and nil
    dlb_memmove(
        textbox->buffer + textbox->cursor + 1,
        textbox->buffer + textbox->cursor,
        len - textbox->cursor
    );
    textbox->buffer[textbox->cursor] = c;
    dlb_vec_hdr(textbox->buffer)->len++;
    textbox->cursor++;

    // Ensure buffer is nil-terminated
    size_t new_len = dlb_vec_len(textbox->buffer);
    DLB_ASSERT(textbox->buffer[new_len] == '\0');

    return true;
}
void ta_ui_textbox_clear(ta_ui_textbox_state *textbox)
{
    DLB_ASSERT(textbox);
    textbox_clear(textbox);
}
void ta_ui_textbox_submit(ta_ui_textbox_state *textbox)
{
    DLB_ASSERT(textbox);
    textbox_submit(textbox);
}
void ta_ui_textbox_cancel(ta_ui_textbox_state *textbox)
{
    DLB_ASSERT(textbox);
    textbox_cancel(textbox);
}
void ta_ui_textbox_set_text(ta_ui_textbox_state *textbox, const char *text, size_t text_len)
{
    DLB_ASSERT(textbox);
    textbox_set_text(textbox, text, text_len);
}

void ta_ui_tooltip_begin(const char *name)
{
    UNUSED(name);
    // TODO: Make this a container
    DLB_ASSERT(0);
}
void ta_ui_tooltip_end(const char *name)
{
    UNUSED(name);
    // TODO: Make this a container
    DLB_ASSERT(0);
}
void ta_ui_tooltip(const char *text, size_t text_len)
{
    ta_rect_uv *text_rects = 0;
    ta_rect text_rect = ta_font_push_text(ui_font, text, text_len, true, 0, 0, 0, &text_rects);

    int x = ta_mouse_x();
    int y = ta_mouse_y();
    int offset_x = x + 10;
    int offset_y = y + 20;

    ta_rect_uv tooltip_bg = { 0 };
    tooltip_bg.rect.x = offset_x - 4;
    tooltip_bg.rect.y = offset_y - 2;
    tooltip_bg.rect.w = text_rect.w + 8;
    tooltip_bg.rect.h = text_rect.h + 3;
    ta_primitive_push_rect_uv(&primitive_quads_tooltip_bg, tooltip_bg, TA_COLOR_GRAY3A, UI_LAYER_TIP_BG, true, false);

    dlb_vec_each(ta_rect_uv *, rect, text_rects) {
        ta_rect_uv offset_rect = *rect;
        offset_rect.rect.x += offset_x;
        offset_rect.rect.y += offset_y;
        ta_primitive_push_rect_uv(&primitive_quads_tooltip_fg, offset_rect, TA_COLOR_WHITE, UI_LAYER_TIP, true, false);
    }
    dlb_vec_zero(text_rects);
}
#if 0
// TODO: This doesn't make any sense.. where does text come from and why are the
// rects going into tooltip_bg queue? Might want a statusbar but NotLikeThis.
void ta_ui_statusbar()
{
    const float statusbar_pad = 4;
    ta_rect_uv status_bg = { 0 };
    status_bg.rect.x = statusbar_pad;
    status_bg.rect.y = -(float)(ui_font->line_height + statusbar_pad);
    status_bg.rect.w = (float)(WINDOW_W - statusbar_pad * 2);
    status_bg.rect.h = (float)ui_font->line_height;
    ta_primitive_push_rect_uv(&primitive_quads_tooltip_bg, status_bg, TA_COLOR_GRAY3A,
        UI_LAYER_TIP_BG, true, false);
}
#endif

static void ui_render_window(ui_frame *frame)
{
    // Window background
    ta_rect bg_rect = frame->rect;
    bg_rect.w = frame->rect.w;
    ta_primitive_push_rect(0, bg_rect, frame->bg_color[frame->state_type], UI_LAYER_EDIT_WINDOW_BG);
    ta_primitive_render_mesh(&primitive_quads, tg_shader_quads, TA_TRIANGLES, true, false);
}
static void ui_render_panel(ui_frame *frame)
{
    // Panel background
    ta_primitive_push_rect(0, frame->rect, frame->bg_color[frame->state_type], UI_LAYER_EDIT_1_BG);
    ta_primitive_render_mesh(&primitive_quads, tg_shader_quads, TA_TRIANGLES, true, false);
}
static void ui_render_button(ui_frame *frame)
{
    ta_primitive_push_rect(0, frame->rect, frame->bg_color[frame->state_type], UI_LAYER_EDIT_1_BG);
    ta_primitive_render_mesh(&primitive_quads, tg_shader_quads, TA_TRIANGLES, true, false);
}
static void ui_render_toggle_button(ui_frame *frame)
{
    ta_rgba bg_color = frame->bg_color[frame->state_type];
    ta_primitive_push_rect(0, frame->rect, bg_color, UI_LAYER_EDIT_1_BG);
    ta_primitive_render_mesh(&primitive_quads, tg_shader_quads, TA_TRIANGLES, true, false);
}
static void ui_render_image(ui_frame *frame)
{
    if (frame->texture) {
        ta_texture *tex = ta_game_by_sym(RES_TEXTURE, frame->texture);
        ta_rect img_rect = rect_shrink(frame->rect, frame->pad);
        DLB_ASSERT(tex->type == TA_TEXTURE_2D_ARRAY);
        ta_shader_set_uint(tg_shader_quads, SYM_U_TEXTURE_POOL_INDEX, tex->gl_texture_pool_index);
        ta_shader_set_uint(tg_shader_quads, SYM_U_TEXTURE_ARRAY_LAYER, tex->gl_texture_pool_layer);
        ta_primitive_push_rect(0, img_rect, TA_COLOR_INVIS, UI_LAYER_EDIT_1);
        ta_primitive_render_mesh(&primitive_quads, tg_shader_quads, TA_TRIANGLES, true, false);
        ta_shader_set_uint(tg_shader_quads, SYM_U_TEXTURE_POOL_INDEX, 0);
        ta_shader_set_uint(tg_shader_quads, SYM_U_TEXTURE_ARRAY_LAYER, 0);
    }
}
static void ui_render_text(int x, int y, ta_rect_uv *text_rects, ta_rect clip_rect)
{
#if 1
    size_t rect_count = dlb_vec_len(text_rects);
    if (rect_count) {
        // Binary search to find overlapping y values (ignore everything before and after)
        size_t first_in_bounds;
        size_t left = 0;
        size_t right = rect_count - 1;
        size_t mid = left;  // NOTE: Start at beginning for O(1) when scrollbar is at top
        while (left < right) {
            if (y + text_rects[mid].rect.y + text_rects[mid].rect.h < clip_rect.y) {
                left = MIN(SIZE_MAX - 1, mid) + 1;
            } else {
                right = MAX(1, mid) - 1;
            }
            mid = (left + right) / 2;
        }
        first_in_bounds = left;

        size_t last_in_bounds;
        left = 0;
        right = rect_count - 1;
        mid = right;  // NOTE: Start at end for O(1) when scrollbar is at bottom
        while (left < right) {
            if (y + text_rects[mid].rect.y > clip_rect.y + clip_rect.h) {
                right = MAX(1, mid) - 1;
            } else {
                left = MIN(SIZE_MAX - 1, mid) + 1;
            }
            mid = (left + right) / 2;
        }
        last_in_bounds = right;

#if 0
        // CLEANUP(dlb): Chop off the first and last letters for easy verification that the correct bounds were found
        //first_in_bounds = MIN(rect_count - 1, first_in_bounds + 1);
        //last_in_bounds = (last_in_bounds >= 1) ? last_in_bounds - 1 : 0;
#endif

        DLB_ASSERT(first_in_bounds < rect_count);
        DLB_ASSERT(last_in_bounds < rect_count);

        size_t push_count = (last_in_bounds - first_in_bounds) + 1;
        dlb_vec_reserve(primitive_quads.positions, push_count);
        dlb_vec_reserve(primitive_quads.uvs, push_count);
        dlb_vec_reserve(primitive_quads.colors, push_count);

        for (size_t i = first_in_bounds; i <= last_in_bounds; ++i) {
            ta_primitive_push_rect_uv(&primitive_quads, text_rects[i], TA_COLOR_WHITE, 0, true, false);
        }

        ta_font_render(ui_font, (float)x, (float)y, UI_LAYER_EDIT_1, true, false, &primitive_quads);
    }
#else
    // Slow code (no culling), for reference
    dlb_vec_each(ta_rect_uv *, text_rect, text_rects)
    {
        ta_primitive_push_rect_uv(&primitive_quads, *text_rect, TA_COLOR_WHITE, 0, true, false);
    }

    ta_font_render(ui_font, (float)x, (float)y, UI_LAYER_EDIT_1, true, false, &primitive_quads);
#endif
}
static void ui_render_label(ui_frame *frame)
{
    // Render background
    if (frame->bg_color[frame->state_type].a == 0.0f) {
        frame->bg_color[frame->state_type].a = TA_EPSILON;
    }
    ta_primitive_push_rect(0, frame->rect, frame->bg_color[frame->state_type], UI_LAYER_EDIT_1_BG);
    ta_primitive_render_mesh(&primitive_quads, tg_shader_quads, TA_TRIANGLES, true, false);

    // Render text
    int x = frame->rect.x + frame->pad.x;
    int y = frame->rect.y + frame->pad.y;
    ui_render_text(x, y, frame->text_rects, frame->clip_rect);
}
static void ui_render_textbox(ui_frame *frame)
{
    // Render background
    ta_rgba bg_color = frame->bg_color[frame->state_type];
    ta_primitive_push_rect(0, frame->rect, bg_color, UI_LAYER_EDIT_1_BG);
    ta_primitive_render_mesh(&primitive_quads, tg_shader_quads, TA_TRIANGLES, true, false);

    // Render text
    int x = frame->rect.x + frame->pad.x;
    int y = frame->rect.y + frame->pad.y;
    ui_render_text(x, y, frame->text_rects, frame->clip_rect);

    // If active, render cursor
    if (frame->data.textbox->focused && !frame->data.textbox->focus_changed) {
        ta_rect cursor_rect = { 0 };
        cursor_rect.x = x + (int)frame->cursor.x;
        cursor_rect.y = y + (int)frame->cursor.y + 1;
        cursor_rect.w = 1;
        cursor_rect.h = ui_font->line_height - 2;
        ta_primitive_push_rect(0, cursor_rect, TA_COLOR_GRAY8, UI_LAYER_EDIT_2);
        ta_primitive_render_mesh(&primitive_quads, tg_shader_quads, TA_TRIANGLES, true, false);
    }
}
static void ui_render_scrollbars(ui_frame *frame)
{
    static bool dragging_v = false;

    DLB_ASSERT(frame);

    ta_ui_scroll_state *scroll = ui_scroll_state(frame);
    if (!scroll) {
        return;
    }

    int overflow_x = frame->content_size.w - frame->rect.w;
    int overflow_y = frame->content_size.h - frame->rect.h;

    // TODO: Scroll acceleration might be nice (for scroll wheel, not mouse drag)

    // Horizontal scrollbar
    if (overflow_x > 0) {
        // TODO: Calc horiz scrollbar
    }

    // Vertical scrollbar
    if (overflow_y > 0) {
        ta_rect scroll_v_rect = { 0 };
        scroll_v_rect.w = SCROLL_THICKNESS;
        scroll_v_rect.x = frame->rect.x + frame->rect.w - scroll_v_rect.w;
        scroll_v_rect.y = frame->rect.y;
        scroll_v_rect.h = frame->rect.h;

        float widget_h_pct = (float)frame->rect.h / frame->content_size.h;
        int widget_h = (int)(frame->rect.h * widget_h_pct);
        int scroll_space_v = frame->rect.h - widget_h;
        int scroll_v = (int)(scroll->percent.y * scroll_space_v);

        ta_rect scroll_v_widget = { 0 };
        scroll_v_widget.w = SCROLL_WIDGET_THICKNESS;
        scroll_v_widget.x = frame->rect.x + frame->rect.w - scroll_v_widget.w - SCROLL_WIDGET_PAD;
        scroll_v_widget.y = frame->rect.y + scroll_v;
        scroll_v_widget.h = widget_h;

        bool widget_hover = rect_contains_mouse(scroll_v_widget);
        ta_rgba widget_color = (ta_rgba){ 0.6f, 0.0f, 0.0f, 1.0f };
        if (widget_hover && !dragging_v) {
            widget_color = (ta_rgba){ 0.8f, 0.0f, 0.0f, 1.0f };
        }

        ta_primitive_push_rect(0, scroll_v_rect, TA_COLOR_GRAY4, UI_LAYER_EDIT_1);
        ta_primitive_push_rect(0, scroll_v_widget, widget_color, UI_LAYER_EDIT_1);
        ta_primitive_render_mesh(&primitive_quads, tg_shader_quads, TA_TRIANGLES, true, false);

        // Update scroll state for next frame
        if (!ta_mouse_captured()) {
            int delta_y = 0;

            if (widget_hover && ta_key_pressed(SDL_SCANCODE_MOUSE_LEFT))
            {
                // Mouse drag
                //scrollbar_y_frame_idx = frame->index;
                dragging_v = true;
                ta_mouse_drag_begin();
            } else if (!dragging_v && rect_contains_mouse(frame->rect)) {
                // Scroll wheel
                delta_y = ta_mouse_scroll_dy() * SCROLL_WHEEL_SPEED;
            } else if (!ta_key_down(SDL_SCANCODE_MOUSE_LEFT)) {
                // Not dragging
                if (dragging_v) {
                    dragging_v = false;
                    ta_mouse_drag_end();
                }
            }

            if (dragging_v) {
                delta_y = ta_mouse_dy();
            }
            scroll->percent.y += (float)delta_y / scroll_space_v;
        }

        scroll->percent.y = clampf(scroll->percent.y, 0.0f, 1.0f);
        scroll->pixels.y = (int)(scroll->percent.y * overflow_y);
    } else {
        scroll->percent.y = 0.0f;
        scroll->pixels.y = 0;
    }
}
static void ui_render_tooltips()
{
    if (primitive_quads_tooltip_fg.positions) {
        ta_primitive_render_mesh(&primitive_quads_tooltip_bg, tg_shader_quads, TA_TRIANGLES, true, false);
        ta_font_render(ui_font, 0, 0, UI_LAYER_TIP, true, false, &primitive_quads_tooltip_fg);
    }
}
#if 0
// TODO: Move this to ta_ui_statusbar
static void ta_ui_render_statusbar()
{
    if (editor.status_msg) {
        static ta_rect_uv *status_rects = 0;
        ta_font *font = ta_game_by_name(RES_FONT, tg_font);
        ta_rectf status_rect = ta_font_push_text(&status_rects, font,
            SYM(editor.status_msg), true, 0, 0, 0);
        dlb_vec_each(ta_rect_uv *, rect, status_rects) {
            ta_primitive_push_rect_uv(&&primitive_quads, *rect, TA_COLOR_WHITE, 0,
                true, false);
        }
        dlb_vec_zero(status_rects);

        int status_halfw = WINDOW_W / 2 - (int)status_rect.w / 2;
        const int status_pad_bottom = 20;
        ta_font_render(&primitive_quads, font, (float)status_halfw,
            (float)(WINDOW_H - (font->ascent + status_pad_bottom)),
            UI_LAYER_TIP, true, false);

        editor.status_msg = 0;
    }
}
#endif
void ta_ui_render()
{
    static void (*ui_renderers[])(ui_frame *frame) = {
        [UI_ROOT]           = 0,
        [UI_WINDOW]         = ui_render_window,
        [UI_PANEL]          = ui_render_panel,
        [UI_BUTTON]         = ui_render_button,
        [UI_TOGGLE_BUTTON]  = ui_render_toggle_button,
        [UI_IMAGE]          = ui_render_image,
        [UI_LABEL]          = ui_render_label,
        [UI_TEXTBOX]        = ui_render_textbox,
    };

    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_SCISSOR_TEST);

    // TODO: Make a shader specifically for rendering 1-channel depth maps so that it doesn't appear red. Not
    // possible with tg_shader_quads because it's used for other things as well.
    ta_shader_set_mat4(tg_shader_quads, SYM_U_PROJ, &MAT4_IDENT);
    ta_shader_set_mat4(tg_shader_quads, SYM_U_VIEW, &MAT4_IDENT);
    ta_shader_set_mat4(tg_shader_quads, SYM_U_MODEL, &MAT4_IDENT);

    dlb_vec_each(ui_frame *, frame, ui_frames) {
        if (!(frame->internal_flags & TA_UI_INVISIBLE) && ui_renderers[frame->type]) {
            frame->clip_rect = TA_RECT_ZERO;
            frame->clip_rect.w = WINDOW_W;
            frame->clip_rect.h = WINDOW_H;
            size_t container_idx = frame->container_idx;
            while(container_idx) {
                DLB_ASSERT(container_idx < dlb_vec_len(ui_frames));
                ui_frame *container = &ui_frames[container_idx];
                frame->clip_rect = rect_intersection(frame->clip_rect, container->rect);
                if (!frame->clip_rect.w || !frame->clip_rect.h) {
                    break;
                }
                container_idx = container->container_idx;
            }

            // NOTE: If overlap has zero width/height, there's nothing to render
            if (frame->clip_rect.w && frame->clip_rect.h) {
                int inv_y = WINDOW_H - (frame->clip_rect.y + frame->clip_rect.h);
                // Note: OpenGL generates error 1281 (invalid value) if w/h is negative
                DLB_ASSERT(frame->clip_rect.w >= 0);
                DLB_ASSERT(frame->clip_rect.h >= 0);
                glScissor(frame->clip_rect.x, inv_y, frame->clip_rect.w, frame->clip_rect.h);

                ui_renderers[frame->type](frame);
                ui_render_scrollbars(frame);
            }
        }

        // Clear any per-frame memory
        if (frame->text_rects_internal) {
            dlb_vec_free(frame->text_rects);
        } else {
            dlb_vec_zero(frame->text_rects);
        }
    }

    glDisable(GL_SCISSOR_TEST);

    ui_render_tooltips();

    last_frame_state = 0;
    dlb_vec_zero(ui_frames);
    dlb_vec_alloc(ui_frames);  // reserve UI_ROOT
}