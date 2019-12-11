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
#include "dlb/dlb_vector.h"
#include "dlb/dlb_murmur3.h"
#include "misc/gl3w.h"
#include "SDL/SDL_keyboard.h"

#define UI_DEBUG_MARGIN         1
#define UI_DEBUG_PAD            1
#define UI_DEBUG_PANEL          0
#define UI_DEBUG_NO_TEXTURES    0
#define UI_DEBUG_RANDOM_COLORS  1

#define WIDGET_PAD              1
#define SCROLL_WIDGET_THICKNESS 8
#define SCROLL_WIDGET_H_MIN     4
#define SCROLL_WHEEL_SPEED      10

// internal flags
#define TA_UI_INVISIBLE         0x20000000  // takes up space but doesn't render (display: hidden)
#define TA_UI_CONTAINER         0x40000000  // [internal] will always be set on containers
#define TA_UI_CONTAINER_ENDED   0x80000000  // [internal] will be set when container ends

static ta_font *ui_font;

typedef struct ui_style {
    ta_rect margin;
    ta_rect pad;
    ta_rgba bg_color[UI_STATE_COUNT];
    ta_rgba fg_color[UI_STATE_COUNT];
    bool invisible;
} ui_style;

static ui_style ui_default_style[UI_COUNT] = { 0 };

static ta_vec2i next_frame_pos_relative;
static ta_size next_frame_size;
static ui_style next_frame_style;
static ta_ui_state *last_frame_state;

// Flags for each override to make it easier to determine if they were set
static struct {
    bool pos_relative;
    bool size;
    bool margin;
    bool pad;
    bool bg_color[UI_STATE_COUNT];
    bool fg_color[UI_STATE_COUNT];
    bool invisible;
} next_frame_dirty;

typedef struct ui_scrollbar {
    ta_rect rect;    // scrollbar background rect
    ta_rect widget;  // scroll widget
} ui_scrollbar;

typedef struct ui_frame {
    u32 index;
    ui_frame_type type;
    const char *name;
    struct ui_frame *container;

    ta_rect margin;
    ta_rect pad;
    ta_rgba bg_color;
    ta_rgba fg_color;
    ta_rect rect;           // position & size (-margin, +pad)
    ta_vec2i offset;        // dynamic offset for layout
    bool skip_flow;         // if true, doesn't affect flow of parent container

    int row_height;         // height of current layout row
    bool row_continue;      // if true, next element will layout on same row
    ta_size content_size;   // dynamic content size (-margin, +pad)
    ui_scrollbar scroll_h;  // horizontal scrollbar
    ui_scrollbar scroll_v;  // vertical scrollbar
    ta_vec2i scroll_offset;  // absolute offset for just this frame, include parents
    ta_rect clip_rect;

    ta_rect_uv *text_rects; // vector, must be freed!
    ta_texture *texture;
    int texture_face;       // for cubemaps
    ta_vec2 cursor;         // cursor location for textboxes

    // Consolidate this and all bools into a single flags bitmap
    ta_ui_state state;

    // TA_UI_INVISIBLE
    u32 internal_flags;

    union {
        void *ptr;
        ta_ui_window_state *window;
        ta_ui_panel_state *panel;
        ta_ui_textbox_state *textbox;
    } data;
} ui_frame;

static ui_frame *ui_frames;

enum {
    TEXTBOX_EVENT_NEWLINE,
    TEXTBOX_EVENT_SUBMIT,
    TEXTBOX_EVENT_CANCEL,
    TEXTBOX_EVENT_BACKSPACE,
    TEXTBOX_EVENT_DELETE,
    TEXTBOX_EVENT_CURSOR_RIGHT,
    TEXTBOX_EVENT_CURSOR_LEFT,
    TEXTBOX_EVENT_CURSOR_DOWN,
    TEXTBOX_EVENT_CURSOR_UP,
    TEXTBOX_EVENT_CURSOR_BOL,
    TEXTBOX_EVENT_CURSOR_EOL,
    TEXTBOX_EVENT_CURSOR_BOF,
    TEXTBOX_EVENT_CURSOR_EOF,
    TEXTBOX_EVENT_COUNT
};
static ta_keybind textbox_keybinds[TEXTBOX_EVENT_COUNT] = { 0 };

static void ui_row_end(ui_frame *container);

