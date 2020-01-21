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
#include "misc/gl3w.h"
#include "SDL/SDL_keyboard.h"
#include "SDL/SDL_mouse.h"

#define UI_DEBUG_PANEL          0
#define UI_DEBUG_NO_TEXTURES    0
#define UI_DEBUG_RANDOM_COLORS  0

#define WIDGET_PAD              1
#define SCROLL_WIDGET_THICKNESS 8
#define SCROLL_WIDGET_H_MIN     4
#define SCROLL_WHEEL_SPEED      17

// internal flags
#define TA_UI_INVISIBLE         0x20000000  // takes up space but doesn't render (display: hidden)
#define TA_UI_CONTAINER         0x40000000  // [internal] will always be set on containers
#define TA_UI_CONTAINER_ENDED   0x80000000  // [internal] will be set when container ends

static const double key_repeat_delay_ms = 200;
static const double key_repeat_interval_ms = 25;
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
    bool margin;
    bool pad;
    bool bg_color[UI_STATE_COUNT];
    bool fg_color[UI_STATE_COUNT];
    bool invisible;
} next_frame_dirty;

typedef struct ui_frame {
    u32 index;
    u32 container_idx;
    ui_frame_type type;

    ta_rect margin;
    ta_rect pad;
    ta_rgba bg_color[UI_STATE_COUNT];
    ta_rgba fg_color[UI_STATE_COUNT];
    ta_rect rect;           // position & size (-margin, +pad)
    ta_vec2i offset;        // dynamic offset for layout
    bool skip_flow;         // if true, doesn't affect flow of parent container

    int row_height;         // height of current layout row
    bool row_continue;      // if true, next element will layout on same row
    ta_size content_size;   // dynamic content size (-margin, +pad)
    ta_rect clip_rect;

    ta_rect_uv *text_rects; // vector, must be freed!
    ta_texture *texture;
    int texture_face;       // for cubemaps
    ta_vec2 cursor;         // cursor location for textboxes

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

typedef enum ui_textbox_command {
    TEXTBOX_COMMAND_CURSOR_RIGHT,
    TEXTBOX_COMMAND_CURSOR_LEFT,
    TEXTBOX_COMMAND_CURSOR_DOWN,
    TEXTBOX_COMMAND_CURSOR_UP,
    TEXTBOX_COMMAND_CURSOR_BOL,
    TEXTBOX_COMMAND_CURSOR_EOL,
    TEXTBOX_COMMAND_CURSOR_BOF,
    TEXTBOX_COMMAND_CURSOR_EOF,
    TEXTBOX_COMMAND_DELETE,
    TEXTBOX_COMMAND_BACKSPACE,
    TEXTBOX_COMMAND_SUBMIT1,
    TEXTBOX_COMMAND_SUBMIT2,
    TEXTBOX_COMMAND_CANCEL,
    TEXTBOX_COMMAND_COUNT
} ui_textbox_command;

// Internal state
static ta_font *ui_font;
static ta_ui_textbox_state **ui_textbox_editing;
static ta_ui_textbox_state **ui_textbox_dragging;
static SDL_Cursor *ui_cursor_arrow;    // normal mouse pointer
static SDL_Cursor *ui_cursor_size_we;  // left/right arrow "<->" cursor
static SDL_Cursor *ui_cursor_ibeam;     // text edit ibeam "I" cursor
static SDL_Cursor *ui_active_cursor;    // current cursor being used
static bool ui_active_cursor_changed;   // (SetCursor is *expensive*)

static ui_style ui_default_style[UI_COUNT] = { 0 };
static ta_vec2i next_frame_pos_relative;
static ta_size next_frame_size;
static ui_style next_frame_style;
static ta_ui_state *last_frame_state;

static ui_frame *ui_frames;

static ta_keybind textbox_keybinds[TEXTBOX_COMMAND_COUNT] = { 0 };

static void ui_row_end(ui_frame *container);

// active_textbox is an external pointer that the ui code will keep updated
// for you automatically when focus changes.
void ta_ui_init(ta_font *font, ta_ui_textbox_state **textbox_editing,
    ta_ui_textbox_state **textbox_dragging)
{
    DLB_ASSERT(font);
    DLB_ASSERT(textbox_editing);
    DLB_ASSERT(textbox_dragging);

    ui_font = font;
    ui_textbox_editing = textbox_editing;
    ui_textbox_dragging = textbox_dragging;
    ui_cursor_arrow = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
    ui_cursor_size_we = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEWE);
    ui_cursor_ibeam = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_IBEAM);

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
    ui_default_style[UI_PANEL].bg_color[UI_STATE_NONE]      = TA_COLOR_GRAY2; //TA_RGBA(0.7f, 0.7f, 0.7f, 0.4f);
    ui_default_style[UI_PANEL].bg_color[UI_STATE_HOVER]     = TA_COLOR_GRAY2; //TA_RGBA(0.7f, 0.7f, 0.7f, 0.4f);
    ui_default_style[UI_PANEL].bg_color[UI_STATE_DOWN]      = TA_COLOR_GRAY2; //TA_RGBA(0.7f, 0.7f, 0.7f, 0.4f);
    ui_default_style[UI_PANEL].bg_color[UI_STATE_ACTIVE]    = TA_COLOR_GRAY2; //TA_RGBA(0.7f, 0.7f, 0.7f, 0.4f);
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
    ui_default_style[UI_TOGGLE_BUTTON].pad                         = TA_RECT(2, 2, 2, 2);
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

    ta_keybind_init1(&textbox_keybinds[TEXTBOX_COMMAND_CURSOR_RIGHT], TA_KEYBIND_HOLD,    SDL_SCANCODE_RIGHT);
    ta_keybind_init1(&textbox_keybinds[TEXTBOX_COMMAND_CURSOR_LEFT],  TA_KEYBIND_HOLD,    SDL_SCANCODE_LEFT);
    ta_keybind_init1(&textbox_keybinds[TEXTBOX_COMMAND_CURSOR_DOWN],  TA_KEYBIND_PRESS,   SDL_SCANCODE_DOWN);
    ta_keybind_init1(&textbox_keybinds[TEXTBOX_COMMAND_CURSOR_UP],    TA_KEYBIND_PRESS,   SDL_SCANCODE_UP);
    ta_keybind_init1(&textbox_keybinds[TEXTBOX_COMMAND_CURSOR_BOL],   TA_KEYBIND_PRESS,   SDL_SCANCODE_HOME);
    ta_keybind_init1(&textbox_keybinds[TEXTBOX_COMMAND_CURSOR_EOL],   TA_KEYBIND_PRESS,   SDL_SCANCODE_END);
    ta_keybind_init2(&textbox_keybinds[TEXTBOX_COMMAND_CURSOR_BOF],   TA_KEYBIND_PRESS,   SDL_SCANCODE_LSHIFT, SDL_SCANCODE_HOME);
    ta_keybind_init2(&textbox_keybinds[TEXTBOX_COMMAND_CURSOR_EOF],   TA_KEYBIND_PRESS,   SDL_SCANCODE_LSHIFT, SDL_SCANCODE_END);
    ta_keybind_init1(&textbox_keybinds[TEXTBOX_COMMAND_DELETE],       TA_KEYBIND_HOLD,    SDL_SCANCODE_DELETE);
    ta_keybind_init1(&textbox_keybinds[TEXTBOX_COMMAND_BACKSPACE],    TA_KEYBIND_HOLD,    SDL_SCANCODE_BACKSPACE);
    ta_keybind_init1(&textbox_keybinds[TEXTBOX_COMMAND_SUBMIT1],      TA_KEYBIND_PRESS,   SDL_SCANCODE_RETURN);
    ta_keybind_init1(&textbox_keybinds[TEXTBOX_COMMAND_SUBMIT2],      TA_KEYBIND_PRESS,   SDL_SCANCODE_KP_ENTER);
    ta_keybind_init1(&textbox_keybinds[TEXTBOX_COMMAND_CANCEL],       TA_KEYBIND_RELEASE, SDL_SCANCODE_ESCAPE);
}
void ta_ui_set_font(ta_font *font)
{
    DLB_ASSERT(font);
    ui_font = font;
}
void ui_set_cursor(SDL_Cursor *cursor)
{
    if (ui_active_cursor == cursor) return;
    ui_active_cursor = cursor;
    ui_active_cursor_changed = true;
}
void ta_ui_set_cursor(ui_cursor_type cursor_type)
{
    SDL_Cursor *cursor = ui_cursor_arrow;
    switch (cursor_type) {
        case UI_CURSOR_ARROW: {
            cursor = ui_cursor_arrow;
            break;
        } case UI_CURSOR_SIZEWE: {
            cursor = ui_cursor_size_we;
            break;
        } case UI_CURSOR_IBEAM: {
            cursor = ui_cursor_ibeam;
            break;
        } default: {
            DLB_ASSERT(!"Need to handle this cursor type");
        }
    };
    ui_set_cursor(cursor);
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
static u32 ui_container(u32 frame_idx)
{
    DLB_ASSERT(dlb_vec_len(ui_frames));
    DLB_ASSERT(frame_idx <= dlb_vec_len(ui_frames));

    int idx = 0;

    // note: UI_ROOT is its own parent
    if (frame_idx) {
        // TODO: Could just keep track of most recent container? Idk.. this most
        // likely only runs for 1-2 iterations worst case right now.
        for (int i = frame_idx - 1; i >= 0; i--) {
            if (ui_frames[i].internal_flags & TA_UI_CONTAINER &&
                !(ui_frames[i].internal_flags & TA_UI_CONTAINER_ENDED))
            {
                idx = i;
                break;
            }
        }
    }

    DLB_ASSERT(idx >= 0 && idx < (int)dlb_vec_len(ui_frames));
    return idx;
}
static inline ui_frame *ui_container_last()
{
    u32 container_idx = ui_container(dlb_vec_len(ui_frames));
    return &ui_frames[container_idx];
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
static void ui_frame_begin(ui_frame_type type, void *data, u32 flags)
{
    // Allocate frame
    ui_frame *frame = dlb_vec_alloc(ui_frames);
    frame->index = dlb_vec_len(ui_frames) - 1;
    frame->type = type;
    frame->data.ptr = data;
    frame->internal_flags = flags;

    frame->margin = next_frame_dirty.margin
        ? next_frame_style.margin
        : ui_default_style[type].margin;
    frame->pad = next_frame_dirty.pad
        ? next_frame_style.pad
        : ui_default_style[type].pad;

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
            frame->rect.w += SCROLL_WIDGET_THICKNESS;
        }
        if (frame->content_size.w > frame->rect.w) {
            frame->rect.h += SCROLL_WIDGET_THICKNESS;
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
void ta_ui_next_margin(int left, int top, int right, int bottom)
{
    next_frame_style.margin.x = left;
    next_frame_style.margin.y = top;
    next_frame_style.margin.w = right;
    next_frame_style.margin.h = bottom;
    next_frame_dirty.margin = true;
}
void ta_ui_next_pad(int left, int top, int right, int bottom)
{
    next_frame_style.pad.x = left;
    next_frame_style.pad.y = top;
    next_frame_style.pad.w = right;
    next_frame_style.pad.h = bottom;
    next_frame_dirty.pad = true;
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
void ta_ui_button_begin(u32 flags)
{
    ui_frame_begin(UI_BUTTON, 0, flags | TA_UI_CONTAINER);
}
bool ta_ui_button_end()
{
    ui_frame *frame = ui_frame_end(UI_BUTTON);
    return frame->state.pressed;
}
bool ta_ui_button(const char *text, u32 text_len)
{
    ta_ui_next_pad(0, 0, 0, 0);
    ta_ui_button_begin(TA_UI_AUTOSIZE);
    ta_ui_next_margin(0, 0, 0, 0);
    ta_ui_next_bg_color(UI_STATE_ALL, 0, 0, 0, 0);
    ta_ui_label(text, text_len);
    return ta_ui_button_end();
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
bool ta_ui_toggle_button(const char *text, u32 text_len, bool *checked)
{
    //ta_ui_next_pad(0, 0, 0, 0);
    ta_ui_toggle_button_begin(TA_UI_AUTOSIZE);
    ta_ui_next_margin(0, 0, 0, 0);
    ta_ui_next_bg_color(UI_STATE_ALL, 0, 0, 0, 0);
    ta_ui_label(text, text_len);
    return ta_ui_toggle_button_end(checked);
}
bool ta_ui_image(ta_texture *texture, int face)
{
#if UI_DEBUG_NO_TEXTURES
    tex = 0;
#endif

    if (texture) {
        if (!next_frame_size.w && !next_frame_size.h) {
            ta_ui_next_size(texture->width, texture->height);
        }
    }

    ui_frame_begin(UI_IMAGE, 0, false);
    ui_frame *frame = ui_frame_end(UI_IMAGE);
    frame->texture = texture;
    frame->texture_face = face;
    return frame->state.pressed;
}
void ta_ui_label(const char *text, u32 text_len)
{
    DLB_ASSERT(text);
    DLB_ASSERT(text_len);

    ta_rect_uv *text_rects = 0;
    ta_rectf text_rect = ta_font_push_text(&text_rects, ui_font, text, text_len,
        true, 0, 0, 0);

    // Auto-expand frame based on contents
    ta_ui_next_size(MAX(next_frame_size.w, (int)text_rect.w),
                    MAX(next_frame_size.h, (int)text_rect.h));

    ui_frame_begin(UI_LABEL, 0, false);
    ui_frame *frame = ui_frame_end(UI_LABEL);
    frame->text_rects = text_rects;
    //return frame->state.pressed;
}

static bool textbox_repeat_valid(double *last_time, bool *repeating,
    const ta_keybind *keybind)
{
    DLB_ASSERT(last_time);
    DLB_ASSERT(repeating);
    DLB_ASSERT(keybind);

    bool first = ta_keybind_pressed(keybind);
    double timer_ms = ta_timer_elapsed_ms();
    double delta_ms = timer_ms - *last_time;

    if (first ||
        (!*repeating && delta_ms >= key_repeat_delay_ms) ||
        (*repeating && delta_ms >= key_repeat_interval_ms))
    {
        *last_time = timer_ms;
        *repeating = !first;
        return true;
    }
    return false;
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
static void textbox_command_cursor_right(ta_ui_textbox_state *textbox)
{
    static double last_repeat_ms = 0;
    static bool repeating = false;

    if (textbox_repeat_valid(&last_repeat_ms, &repeating,
        &textbox_keybinds[TEXTBOX_COMMAND_CURSOR_RIGHT]))
    {
        u32 len = dlb_vec_len(textbox->buffer);
        if (textbox->cursor < len) {
            textbox->cursor++;
        }
    }
}
static void textbox_command_cursor_left(ta_ui_textbox_state *textbox)
{
    static double last_repeat_ms = 0;
    static bool repeating = false;

    if (textbox_repeat_valid(&last_repeat_ms, &repeating,
        &textbox_keybinds[TEXTBOX_COMMAND_CURSOR_LEFT]))
    {
        if (textbox->cursor) {
            textbox->cursor--;
        }
    }
}
static void textbox_command_cursor_down(ta_ui_textbox_state *textbox)
{
    //TODO: Move cursor up
    UNUSED(textbox);
}
static void textbox_command_cursor_up(ta_ui_textbox_state *textbox)
{
    //TODO: Move cursor down
    UNUSED(textbox);
}
static void textbox_command_cursor_bol(ta_ui_textbox_state *textbox)
{
    while (textbox->cursor && textbox->buffer[textbox->cursor - 1] != '\n') {
        textbox->cursor--;
    }
}
static void textbox_command_cursor_eol(ta_ui_textbox_state *textbox)
{
    u32 len = dlb_vec_len(textbox->buffer);
    while (textbox->cursor < len && textbox->buffer[textbox->cursor + 1] != '\n') {
        textbox->cursor++;
    }
}
static void textbox_command_cursor_bof(ta_ui_textbox_state *textbox)
{
    textbox->cursor = 0;
}
static void textbox_command_cursor_eof(ta_ui_textbox_state *textbox)
{
    u32 len = dlb_vec_len(textbox->buffer);
    textbox->cursor = len;
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
        textbox->buffer[len - 1] = 0;
        dlb_vec_hdr(textbox->buffer)->len--;
    }
}
static void textbox_command_delete(ta_ui_textbox_state *textbox)
{
    static double last_repeat_ms = 0;
    static bool repeating = false;

    if (textbox_repeat_valid(&last_repeat_ms, &repeating,
        &textbox_keybinds[TEXTBOX_COMMAND_DELETE]))
    {
        textbox_delete(textbox);
    }
}
static void textbox_command_backspace(ta_ui_textbox_state *textbox)
{
    static double last_repeat_ms = 0;
    static bool repeating = false;

    if (textbox_repeat_valid(&last_repeat_ms, &repeating,
        &textbox_keybinds[TEXTBOX_COMMAND_BACKSPACE]))
    {
        if (textbox->cursor) {
            textbox->cursor--;
            textbox_delete(textbox);
        }
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
static void textbox_command_submit(ta_ui_textbox_state *textbox)
{
    textbox->submit = true;
    // TODO: Unfocus and free on client's request, after they've been able to
    // use the buffer contents for whatever they need.
    textbox_unfocus(textbox);
}
static void textbox_command_cancel(ta_ui_textbox_state *textbox)
{
    textbox->submit = false;
    dlb_vec_free(textbox->buffer);
    textbox_unfocus(textbox);
}

ta_textbox_filter *ta_textbox_filter_default = &textbox_filter_default;
static void (*textbox_commands[TEXTBOX_COMMAND_COUNT])(ta_ui_textbox_state *textbox) = {
    [TEXTBOX_COMMAND_CURSOR_RIGHT] = textbox_command_cursor_right,
    [TEXTBOX_COMMAND_CURSOR_LEFT]  = textbox_command_cursor_left,
    [TEXTBOX_COMMAND_CURSOR_DOWN]  = textbox_command_cursor_down,
    [TEXTBOX_COMMAND_CURSOR_UP]    = textbox_command_cursor_up,
    [TEXTBOX_COMMAND_CURSOR_BOL]   = textbox_command_cursor_bol,
    [TEXTBOX_COMMAND_CURSOR_EOL]   = textbox_command_cursor_eol,
    [TEXTBOX_COMMAND_CURSOR_BOF]   = textbox_command_cursor_bof,
    [TEXTBOX_COMMAND_CURSOR_EOF]   = textbox_command_cursor_eof,
    [TEXTBOX_COMMAND_DELETE]       = textbox_command_delete,
    [TEXTBOX_COMMAND_BACKSPACE]    = textbox_command_backspace,
    [TEXTBOX_COMMAND_SUBMIT1]      = textbox_command_submit,
    [TEXTBOX_COMMAND_SUBMIT2]      = textbox_command_submit,
    [TEXTBOX_COMMAND_CANCEL]       = textbox_command_cancel,
};

// TODO: Run filter on input string.. maybe?
static void textbox_set_text(ta_ui_textbox_state *textbox, const char *text, u32 text_len)
{
    if (textbox->buffer) {
        dlb_vec_zero(textbox->buffer);
    }
    dlb_vec_reserve(textbox->buffer, text_len + 1);  // reserve 1 extra for nil
    dlb_memcpy(textbox->buffer, text, text_len);
    dlb_vec_hdr(textbox->buffer)->len = text_len;

    // Ensure buffer is nil-terminated
    u32 new_len = dlb_vec_len(textbox->buffer);
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

bool ta_ui_textbox(const char *text, u32 text_len, ta_ui_textbox_state *textbox,
    u32 flags)
{
    //DLB_ASSERT(text);
    //DLB_ASSERT(text_len);
    DLB_ASSERT(textbox);

    ta_rect_uv *text_rects = 0;
    ta_vec2 cursor = { 0 };
    ta_rectf text_rect = { 0 };

    if (textbox->buffer) {
        ta_vec2i *mouse_coords = 0;
        if (textbox->mouse_down) {
            mouse_coords = &textbox->mouse_coords;
            textbox->mouse_down = false;
        }

        // If still editing, render buffer
        u32 buffer_len = dlb_vec_len(textbox->buffer);
        text_rect = ta_font_push_text(&text_rects, ui_font, textbox->buffer,
            buffer_len, true, &textbox->cursor, &cursor, mouse_coords);
    } else if (text) {
        // If not editing (or just canceled), render text
        text_rect = ta_font_push_text(&text_rects, ui_font, text, text_len,
            true, 0, 0, 0);
    }

    // Auto-expand frame based on contents
    ta_ui_next_size(MAX(next_frame_size.w, (int)text_rect.w),
                    MAX(next_frame_size.h, (int)text_rect.h));

    ui_frame_begin(UI_TEXTBOX, textbox, flags);
    ui_frame *frame = ui_frame_end(UI_TEXTBOX);

    if (textbox->focused) {
        // Textbox is active, handle hotkeys
        for (ui_textbox_command cmd = 0; cmd < TEXTBOX_COMMAND_COUNT; ++cmd) {
            ta_keybind_update(&textbox_keybinds[cmd]);
            if (ta_keybind_triggered(&textbox_keybinds[cmd])) {
                textbox_commands[cmd](textbox);
            }
        }
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
            textbox_command_submit(textbox);
        } else if (ta_key_pressed(SDL_SCANCODE_MOUSE_RIGHT)) {
            // TODO(cleanup): Right click usually means cancel, but in the case
            // of search box I want to right click to rotate camera without
            // losing my search results, sooo...
            textbox_command_submit(textbox);
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
        ta_ui_next_size(80, 0);
    }

    if (textbox->submit) {
        ta_ui_textbox_clear(textbox);
    }

    ta_rect_uv *text_rects = 0;
    ta_vec2 cursor = { 0 };
    ta_rectf text_rect = { 0 };

    if (textbox->buffer) {
        ta_vec2i *mouse_coords = 0;
        if (textbox->mouse_down) {
            mouse_coords = &textbox->mouse_coords;
            textbox->mouse_down = false;
        }

        // If still editing, render buffer
        u32 buffer_len = dlb_vec_len(textbox->buffer);
        text_rect = ta_font_push_text(&text_rects, ui_font, textbox->buffer,
            buffer_len, true, &textbox->cursor, &cursor, mouse_coords);
    } else {
        // If not editing (or just canceled), render text
        char text[16] = { 0 };
        int text_len = snprintf(CSTR(text), "%.3f", *value);
        DLB_ASSERT(text_len < sizeof(text));
        text_rect = ta_font_push_text(&text_rects, ui_font, text, text_len,
            true, 0, 0, 0);
    }

    // Auto-expand frame based on contents
    ta_ui_next_size(MAX(next_frame_size.w, (int)text_rect.w),
        MAX(next_frame_size.h, (int)text_rect.h));

    ui_frame_begin(UI_TEXTBOX, textbox, flags);
    ui_frame *frame = ui_frame_end(UI_TEXTBOX);

    if (textbox->focused) {
        // Textbox is active, handle hotkeys
        for (ui_textbox_command cmd = 0; cmd < TEXTBOX_COMMAND_COUNT; ++cmd) {
            ta_keybind_update(&textbox_keybinds[cmd]);
            if (ta_keybind_triggered(&textbox_keybinds[cmd])) {
                textbox_commands[cmd](textbox);
            }
        }
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
            textbox_command_submit(textbox);
        } else if (ta_key_pressed(SDL_SCANCODE_MOUSE_RIGHT)) {
            textbox_command_cancel(textbox);
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
                    int text_len = snprintf(CSTR(text), "%.3f", *value);
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
                ta_keybind_update(&textbox_keybinds[TEXTBOX_COMMAND_CANCEL]);
                if (ta_keybind_triggered(&textbox_keybinds[TEXTBOX_COMMAND_CANCEL])) {
                    drag_float_cancel();
                } else {
                    frame->state_type = UI_STATE_ACTIVE;
                }
            }
        }
    }

    if (frame->data.textbox->submit) {
        *value = parse_float(textbox->buffer);
        ta_ui_textbox_clear(textbox);
    } else if (ta_ui_last_frame_state().hover && !textbox->focused) {
        ui_set_cursor(ui_cursor_size_we);
    }

    frame->text_rects = text_rects;
    frame->cursor = cursor;
    return frame->data.textbox->submit;
}
void ta_ui_textbox_vec2(ta_vec2 *vec, ta_ui_textbox_vec2_state* vec_state,
    bool normalize, bool multiple_rows, bool reset_button)
{
    DLB_ASSERT(vec);
    DLB_ASSERT(vec_state);

    ta_ui_next_pad(0, 0, 0, 0);
    ta_ui_panel_begin(&vec_state->panel_state, TA_UI_AUTOSIZE);
    if (!multiple_rows) ta_ui_row_begin();

    const char *labels[2] = { "x:", "y:" };
    float *components = (float *)vec;
    for (int i = 0; i < 2; ++i) {
        if (multiple_rows) ta_ui_row_begin();

        ta_ui_label(CSTR(labels[i]));
        ta_ui_textbox_state *state = &vec_state->textbox_states[i];
        ta_ui_textbox_float(&components[i], state, 0);

        if ((reset_button && multiple_rows && i == 0) ||
            (reset_button && !multiple_rows && i == 1)) {
            ta_ui_next_margin(6, 1, 0, 1);
            ta_rgba c = TA_COLOR_DARK_RED;
            ta_ui_next_bg_color(UI_STATE_NONE, c.r, c.g, c.b, c.a);
            ta_ui_next_bg_color(UI_STATE_INTERACT, 0.8f, 0.0f, 0.0f, 0.9f);
            if (ta_ui_button(CSTR("Reset"))) {
                *vec = VEC2_ZERO;
            }
        }
    }
    if (normalize) {
        *vec = vec2_normalize(*vec);
    }

    ta_ui_panel_end();
}
void ta_ui_textbox_vec3(ta_vec3 *vec, ta_ui_textbox_vec3_state* vec_state,
    bool normalize, bool multiple_rows, bool reset_button)
{
    DLB_ASSERT(vec);
    DLB_ASSERT(vec_state);

    ta_ui_next_pad(0, 0, 0, 0);
    ta_ui_panel_begin(&vec_state->panel_state, TA_UI_AUTOSIZE);
    if (!multiple_rows) ta_ui_row_begin();

    const char *labels[3] = { "x:", "y:", "z:" };
    float *components = (float *)vec;
    for (int i = 0; i < 3; ++i) {
        if (multiple_rows) ta_ui_row_begin();

        ta_ui_label(CSTR(labels[i]));
        ta_ui_textbox_state *state = &vec_state->textbox_states[i];
        ta_ui_textbox_float(&components[i], state, 0);

        if ((reset_button && multiple_rows && i == 0) ||
            (reset_button && !multiple_rows && i == 2)) {
            ta_ui_next_margin(6, 1, 0, 1);
            ta_rgba c = TA_COLOR_DARK_RED;
            ta_ui_next_bg_color(UI_STATE_NONE, c.r, c.g, c.b, c.a);
            ta_ui_next_bg_color(UI_STATE_INTERACT, 0.8f, 0.0f, 0.0f, 0.9f);
            if (ta_ui_button(CSTR("Reset"))) {
                *vec = VEC3_ZERO;
            }
        }
    }
    if (normalize) {
        *vec = vec3_normalize(*vec);
    }

    ta_ui_panel_end();
}
void ta_ui_textbox_vec4(ta_vec4 *vec, ta_ui_textbox_vec4_state* vec_state,
    bool normalize, bool multiple_rows, bool reset_button)
{
    DLB_ASSERT(vec);
    DLB_ASSERT(vec_state);

    ta_ui_next_pad(0, 0, 0, 0);
    ta_ui_panel_begin(&vec_state->panel_state, TA_UI_AUTOSIZE);
    if (!multiple_rows) ta_ui_row_begin();

    const char *labels[4] = { "x:", "y:", "z:", "w:" };
    float *components = (float *)vec;
    for (int i = 0; i < 4; ++i) {
        if (multiple_rows) ta_ui_row_begin();

        ta_ui_label(CSTR(labels[i]));
        ta_ui_textbox_state *state = &vec_state->textbox_states[i];
        ta_ui_textbox_float(&components[i], state, 0);

        if (i == 0 && reset_button) {
            ta_ui_next_margin(6, 1, 0, 1);
            ta_ui_next_bg_color(UI_STATE_NONE, 0.5f, 0.0f, 0.0f, 0.9f);
            ta_ui_next_bg_color(UI_STATE_INTERACT, 0.9f, 0.0f, 0.0f, 0.9f);
            if (ta_ui_button(CSTR("Reset"))) {
                *vec = QUAT_IDENT;
            }
        }
    }
    if (normalize) {
        *vec = quat_normalize(*vec);
    }

    ta_ui_panel_end();
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
    dlb_vec_reserve(textbox->buffer, len + 2);  // reserve 1 extra for nil
    dlb_memmove(
        textbox->buffer + textbox->cursor + 1,
        textbox->buffer + textbox->cursor,
        len - textbox->cursor
    );
    textbox->buffer[textbox->cursor] = c;
    dlb_vec_hdr(textbox->buffer)->len++;
    textbox->cursor++;

    // Ensure buffer is nil-terminated
    u32 new_len = dlb_vec_len(textbox->buffer);
    DLB_ASSERT(textbox->buffer[new_len] == '\0');

    return true;
}
void ta_ui_textbox_submit(ta_ui_textbox_state *textbox)
{
    DLB_ASSERT(textbox);
    textbox_command_submit(textbox);
}
void ta_ui_textbox_clear(ta_ui_textbox_state *textbox)
{
    DLB_ASSERT(textbox);
    textbox_command_cancel(textbox);
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
void ta_ui_tooltip(const char *text, u32 text_len)
{
    ta_rect_uv *text_rects = 0;
    ta_rectf text_rect = ta_font_push_text(&text_rects, ui_font, text, text_len,
        true, 0, 0, 0);

    float offset_x = MOUSE_X + 10.0f;
    float offset_y = MOUSE_Y + 20.0f;

    ta_rect_uv tooltip_bg = { 0 };
    tooltip_bg.rect.x = offset_x - 4.0f;
    tooltip_bg.rect.y = offset_y - 2.0f;
    tooltip_bg.rect.w = text_rect.w + 8.0f;
    tooltip_bg.rect.h = text_rect.h + 3.0f;
    ta_primitive_push_rect_uv(&primitive_quads_tooltip_bg, tooltip_bg, TA_COLOR_GRAY3A,
        UI_LAYER_TIP_BG, true, false);

    dlb_vec_each(ta_rect_uv *, rect, text_rects) {
        ta_rect_uv offset_rect = *rect;
        offset_rect.rect.x += offset_x;
        offset_rect.rect.y += offset_y;
        ta_primitive_push_rect_uv(&primitive_quads_tooltip_fg, offset_rect,
            TA_COLOR_WHITE, UI_LAYER_TIP, true, false);
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
    ta_primitive_push_rect(0, bg_rect, frame->bg_color[frame->state_type],
        UI_LAYER_EDIT_WINDOW_BG);
    ta_primitive_render_mesh(&primitive_quads, tg_shader_quads, TA_TRIANGLES,
        true, false);
}
static void ui_render_panel(ui_frame *frame)
{
    // Panel background
    ta_primitive_push_rect(0, frame->rect, frame->bg_color[frame->state_type],
        UI_LAYER_EDIT_1_BG);
    ta_primitive_render_mesh(&primitive_quads, tg_shader_quads, TA_TRIANGLES,
        true, false);
}
static void ui_render_button(ui_frame *frame)
{
    ta_primitive_push_rect(0, frame->rect, frame->bg_color[frame->state_type],
        UI_LAYER_EDIT_1_BG);
    ta_primitive_render_mesh(&primitive_quads, tg_shader_quads, TA_TRIANGLES,
        true, false);
}
static void ui_render_toggle_button(ui_frame *frame)
{
    ta_rgba bg_color = frame->bg_color[frame->state_type];
    ta_primitive_push_rect(0, frame->rect, bg_color, UI_LAYER_EDIT_1_BG);
    ta_primitive_render_mesh(&primitive_quads, tg_shader_quads, TA_TRIANGLES,
        true, false);
}
static void ui_render_image(ui_frame *frame)
{
    if (frame->texture) {
        if (frame->texture->cubemap) {
            DLB_ASSERT(frame->texture_face >= 0 && frame->texture_face <= 6);
            ta_shader_set_sampler_cube(tg_shader_cubemap, SYM_U_TEX, frame->texture->gl_id);
            ta_shader_set_int(tg_shader_cubemap, SYM_U_FACE, frame->texture_face);
            ta_rect img_rect = rect_shrink(frame->rect, frame->pad);
            ta_primitive_push_rect(0, img_rect, TA_COLOR_INVIS, UI_LAYER_EDIT_1);
            ta_primitive_render_mesh(&primitive_quads, tg_shader_quads,
                TA_TRIANGLES, true, false);
            ta_shader_set_sampler_cube(tg_shader_cubemap, SYM_U_TEX, 0);
        } else {
            ta_shader_set_mat4(tg_shader_quads, SYM_U_PROJ, &MAT4_IDENT);
            ta_shader_set_mat4(tg_shader_quads, SYM_U_VIEW, &MAT4_IDENT);
            ta_shader_set_mat4(tg_shader_quads, SYM_U_MODEL, &MAT4_IDENT);
            ta_shader_set_sampler2d(tg_shader_quads, SYM_U_TEX, frame->texture->gl_id);
            ta_rect img_rect = rect_shrink(frame->rect, frame->pad);
            ta_primitive_push_rect(0, img_rect, TA_COLOR_INVIS, UI_LAYER_EDIT_1);
            ta_primitive_render_mesh(&primitive_quads, tg_shader_quads,
                TA_TRIANGLES, true, false);
            ta_shader_set_sampler2d(tg_shader_quads, SYM_U_TEX, 0);
        }
    }
}
static void ui_render_text(float x, float y, ta_rect_uv *text_rects)
{
    if (dlb_vec_len(text_rects)) {
        dlb_vec_each(ta_rect_uv *, rect, text_rects) {
            ta_primitive_push_rect_uv(&primitive_quads, *rect, TA_COLOR_WHITE, 0,
                true, false);
        }

        ta_font_render(&primitive_quads, ui_font, x, y, UI_LAYER_EDIT_1, true, false);
    }
}
static void ui_render_label(ui_frame *frame)
{
    // Render background
    if (frame->bg_color[frame->state_type].a == 0.0f) {
        frame->bg_color[frame->state_type].a = TA_EPSILON;
    }
    ta_primitive_push_rect(0, frame->rect, frame->bg_color[frame->state_type],
        UI_LAYER_EDIT_1_BG);
    ta_primitive_render_mesh(&primitive_quads, tg_shader_quads, TA_TRIANGLES,
        true, false);

    // Render text
    float x = (float)frame->rect.x + frame->pad.x;
    float y = (float)frame->rect.y + frame->pad.y;
    ui_render_text(x, y, frame->text_rects);
}
static void ui_render_textbox(ui_frame *frame)
{
    // Render background
    ta_rgba bg_color = frame->bg_color[frame->state_type];
    ta_primitive_push_rect(0, frame->rect, bg_color, UI_LAYER_EDIT_1_BG);
    ta_primitive_render_mesh(&primitive_quads, tg_shader_quads, TA_TRIANGLES,
        true, false);

    // Render text
    int x = frame->rect.x + frame->pad.x;
    int y = frame->rect.y + frame->pad.y;
    ui_render_text((float)x, (float)y, frame->text_rects);

    // If active, render cursor
    if (frame->data.textbox->focused && !frame->data.textbox->focus_changed) {
        ta_rect cursor_rect = { 0 };
        cursor_rect.x = x + (int)frame->cursor.x;
        cursor_rect.y = y + (int)frame->cursor.y + 1;
        cursor_rect.w = 1;
        cursor_rect.h = ui_font->line_height - 2;
        ta_primitive_push_rect(0, cursor_rect, TA_COLOR_GRAY8, UI_LAYER_EDIT_2);
        ta_primitive_render_mesh(&primitive_quads, tg_shader_quads, TA_TRIANGLES,
            true, false);
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
        scroll_v_rect.x = frame->rect.x + frame->content_size.w;
        scroll_v_rect.y = frame->rect.y;
        scroll_v_rect.w = SCROLL_WIDGET_THICKNESS;
        scroll_v_rect.h = frame->rect.h;

        float widget_h_pct = (float)frame->rect.h / frame->content_size.h;
        int widget_h = (int)(frame->rect.h * widget_h_pct);
        int scroll_space_v = frame->rect.h - widget_h;
        int scroll_v = (int)(scroll->percent.y * scroll_space_v);

        ta_rect scroll_v_widget = { 0 };
        scroll_v_widget.x = frame->rect.x + frame->content_size.w;
        scroll_v_widget.w = SCROLL_WIDGET_THICKNESS;
        scroll_v_widget.y = frame->rect.y + scroll_v;
        scroll_v_widget.h = widget_h;

        bool widget_hover = rect_contains_mouse(scroll_v_widget);
        bool down = ta_key_down(SDL_SCANCODE_MOUSE_LEFT);
        ta_rgba widget_color = (ta_rgba){ 0.6f, 0.0f, 0.0f, 1.0f };
        if (widget_hover && !dragging_v) {
            widget_color = (ta_rgba){ 0.8f, 0.0f, 0.0f, 1.0f };
        }

        ta_primitive_push_rect(0, scroll_v_rect, TA_COLOR_GRAY4, UI_LAYER_EDIT_1);
        ta_primitive_push_rect(0, scroll_v_widget, widget_color, UI_LAYER_EDIT_1);
        ta_primitive_render_mesh(&primitive_quads, tg_shader_quads, TA_TRIANGLES,
            true, false);

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

            if (delta_y) {
                scroll->percent.y += (float)delta_y / scroll_space_v;
                scroll->percent.y = clampf(scroll->percent.y, 0.0f, 1.0f);
                scroll->pixels.y = (int)(scroll->percent.y * overflow_y);
            }
        }
    }
}
static void ui_render_tooltips()
{
    ta_primitive_render_mesh(&primitive_quads_tooltip_bg, tg_shader_quads, TA_TRIANGLES,
        true, false);
    ta_font_render(&primitive_quads_tooltip_fg, ui_font, 0, 0, UI_LAYER_TIP,
        true, false);
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

        frame->clip_rect = TA_RECT_ZERO;
        frame->clip_rect.w = WINDOW_W;
        frame->clip_rect.h = WINDOW_H;
        u32 container_idx = frame->container_idx;
        while(container_idx) {
            DLB_ASSERT(container_idx < dlb_vec_len(ui_frames));
            ui_frame *container = &ui_frames[container_idx];
            frame->clip_rect = rect_intersect(frame->clip_rect, container->rect);
            container_idx = container->container_idx;
        }
        int inv_y = WINDOW_H - (frame->clip_rect.y + frame->clip_rect.h);
        glScissor(frame->clip_rect.x, inv_y, frame->clip_rect.w, frame->clip_rect.h);

        ui_renderers[frame->type](frame);
        ui_render_scrollbars(frame);

        // Free any per-frame memory
        dlb_vec_free(frame->text_rects);
    }

    glDisable(GL_SCISSOR_TEST);

    ui_render_tooltips();

    last_frame_state = 0;
    dlb_vec_zero(ui_frames);
    dlb_vec_alloc(ui_frames);  // reserve UI_ROOT

    if (ui_active_cursor_changed) {
        SDL_SetCursor(ui_active_cursor);
        ui_active_cursor_changed = false;
    }
}