void ta_ui_init(ta_font *font)
{
    ui_font = font;

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

    //ui_default_style[UI_WINDOW].margin                      = TA_RECT_ZERO;
    ui_default_style[UI_WINDOW].pad                         = TA_RECT(4, 4, 4, 4);
    //ui_default_style[UI_WINDOW].bg_color[UI_STATE_NONE]     = TA_COLOR_INVIS;
    ui_default_style[UI_WINDOW].bg_color[UI_STATE_HOVER]    = TA_RGBA(0.0f, 1.0f, 0.0f, 0.9f);
    //ui_default_style[UI_WINDOW].bg_color[UI_STATE_DOWN]     = TA_COLOR_INVIS;
    //ui_default_style[UI_WINDOW].bg_color[UI_STATE_ACTIVE]   = TA_COLOR_INVIS;
    //ui_default_style[UI_WINDOW].fg_color[UI_STATE_NONE]     = TA_COLOR_INVIS;
    //ui_default_style[UI_WINDOW].fg_color[UI_STATE_HOVER]    = TA_COLOR_INVIS;
    //ui_default_style[UI_WINDOW].fg_color[UI_STATE_DOWN]     = TA_COLOR_INVIS;
    //ui_default_style[UI_WINDOW].fg_color[UI_STATE_ACTIVE]   = TA_COLOR_INVIS;

    //ui_default_style[UI_PANEL].margin                     = TA_RECT(0, 0, 0, 0);
    ui_default_style[UI_PANEL].pad                          = TA_RECT(4, 4, 4, 4);
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

    ta_keybind_init1(&textbox_keybinds[TEXTBOX_EVENT_NEWLINE],      TA_KEYBIND_PRESS,   SDL_SCANCODE_RETURN);
    ta_keybind_init1(&textbox_keybinds[TEXTBOX_EVENT_SUBMIT],       TA_KEYBIND_PRESS,   SDL_SCANCODE_RETURN);
    ta_keybind_init1(&textbox_keybinds[TEXTBOX_EVENT_CANCEL],       TA_KEYBIND_RELEASE, SDL_SCANCODE_ESCAPE);
    ta_keybind_init1(&textbox_keybinds[TEXTBOX_EVENT_BACKSPACE],    TA_KEYBIND_PRESS,   SDL_SCANCODE_BACKSPACE);
    ta_keybind_init1(&textbox_keybinds[TEXTBOX_EVENT_DELETE],       TA_KEYBIND_PRESS,   SDL_SCANCODE_DELETE);
    ta_keybind_init1(&textbox_keybinds[TEXTBOX_EVENT_CURSOR_RIGHT], TA_KEYBIND_HOLD,    SDL_SCANCODE_RIGHT);
    ta_keybind_init1(&textbox_keybinds[TEXTBOX_EVENT_CURSOR_LEFT],  TA_KEYBIND_HOLD,    SDL_SCANCODE_LEFT);
    ta_keybind_init1(&textbox_keybinds[TEXTBOX_EVENT_CURSOR_DOWN],  TA_KEYBIND_PRESS,   SDL_SCANCODE_DOWN);
    ta_keybind_init1(&textbox_keybinds[TEXTBOX_EVENT_CURSOR_UP],    TA_KEYBIND_PRESS,   SDL_SCANCODE_UP);
    ta_keybind_init1(&textbox_keybinds[TEXTBOX_EVENT_CURSOR_BOL],   TA_KEYBIND_PRESS,   SDL_SCANCODE_HOME);
    ta_keybind_init1(&textbox_keybinds[TEXTBOX_EVENT_CURSOR_EOL],   TA_KEYBIND_PRESS,   SDL_SCANCODE_END);
    ta_keybind_init2(&textbox_keybinds[TEXTBOX_EVENT_CURSOR_BOF],   TA_KEYBIND_PRESS,   SDL_SCANCODE_LSHIFT, SDL_SCANCODE_HOME);
    ta_keybind_init2(&textbox_keybinds[TEXTBOX_EVENT_CURSOR_EOF],   TA_KEYBIND_PRESS,   SDL_SCANCODE_LSHIFT, SDL_SCANCODE_END);
}

#if 1
static ta_rgba ui_random_color(u32 frame_idx, ui_state_type state)
{
#if 1
    int seed = state;
#else
    int seed = ui_frames[frame_idx].type;
    if (seed == UI_PANEL) {
        seed += UI_COUNT * state;
    }
#endif
    // HACK: Generate random colors based on name of control
    ta_rgba color;
    u32 hash = dlb_murmur3(&frame_idx, sizeof(frame_idx));
    color.r = ((hash >> (seed     )) & 255) / 255.0f;
    color.g = ((hash >> (seed +  8)) & 255) / 255.0f;
    color.b = ((hash >> (seed + 16)) & 255) / 255.0f;
    color.a = 1.0f;
    return color;
}
#endif
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
    int idx = 0;
    for (int i = frame_idx - 1; i >= 0; i--) {
        if (ui_frames[i].internal_flags & TA_UI_CONTAINER &&
            !(ui_frames[i].internal_flags & TA_UI_CONTAINER_ENDED))
        {
            idx = i;
            break;
        }
    }
    ui_frame *frame = &ui_frames[idx];
    DLB_ASSERT(frame);
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
static ta_rect rect_intersect(ta_rect a, ta_rect b)
{
    ta_rect result = { 0 };
    result.x = MAX(a.x, b.x);
    result.y = MAX(a.y, b.y);
    result.w = MIN(a.x + a.w, b.x + b.w) - result.x;
    result.h = MIN(a.y + a.h, b.y + b.h) - result.y;
    return result;
}
static bool rect_contains_mouse(ta_rect rect)
{
    if (!ta_mouse_captured() && !ta_mouse_dragging() &&
        MOUSE_X >= rect.x && MOUSE_X < rect.x + rect.w &&
        MOUSE_Y >= rect.y && MOUSE_Y < rect.y + rect.h)
    {
        return true;
    }
    return false;
}
// returns frame index
static void ui_frame_begin(ui_frame_type type, const char *name,
    void *data, u32 flags)
{
    // Allocate frame
    ui_frame *frame = dlb_vec_alloc(ui_frames);
    frame->index = dlb_vec_len(ui_frames) - 1;
    frame->type = type;
    frame->name = name;
    frame->data.ptr = data;
    frame->internal_flags = flags;

    frame->margin = next_frame_dirty.margin
        ? next_frame_style.margin
        : ui_default_style[type].margin;
    frame->pad = next_frame_dirty.pad
        ? next_frame_style.pad
        : ui_default_style[type].pad;

    frame->container = ui_container(frame->index);
    DLB_ASSERT(frame->container->index < dlb_vec_len(ui_frames));

    ta_vec2i offset = frame->container->offset;
    if (next_frame_dirty.pos_relative) {
        offset = next_frame_pos_relative;
        frame->skip_flow = next_frame_dirty.pos_relative;
    }

    frame->rect.x = frame->container->rect.x + offset.x + frame->margin.x;
    frame->rect.y = frame->container->rect.y + offset.y + frame->margin.y;
    frame->rect.w = frame->pad.x + frame->pad.w;
    frame->rect.h = frame->pad.y + frame->pad.h;
    frame->content_size.w = frame->rect.w;
    frame->content_size.h = frame->rect.h;

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
        }
    }
    return scroll;
}
static void ui_frame_scrollbars(ui_frame *frame)
{
    DLB_ASSERT(frame);

    ta_ui_scroll_state *scroll = ui_scroll_state(frame);
    if (!scroll) {
        return;
    }

    dlb_memset(&frame->scroll_h, 0, sizeof(frame->scroll_h));
    dlb_memset(&frame->scroll_v, 0, sizeof(frame->scroll_v));

    int overflow_x = frame->content_size.w - frame->rect.w;
    int overflow_y = frame->content_size.h - frame->rect.h;

    // TODO: Scroll acceleration might be nice (for scroll wheel, not mouse drag)

    // TODO: Horizontal scrollbar
    if (overflow_x > 0) {

    }

    // Vertical scrollbar
    if (overflow_y > 0) {
        frame->scroll_v.rect.x = frame->rect.x + frame->content_size.w;
        frame->scroll_v.rect.y = frame->rect.y;
        frame->scroll_v.rect.w = SCROLL_WIDGET_THICKNESS;
        frame->scroll_v.rect.h = frame->rect.h;

        float widget_h_pct = (float)frame->rect.h / frame->content_size.h;
        int widget_h = (int)(frame->rect.h * widget_h_pct);
        int scroll_space_v = frame->rect.h - widget_h;
        int scroll_v = (int)(scroll->scroll_pct.y * scroll_space_v);

        frame->scroll_v.widget.x = frame->rect.x + frame->content_size.w;
        frame->scroll_v.widget.w = SCROLL_WIDGET_THICKNESS;
        frame->scroll_v.widget.y = frame->rect.y + scroll_v;
        frame->scroll_v.widget.h = widget_h;
    }

    // If mouse is captured we're not scrolling
    if (!ta_mouse_captured()) {
        // TODO: Horizontal scrolling
        if (overflow_x > 0) {

        }

        if (overflow_y > 0) {
            static bool dragging_v = false;
            int delta_y = 0;

            if (ta_key_pressed(SDL_SCANCODE_MOUSE_LEFT) && rect_contains_mouse(frame->scroll_v.widget)) {
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

            if (delta_y) {
                int scroll_space_v = frame->rect.h - frame->scroll_v.widget.h;
                scroll->scroll_pct.y += (float)delta_y / scroll_space_v;
                scroll->scroll_pct.y = clampf(scroll->scroll_pct.y, 0.0f, 1.0f);
            }
        }
    }

    frame->scroll_offset.x = (int)(scroll->scroll_pct.x * overflow_x);
    frame->scroll_offset.y = (int)(scroll->scroll_pct.y * overflow_y);
}
static ui_frame *ui_frame_end(ui_frame_type type)
{
    ui_frame *frame = 0;
    for (frame = dlb_vec_last(ui_frames); frame != ui_frames; --frame) {
        if (frame->type == type &&
            !(frame->internal_flags & TA_UI_CONTAINER_ENDED))
        {
            frame = frame;
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
            frame->rect.w += SCROLL_WIDGET_THICKNESS;
        }
        if (frame->content_size.w > frame->rect.w) {
            frame->rect.h += SCROLL_WIDGET_THICKNESS;
        }
    }

    // NOTE: Setting offset manually is like position: relative. May want to
    // have a position: absolute that also doesn't affect flow.
    if (!frame->skip_flow) {
        int frame_w = frame->margin.x + frame->rect.w + frame->margin.w;
        int frame_h = frame->margin.y + frame->rect.h + frame->margin.h;
        frame->container->offset.x += frame_w;
        frame->container->row_height = MAX(frame->container->row_height, frame_h);

        if (!frame->container->row_continue) {
            ui_row_end(frame->container);
        }
    }

    ui_frame_scrollbars(frame);

    // Updated frame states
    ui_state_type state = UI_STATE_NONE;
    frame->state.hover = false;
    frame->state.down = false;
    frame->state.pressed = false;
    frame->state.released = false;
    if (rect_contains_mouse(frame->rect))
    {
        state = UI_STATE_HOVER;
        frame->state.hover = true;
        if (ta_key_down(SDL_SCANCODE_MOUSE_LEFT)) {
            state = UI_STATE_DOWN;
            frame->state.down = true;
            frame->state.pressed = ta_key_pressed(SDL_SCANCODE_MOUSE_LEFT);
        } else {
            frame->state.released = ta_key_released(SDL_SCANCODE_MOUSE_LEFT);
        }
    }
    last_frame_state = &frame->state;

    // Update style based on state
    frame->bg_color = next_frame_dirty.bg_color[state]
        ? next_frame_style.bg_color[state]
        : ui_default_style[type].bg_color[state];
    frame->fg_color = next_frame_dirty.fg_color[state]
        ? next_frame_style.fg_color[state]
        : ui_default_style[type].fg_color[state];

#if UI_DEBUG_RANDOM_COLORS
    frame->bg_color = ui_random_color(frame->index, state);
#endif

    return frame;
}

// TODO: Replace these with ta_ui_push_style that persists until pop
void ta_ui_next_margin(int left, int top, int right, int bottom)
{
#if UI_DEBUG_MARGIN
    next_frame_style.margin.x = left;
    next_frame_style.margin.y = top;
    next_frame_style.margin.w = right;
    next_frame_style.margin.h = bottom;
    next_frame_dirty.margin = true;
#endif
}
void ta_ui_next_pad(int left, int top, int right, int bottom)
{
#if UI_DEBUG_PAD
    next_frame_style.pad.x = left;
    next_frame_style.pad.y = top;
    next_frame_style.pad.w = right;
    next_frame_style.pad.h = bottom;
    next_frame_dirty.pad = true;
#endif
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

ta_ui_state ta_ui_last_frame_state()
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
void ta_ui_window_begin(const char *name, ta_ui_window_state *window, u32 flags)
{
    DLB_ASSERT(window);
    ui_frame_begin(UI_WINDOW, name, window, flags | TA_UI_CONTAINER);
}
void ta_ui_window_end()
{
    ui_frame_end(UI_WINDOW);
}
void ta_ui_panel_begin(const char *name, ta_ui_panel_state *panel, u32 flags)
{
    DLB_ASSERT(panel);
    ui_frame_begin(UI_PANEL, name, panel, flags | TA_UI_CONTAINER);
}
void ta_ui_panel_end()
{
    ui_frame *frame = ui_frame_end(UI_PANEL);
#if UI_DEBUG_PANEL
    if (frame->state.hover) {
        char tex_buf[512] = { 0 };
        int len = snprintf(tex_buf, sizeof(tex_buf),
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
#endif
}
void ta_ui_button_begin(const char *name, u32 flags)
{
    ui_frame_begin(UI_BUTTON, name, 0, flags | TA_UI_CONTAINER);
}
bool ta_ui_button_end()
{
    ui_frame *frame = ui_frame_end(UI_BUTTON);
    return frame->state.pressed;
}
bool ta_ui_button(const char *name)
{
    ui_frame_begin(UI_BUTTON, name, 0, false);
    return ta_ui_button_end();
}
void ta_ui_button_toggle_begin(const char *name, u32 flags)
{
    ui_frame_begin(UI_BUTTON, name, 0, flags | TA_UI_CONTAINER);
}
bool ta_ui_button_toggle_end(bool *checked)
{
    ui_frame *frame = ui_frame_end(UI_BUTTON);
    if (frame->state.pressed) {
        *checked = !*checked;
        frame->state.checked = *checked;
    }
    return *checked;
}
bool ta_ui_button_toggle(const char *name, bool *checked)
{
    ui_frame_begin(UI_BUTTON, name, 0, false);
    return ta_ui_button_toggle_end(checked);
}
bool ta_ui_image(const char *name, ta_texture *texture, int face)
{
#if UI_DEBUG_NO_TEXTURES
    tex = 0;
#endif

    if (texture) {
        if (!next_frame_size.w && !next_frame_size.h) {
            ta_ui_next_size(texture->width, texture->height);
        }
    }

    ui_frame_begin(UI_IMAGE, name, 0, false);
    ui_frame *frame = ui_frame_end(UI_IMAGE);
    frame->texture = texture;
    frame->texture_face = face;
    return frame->state.pressed;
}
bool ta_ui_label(const char *name, const char *text, u32 text_len)
{
    DLB_ASSERT(text);
    DLB_ASSERT(text_len);

    ta_rect_uv *text_rects = 0;
    ta_rectf text_rect = ta_font_push_text(&text_rects, ui_font, text, text_len,
        true, 0, 0, 0, 0);

    // Auto-expand frame based on contents
    ta_ui_next_size(MAX(next_frame_size.w, (int)text_rect.w),
                    MAX(next_frame_size.h, (int)text_rect.h));

    ui_frame_begin(UI_LABEL, name, 0, false);
    ui_frame *frame = ui_frame_end(UI_LABEL);
    frame->text_rects = text_rects;
    return frame->state.pressed;
}
static bool textbox_filter_default(char c)
{
    if ((c >= ui_font->first_char && c <= ui_font->last_char) ||
        c == '\n')
    {
        return true;
    }
    return false;
}
static void textbox_cursor_bof(ta_ui_textbox_state *textbox)
{
    textbox->cursor = 0;
}
static void textbox_cursor_bol(ta_ui_textbox_state *textbox)
{
    while (textbox->cursor && textbox->buffer[textbox->cursor - 1] != '\n') {
        textbox->cursor--;
    }
}
static void textbox_cursor_eof(ta_ui_textbox_state *textbox)
{
    u32 len = dlb_vec_len(textbox->buffer);
    textbox->cursor = len;
}
static void textbox_cursor_eol(ta_ui_textbox_state *textbox)
{
    while (textbox->cursor && textbox->buffer[textbox->cursor + 1] != '\n') {
        textbox->cursor++;
    }
}
static void textbox_cursor_right(ta_ui_textbox_state *textbox)
{
    u32 len = dlb_vec_len(textbox->buffer);
    if (textbox->cursor < len) {
        textbox->cursor++;
    }
}
static void textbox_cursor_left(ta_ui_textbox_state *textbox)
{
    if (textbox->cursor) {
        textbox->cursor--;
    }
}
static void textbox_cursor_down(ta_ui_textbox_state *textbox)
{
    //TODO: Move cursor up
    UNUSED(textbox);
}
static void textbox_cursor_up(ta_ui_textbox_state *textbox)
{
    //TODO: Move cursor down
    UNUSED(textbox);
}
static void textbox_delete(ta_ui_textbox_state *textbox)
{
    // TODO: dlb_vec_remove_at
    u32 len = dlb_vec_len(textbox->buffer);
    if (textbox->cursor < len) {
        dlb_memcpy(
            textbox->buffer + textbox->cursor,
            textbox->buffer + textbox->cursor + 1,
            len - 1 - textbox->cursor
        );
        textbox->buffer[len] = 0;
        dlb_vec_hdr(textbox->buffer)->len--;
    }
}
static void textbox_backspace(ta_ui_textbox_state *textbox)
{
    if (textbox->cursor) {
        textbox->cursor--;
        textbox_delete(textbox);
    }
}
// TODO: Run filter on input string.. maybe?
static void textbox_set_text(ta_ui_textbox_state *textbox, const char *text, u32 text_len)
{
    DLB_ASSERT(text);
    DLB_ASSERT(text_len);

    if (textbox->buffer) {
        dlb_vec_zero(textbox->buffer);
    }
    dlb_vec_reserve(textbox->buffer, text_len);
    dlb_memcpy(textbox->buffer, text, text_len);
    textbox->cursor = text_len;
    textbox->selection_start = 0;
    textbox->selection_len = text_len;
}

ta_textbox_filter *ta_textbox_filter_default = &textbox_filter_default;
bool ta_ui_textbox(const char *name, const char *text, u32 text_len,
    ta_ui_textbox_state *textbox, u32 flags)
{
    DLB_ASSERT(text);
    DLB_ASSERT(text_len);
    DLB_ASSERT(textbox);

    ta_rect_uv *text_rects = 0;
    ta_vec2 cursor = { 0 };
    ta_rectf text_rect = ta_font_push_text(&text_rects, ui_font, text, text_len, true,
        &textbox->cursor, &cursor, 0, 0);

    // Auto-expand frame based on contents
    ta_ui_next_size(MAX(next_frame_size.w, (int)text_rect.w),
                    MAX(next_frame_size.h, (int)text_rect.h));

    ui_frame_begin(UI_TEXTBOX, name, textbox, flags);
    ui_frame *frame = ui_frame_end(UI_TEXTBOX);

    // Textbox is in edit mode
    if (textbox->buffer) {
        if (ta_keybind_down(&textbox_keybinds[TEXTBOX_EVENT_NEWLINE])) {
            // TODO: Handle multiline correctly, can't be same hotkey as submit
            //if (textbox->multiline) {
            //    textbox_insert(textbox, '\n');
            //}
        } else if (ta_keybind_down(&textbox_keybinds[TEXTBOX_EVENT_SUBMIT])) {
            //ta_textbox_submit(textbox);
            frame->state.submit = true;
        } else if (ta_keybind_down(&textbox_keybinds[TEXTBOX_EVENT_CANCEL])) {
            //ta_textbox_cancel(textbox);
            frame->state.cancel = true;
        } else if (ta_keybind_down(&textbox_keybinds[TEXTBOX_EVENT_BACKSPACE])) {
            textbox_backspace(textbox);
        } else if (ta_keybind_down(&textbox_keybinds[TEXTBOX_EVENT_CURSOR_BOF])) {
            textbox_cursor_bof(textbox);
        } else if (ta_keybind_down(&textbox_keybinds[TEXTBOX_EVENT_CURSOR_BOL])) {
            textbox_cursor_bol(textbox);
        } else if (ta_keybind_down(&textbox_keybinds[TEXTBOX_EVENT_CURSOR_EOF])) {
            textbox_cursor_eof(textbox);
        } else if (ta_keybind_down(&textbox_keybinds[TEXTBOX_EVENT_CURSOR_EOL])) {
            textbox_cursor_eol(textbox);
        } else if (ta_keybind_down(&textbox_keybinds[TEXTBOX_EVENT_DELETE])) {
            textbox_delete(textbox);
        } else if (ta_keybind_down(&textbox_keybinds[TEXTBOX_EVENT_CURSOR_RIGHT])) {
            textbox_cursor_right(textbox);
        } else if (ta_keybind_down(&textbox_keybinds[TEXTBOX_EVENT_CURSOR_LEFT])) {
            textbox_cursor_left(textbox);
        } else if (ta_keybind_down(&textbox_keybinds[TEXTBOX_EVENT_CURSOR_DOWN])) {
            textbox_cursor_down(textbox);
        } else if (ta_keybind_down(&textbox_keybinds[TEXTBOX_EVENT_CURSOR_UP])) {
            textbox_cursor_up(textbox);
        }
    } else if (frame->state.down) {
        // Enter edit mode
        textbox_set_text(textbox, text, text_len);
    }

    // Focus/unfocus textbox
    if (frame->state.down) {
        frame->state.focused = true;
    } else if (ta_key_pressed(SDL_SCANCODE_MOUSE_LEFT)) {
        frame->state.focused = false;  // User clicked elsewhere
    }

    frame->text_rects = text_rects;
    frame->cursor = cursor;
    return frame->state.submit;
}
bool ta_ui_textbox_insert(ta_ui_textbox_state *textbox, char c)
{
    DLB_ASSERT(textbox);
    DLB_ASSERT(textbox->buffer);

    if (!textbox->filter) {
        textbox->filter = ta_textbox_filter_default;
    }
    if (!textbox->filter(c)) {
        return false;
    }

    // TODO: dlb_vec_insert_at
    u32 len = dlb_vec_len(textbox->buffer);
    dlb_vec_reserve(textbox->buffer, len + 1);
    dlb_memcpy(
        textbox->buffer + textbox->cursor + 1,
        textbox->buffer + textbox->cursor,
        len - textbox->cursor
    );
    textbox->buffer[textbox->cursor] = c;
    textbox->cursor++;
    return true;
}
void ta_ui_vec3(ta_vec3 *vec)
{
    char x_str[16] = { 0 };
    int x_len = snprintf(x_str, sizeof(x_str), "%3.4f", vec->x);
    DLB_ASSERT(x_len < sizeof(x_str));
    ta_ui_label(0, CSTR("x:"));
    static ta_ui_textbox_state entry_x = { 0 };
    ta_ui_textbox(0, x_str, x_len, &entry_x, 0);

    char y_str[16] = { 0 };
    int y_len = snprintf(y_str, sizeof(y_str), "%3.4f", vec->y);
    DLB_ASSERT(y_len < sizeof(y_str));
    ta_ui_label(0, CSTR("y:"));
    static ta_ui_textbox_state entry_y = { 0 };
    ta_ui_textbox(0, y_str, y_len, &entry_y, 0);

    char z_str[16] = { 0 };
    int z_len = snprintf(z_str, sizeof(z_str), "%3.4f", vec->z);
    DLB_ASSERT(z_len < sizeof(z_str));
    ta_ui_label(0, CSTR("z:"));
    static ta_ui_textbox_state entry_z = { 0 };
    ta_ui_textbox(0, z_str, z_len, &entry_z, 0);
}
void ta_ui_tooltip(const char *text, u32 text_len)
{
    ta_rect_uv *text_rects = 0;
    ta_rectf text_rect = ta_font_push_text(&text_rects, ui_font, text, text_len,
        true, 0, 0, 0, 0);

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
#if 0
// TODO: This doesn't make any sense.. where does text come from and why are the
// rects going into tooltip_bg queue? Might want a statusbar but NotLikeThis.
void ta_ui_statusbar()
{
    // TODO: Move this font to editor.dml or font.dml
    ta_font *font = ta_game_by_name(RES_FONT, tg_font);
    const float statusbar_pad = 4;
    ta_rect_uv status_bg = { 0 };
    status_bg.rect.x = statusbar_pad;
    status_bg.rect.y = -(float)(font->line_height + statusbar_pad);
    status_bg.rect.w = (float)(WINDOW_W - statusbar_pad * 2);
    status_bg.rect.h = (float)font->line_height;
    ta_primitive_push_rect_uv(&tooltip_bg_queue, status_bg, TA_COLOR_GRAY3A,
        UI_LAYER_TIP_BG, true, false);
}
#endif

static void ui_render_window(ui_frame *frame)
{
    // Render window background
    ta_rect bg_rect = frame->rect;
    bg_rect.w = frame->rect.w;
    ta_primitive_push_rect(bg_rect, frame->bg_color, UI_LAYER_EDIT_WINDOW_BG);
    ta_primitive_render_quads(quads_queue, tg_shader_quads, true, true);
}
static void ui_render_panel(ui_frame *frame)
{
    ta_primitive_push_rect(frame->rect, frame->bg_color, UI_LAYER_EDIT_1_BG);
    ta_primitive_render_quads(quads_queue, tg_shader_quads, true, true);
}
static void ui_render_button(ui_frame *frame)
{
    ta_primitive_push_rect(frame->rect, frame->bg_color, UI_LAYER_EDIT_1_BG);
    ta_primitive_render_quads(quads_queue, tg_shader_quads, true, true);
}
static void ui_render_button_toggle(ui_frame *frame)
{
    ta_rgba bg_color = frame->bg_color;
    if (frame->state.checked) {
        bg_color = ui_default_style[frame->type].bg_color[UI_STATE_ACTIVE];
    }

    ta_primitive_push_rect(frame->rect, bg_color, UI_LAYER_EDIT_1_BG);
    ta_primitive_render_quads(quads_queue, tg_shader_quads, true, true);
}
static void ui_render_image(ui_frame *frame)
{
    if (frame->texture) {
        if (frame->texture->cubemap) {
            DLB_ASSERT(frame->texture_face >= 0 && frame->texture_face <= 6);
            ta_shader_set_sampler_cube(tg_shader_cubemap, SYM_U_TEX, frame->texture->gl_id);
            ta_shader_set_int(tg_shader_cubemap, SYM_U_FACE, frame->texture_face);
            ta_rect img_rect = rect_shrink(frame->rect, frame->pad);
            ta_primitive_push_rect(img_rect, TA_COLOR_INVIS, UI_LAYER_EDIT_1);
            ta_primitive_render_quads(quads_queue, tg_shader_cubemap, true, true);
            ta_shader_set_sampler_cube(tg_shader_cubemap, SYM_U_TEX, 0);
        } else {
            ta_shader_set_mat4(tg_shader_quads, SYM_U_PROJ, &MAT4_IDENT);
            ta_shader_set_mat4(tg_shader_quads, SYM_U_VIEW, &MAT4_IDENT);
            ta_shader_set_mat4(tg_shader_quads, SYM_U_MODEL, &MAT4_IDENT);
            ta_shader_set_sampler2d(tg_shader_quads, SYM_U_TEX, frame->texture->gl_id);
            ta_rect img_rect = rect_shrink(frame->rect, frame->pad);
            ta_primitive_push_rect(img_rect, TA_COLOR_INVIS, UI_LAYER_EDIT_1);
            ta_primitive_render_quads(quads_queue, tg_shader_quads, true, true);
            ta_shader_set_sampler2d(tg_shader_quads, SYM_U_TEX, 0);
        }
    }
}
static void ui_render_text(float x, float y, ta_rect_uv *text_rects)
{
    if (dlb_vec_len(text_rects)) {
        dlb_vec_each(ta_rect_uv *, rect, text_rects) {
            ta_primitive_push_rect_uv(&quads_queue, *rect, TA_COLOR_WHITE, 0,
                true, false);
        }

        ta_font_render(quads_queue, ui_font, x, y, UI_LAYER_EDIT_1, true, true);
    }
}
static void ui_render_label(ui_frame *frame)
{
    // Render background
    if (frame->bg_color.a == 0.0f) {
        frame->bg_color.a = TA_EPSILON;
    }
    ta_primitive_push_rect(frame->rect, frame->bg_color, UI_LAYER_EDIT_1_BG);
    ta_primitive_render_quads(quads_queue, tg_shader_quads, true, true);

    // Render text
    float x = (float)frame->rect.x + frame->pad.x;
    float y = (float)frame->rect.y + frame->pad.y;
    ui_render_text(x, y, frame->text_rects);
}
static void ui_render_textbox(ui_frame *frame)
{
    // Render background
    ta_rgba bg_color = frame->state.focused ? TA_COLOR_BLUE3 : TA_COLOR_BLUE2;
    ta_primitive_push_rect(frame->rect, bg_color, UI_LAYER_EDIT_1_BG);
    ta_primitive_render_quads(quads_queue, tg_shader_quads, true, true);

    // Render text
    int x = frame->rect.x + frame->pad.x;
    int y = frame->rect.y + frame->pad.y;
    ui_render_text((float)x, (float)y, frame->text_rects);

    // If active, render cursor
    if (frame->state.focused) {
        ta_rect cursor_rect = { 0 };
        cursor_rect.x = x + (int)frame->cursor.x;
        cursor_rect.y = y + (int)frame->cursor.y + 1;
        cursor_rect.w = 1;
        cursor_rect.h = ui_font->line_height - 2;
        ta_primitive_push_rect(cursor_rect, TA_COLOR_GRAY8, UI_LAYER_EDIT_2);
        ta_primitive_render_quads(quads_queue, tg_shader_quads, true, true);
    }
}
static void ui_render_scrollbars(ui_frame *frame)
{
    if (frame->scroll_h.rect.h) {
        ta_primitive_push_rect(frame->scroll_h.rect, TA_COLOR_BLUE,
            UI_LAYER_EDIT_1);
        ta_primitive_push_rect(frame->scroll_h.widget, (ta_rgba){ 0.6f, 0.0f, 0.0f, 1.0f },
            UI_LAYER_EDIT_1);
        ta_primitive_render_quads(quads_queue, tg_shader_quads, true, true);
    }

    if (frame->scroll_v.rect.w) {
        ta_primitive_push_rect(frame->scroll_v.rect, TA_COLOR_BLUE,
            UI_LAYER_EDIT_1);
        ta_primitive_push_rect(frame->scroll_v.widget, (ta_rgba){ 0.6f, 0.0f, 0.0f, 1.0f },
            UI_LAYER_EDIT_1);
        ta_primitive_render_quads(quads_queue, tg_shader_quads, true, true);
    }
}

void ta_ui_render()
{
    static void (*ui_renderers[])(ui_frame *frame) = {
        [UI_ROOT]    = 0,
        [UI_WINDOW]  = ui_render_window,
        [UI_PANEL]   = ui_render_panel,
        [UI_BUTTON]  = ui_render_button,
        [UI_IMAGE]   = ui_render_image,
        [UI_LABEL]   = ui_render_label,
        [UI_TEXTBOX] = ui_render_textbox,
    };

    // TODO: Shouldn't have to do this.. not sure where it's being bound
    ta_shader_set_sampler2d(tg_shader_quads, SYM_U_TEX, 0);

    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_SCISSOR_TEST);

    ta_vec2i scroll_offset = { 0 };

    dlb_vec_each(ui_frame *, frame, ui_frames) {
        if (frame->internal_flags & TA_UI_INVISIBLE || !ui_renderers[frame->type]) {
            continue;
        }

        // NOTE: Can't do this, because will be desync'd with all layout pass
        // (e.g. tooltip locations and mouse hover, etc.). Need to accumulate
        // scroll_offset in all containers during layout pass, then apply to
        // children controls.. somehow.
        //frame->rect.x -= scroll_offset.x;
        //frame->rect.y -= scroll_offset.y;

        // should be 464, but is 528

        frame->clip_rect = TA_RECT_ZERO;
        frame->clip_rect.w = WINDOW_W;
        frame->clip_rect.h = WINDOW_H;
        ui_frame *f = frame->container;
        while(f->index) {
            if (f->data.ptr) {
                int overflow_y = f->rect.h - f->content_size.h;
                ta_ui_scroll_state *scroll = ui_scroll_state(f);
                frame->rect.y += (int)(scroll->scroll_pct.y * overflow_y);
            }
            frame->clip_rect = rect_intersect(frame->clip_rect, f->rect);
            f = f->container;
            DLB_ASSERT(f->index < dlb_vec_len(ui_frames));
        }
        int inv_y = WINDOW_H - (frame->clip_rect.y + frame->clip_rect.h);
        glScissor(frame->clip_rect.x, inv_y, frame->clip_rect.w, frame->clip_rect.h);

        ui_renderers[frame->type](frame);
        ui_render_scrollbars(frame);

        // Free any per-frame memory
        dlb_vec_free(frame->text_rects);
    }

    glDisable(GL_SCISSOR_TEST);

    // Render tooltips
    ta_primitive_render_quads(tooltip_bg_queue, tg_shader_quads, true, true);
    ta_font_render(tooltip_fg_queue, ui_font, 0, 0, UI_LAYER_TIP, true, true);

    last_frame_state = 0;
    dlb_vec_zero(ui_frames);
    dlb_vec_alloc(ui_frames);  // reserve UI_ROOT
}