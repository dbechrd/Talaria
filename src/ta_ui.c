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
#include "ta_log.h"
#include "ta_scene.h"
#include "dlb/dlb_vector.h"
#include "dlb/dlb_murmur3.h"
#include "misc/gl3w.h"

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
#define TA_UI_INVISIBLE         0x00000001  // takes up space but doesn't render (display: hidden)
//#define TA_UI_AUTOSIZE_W      0x00000002  // auto-grow container to fit contents
//#define TA_UI_AUTOSIZE_H      0x00000004  // auto-grow container to fit contents
// internal container flags
#define TA_UI_CONTAINER         0x40000000  // will always be set on containers
#define TA_UI_CONTAINER_ENDED   0x80000000  // will be set when container ends

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
} next_frame_flags;

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

    ta_rect_uv *text_rects; // vector, must be freed!
    ta_texture *texture;
    int texture_face;       // for cubemaps
    ta_vec2 *scroll_offset; // vertical scroll as percentage from top (0.0 - 1.0)
    ta_vec2 cursor;         // cursor location for textboxes

    // Consolidate this and all bools into a single flags bitmap
    ta_ui_state state;

    // TA_UI_INVISIBLE
    u32 flags;

    // TA_UI_CONTAINER
    // TA_UI_AUTOSIZE
    u32 container_flags;
} ui_frame;

static ui_frame *ui_frames;

static void ui_row_end(ui_frame *container);

void ta_ui_init()
{
    // Reserve element zero for UI_ROOT
    dlb_vec_alloc(ui_frames);

    //ui_default_style[UI_ROOT].margin                        = TA_RECT_ZERO;
    //ui_default_style[UI_ROOT].pad                           = TA_RECT_ZERO;
    //ui_default_style[UI_ROOT].bg_color[UI_STATE_NONE]       = TA_COLOR_INVIS;
    //--ui_default_style[UI_ROOT].bg_color[UI_STATE_HOVER]      = TA_RGBA(1.0f, 0.0f, 0.0f, 0.9f);
    //ui_default_style[UI_ROOT].bg_color[UI_STATE_DOWN]       = TA_COLOR_INVIS;
    //ui_default_style[UI_ROOT].bg_color[UI_STATE_ACTIVE]     = TA_COLOR_INVIS;
    //ui_default_style[UI_ROOT].fg_color[UI_STATE_NONE]       = TA_COLOR_INVIS;
    //ui_default_style[UI_ROOT].fg_color[UI_STATE_HOVER]      = TA_COLOR_INVIS;
    //ui_default_style[UI_ROOT].fg_color[UI_STATE_DOWN]       = TA_COLOR_INVIS;
    //ui_default_style[UI_ROOT].fg_color[UI_STATE_ACTIVE]     = TA_COLOR_INVIS;

    //ui_default_style[UI_WINDOW].margin                      = TA_RECT_ZERO;
    ui_default_style[UI_WINDOW].pad                         = TA_RECT(4, 4, 4, 4);
    //ui_default_style[UI_WINDOW].bg_color[UI_STATE_NONE]     = TA_COLOR_INVIS;
    //--ui_default_style[UI_WINDOW].bg_color[UI_STATE_HOVER]    = TA_RGBA(0.0f, 1.0f, 0.0f, 0.9f);
    //ui_default_style[UI_WINDOW].bg_color[UI_STATE_DOWN]     = TA_COLOR_INVIS;
    //ui_default_style[UI_WINDOW].bg_color[UI_STATE_ACTIVE]   = TA_COLOR_INVIS;
    //ui_default_style[UI_WINDOW].fg_color[UI_STATE_NONE]     = TA_COLOR_INVIS;
    //ui_default_style[UI_WINDOW].fg_color[UI_STATE_HOVER]    = TA_COLOR_INVIS;
    //ui_default_style[UI_WINDOW].fg_color[UI_STATE_DOWN]     = TA_COLOR_INVIS;
    //ui_default_style[UI_WINDOW].fg_color[UI_STATE_ACTIVE]   = TA_COLOR_INVIS;

    //--ui_default_style[UI_PANEL].margin                     = TA_RECT(0, 0, 0, 0);
    ui_default_style[UI_PANEL].pad                          = TA_RECT(4, 4, 4, 4);
    //ui_default_style[UI_PANEL].bg_color[UI_STATE_NONE]      = TA_COLOR_INVIS;
    //--ui_default_style[UI_PANEL].bg_color[UI_STATE_HOVER]     = TA_RGBA(0.0f, 0.0f, 1.0f, 0.9f);
    //ui_default_style[UI_PANEL].bg_color[UI_STATE_DOWN]      = TA_COLOR_INVIS;
    //ui_default_style[UI_PANEL].bg_color[UI_STATE_ACTIVE]    = TA_COLOR_INVIS;
    //ui_default_style[UI_PANEL].fg_color[UI_STATE_NONE]      = TA_COLOR_INVIS;
    //ui_default_style[UI_PANEL].fg_color[UI_STATE_HOVER]     = TA_COLOR_INVIS;
    //ui_default_style[UI_PANEL].fg_color[UI_STATE_DOWN]      = TA_COLOR_INVIS;
    //ui_default_style[UI_PANEL].fg_color[UI_STATE_ACTIVE]    = TA_COLOR_INVIS;

    //--ui_default_style[UI_BUTTON].margin                      = TA_RECT(2, 1, 0, 1);
    //--ui_default_style[UI_BUTTON].pad                         = TA_RECT(4, 4, 4, 4); //TA_RECT(4, 1, 4, 1);
    //--ui_default_style[UI_BUTTON].bg_color[UI_STATE_NONE]     = TA_RGBA(1.0f, 1.0f, 1.0f, 0.9f);
    //--ui_default_style[UI_BUTTON].bg_color[UI_STATE_HOVER]    = TA_RGBA(1.0f, 1.0f, 0.0f, 0.9f);
    //--ui_default_style[UI_BUTTON].bg_color[UI_STATE_DOWN]     = TA_RGBA(1.0f, 1.0f, 0.0f, 0.9f);
    //--ui_default_style[UI_BUTTON].bg_color[UI_STATE_ACTIVE]   = TA_RGBA(0.0f, 1.0f, 1.0f, 0.9f);
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

    //--ui_default_style[UI_LABEL].margin                       = TA_RECT(2, 1, 0, 1);
    //--ui_default_style[UI_LABEL].pad                          = TA_RECT(4, 1, 4, 1);
    //--ui_default_style[UI_LABEL].bg_color[UI_STATE_NONE]      = TA_RGBA(0.2f, 0.2f, 0.4f, 0.9f);
    //--ui_default_style[UI_LABEL].bg_color[UI_STATE_HOVER]     = TA_RGBA(0.2f, 0.2f, 0.4f, 0.9f);
    //--ui_default_style[UI_LABEL].bg_color[UI_STATE_DOWN]      = TA_RGBA(0.2f, 0.2f, 0.4f, 0.9f);
    //--ui_default_style[UI_LABEL].bg_color[UI_STATE_ACTIVE]    = TA_RGBA(0.2f, 0.2f, 0.4f, 0.9f);
    //ui_default_style[UI_LABEL].fg_color[UI_STATE_NONE]      = TA_COLOR_INVIS;
    //ui_default_style[UI_LABEL].fg_color[UI_STATE_HOVER]     = TA_COLOR_INVIS;
    //ui_default_style[UI_LABEL].fg_color[UI_STATE_DOWN]      = TA_COLOR_INVIS;
    //ui_default_style[UI_LABEL].fg_color[UI_STATE_ACTIVE]    = TA_COLOR_INVIS;

    //--ui_default_style[UI_TEXTBOX].margin                     = TA_RECT(2, 1, 0, 1);
    //--ui_default_style[UI_TEXTBOX].pad                        = TA_RECT(4, 1, 4, 1);
    //--ui_default_style[UI_TEXTBOX].bg_color[UI_STATE_NONE]    = TA_RGBA(1.0f, 1.0f, 1.0f, 0.9f);
    //--ui_default_style[UI_TEXTBOX].bg_color[UI_STATE_HOVER]   = TA_RGBA(1.0f, 1.0f, 0.0f, 0.9f);
    //--ui_default_style[UI_TEXTBOX].bg_color[UI_STATE_DOWN]    = TA_RGBA(1.0f, 0.0f, 1.0f, 0.9f);
    //--ui_default_style[UI_TEXTBOX].bg_color[UI_STATE_ACTIVE]  = TA_RGBA(0.0f, 1.0f, 1.0f, 0.9f);
    //ui_default_style[UI_TEXTBOX].fg_color[UI_STATE_NONE]    = TA_COLOR_INVIS;
    //ui_default_style[UI_TEXTBOX].fg_color[UI_STATE_HOVER]   = TA_COLOR_INVIS;
    //ui_default_style[UI_TEXTBOX].fg_color[UI_STATE_DOWN]    = TA_COLOR_INVIS;
    //ui_default_style[UI_TEXTBOX].fg_color[UI_STATE_ACTIVE]  = TA_COLOR_INVIS;
}

#if 1
static ta_rgba ui_random_color(u32 frame_idx, ui_state_type state)
{
    // HACK: Generate random colors based on name of control
    ta_rgba color;
    u32 hash = dlb_murmur3(&frame_idx, sizeof(frame_idx));
    color.r = ((hash >> (state     )) & 255) / 255.0f;
    color.g = ((hash >> (state +  8)) & 255) / 255.0f;
    color.b = ((hash >> (state + 16)) & 255) / 255.0f;
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
        if (ui_frames[i].container_flags &&
            !(ui_frames[i].container_flags & TA_UI_CONTAINER_ENDED))
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
static ui_frame *ui_frame_begin(ui_frame_type type, const char *name,
    u32 container_flags)
{
    // Allocate frame
    ui_frame *frame = dlb_vec_alloc(ui_frames);
    frame->index = dlb_vec_len(ui_frames) - 1;
    frame->type = type;
    frame->name = name;
    frame->container_flags = container_flags;

    frame->margin = next_frame_flags.margin
        ? next_frame_style.margin
        : ui_default_style[type].margin;
    frame->pad = next_frame_flags.pad
        ? next_frame_style.pad
        : ui_default_style[type].pad;

    frame->container = ui_container(frame->index);

    ta_vec2i offset = frame->container->offset;
    if (next_frame_flags.pos_relative) {
        offset = next_frame_pos_relative;
        frame->skip_flow = next_frame_flags.pos_relative;
    }

    frame->rect.x = frame->container->rect.x + offset.x + frame->margin.x;
    frame->rect.y = frame->container->rect.y + offset.y + frame->margin.y;
    frame->rect.w = frame->pad.x + frame->pad.w;
    frame->rect.h = frame->pad.y + frame->pad.h;
    frame->content_size.w = frame->rect.w;
    frame->content_size.h = frame->rect.h;

    if (next_frame_flags.size) {
        frame->rect.w = frame->pad.x + next_frame_size.w + frame->pad.w;
        frame->rect.h = frame->pad.y + next_frame_size.h + frame->pad.h;
    }

    // Pad container offsets
    if (frame->container_flags) {
        frame->offset.x += frame->pad.x;
        frame->offset.y += frame->pad.y;
    }

    if (next_frame_flags.invisible) {
        frame->flags |= TA_UI_INVISIBLE;
    }

    next_frame_pos_relative = TA_VEC2I_ZERO;
    next_frame_size = TA_SIZE_ZERO;
    dlb_memset(&next_frame_style, 0, sizeof(next_frame_style));
    dlb_memset(&next_frame_flags, 0, sizeof(next_frame_flags));

    return frame;
}
static ui_frame *ui_frame_end(ui_frame_type type)
{
    // NOTE: This currently prevents nesting two controls of the same type
    // inside one another, but I think that's okay for now. The only way I can
    // think to fix this would be to have a "container_finalized" flag on each
    // container that's set to true when _end is called, then skip over those
    // when searching backwards. Could still cause weird bugs where we end the
    // wrong container though, so going back to u32 index instead of searching
    // by type is probaby less error-prone.
    ui_frame *frame = 0;
    for (frame = dlb_vec_last(ui_frames); frame != ui_frames; --frame) {
        if (frame->type == type &&
            !(frame->container_flags & TA_UI_CONTAINER_ENDED))
        {
            frame = frame;
            break;
        }
    }
    DLB_ASSERT(frame);  // If someone calls _end before _begin this will assert

    // Finalize containers
    if (frame->container_flags) {
        ui_row_end(frame);
        if (frame->container_flags & TA_UI_AUTOSIZE_W) {
            frame->rect.w = MAX(frame->rect.w, frame->content_size.w);
        }
        if (frame->container_flags & TA_UI_AUTOSIZE_H) {
            frame->rect.h = MAX(frame->rect.h, frame->content_size.h);
        }
        frame->container_flags |= TA_UI_CONTAINER_ENDED;

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

    // Updated frame states
    ui_state_type state = UI_STATE_NONE;
    frame->state.hover = false;
    frame->state.down = false;
    frame->state.pressed = false;
    frame->state.released = false;
    if (!ta_mouse_captured() && !ta_mouse_dragging() && rect_contains_mouse(frame->rect))
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
    frame->bg_color = next_frame_flags.bg_color[state]
        ? next_frame_style.bg_color[state]
        : ui_default_style[type].bg_color[state];
    frame->fg_color = next_frame_flags.fg_color[state]
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
    next_frame_flags.margin = true;
#endif
}
void ta_ui_next_pad(int left, int top, int right, int bottom)
{
#if UI_DEBUG_PAD
    next_frame_style.pad.x = left;
    next_frame_style.pad.y = top;
    next_frame_style.pad.w = right;
    next_frame_style.pad.h = bottom;
    next_frame_flags.pad = true;
#endif
}
void ta_ui_next_offset(int x, int y)
{
    next_frame_pos_relative.x = x;
    next_frame_pos_relative.y = y;
    next_frame_flags.pos_relative = true;
}
void ta_ui_next_size(int w, int h)
{
    next_frame_size.w = w;
    next_frame_size.h = h;
    next_frame_flags.size = true;
}
void ta_ui_next_invisible()
{
    next_frame_style.invisible = true;
    next_frame_flags.invisible = true;
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
            next_frame_flags.bg_color[i] = true;
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
            next_frame_flags.fg_color[i] = true;
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
void ta_ui_window_begin(const char *name, ta_ui_window_state *state,
    u32 container_flags)
{
    DLB_ASSERT(state);
    ui_frame *frame = ui_frame_begin(UI_WINDOW, name, container_flags | TA_UI_CONTAINER);
    frame->scroll_offset = &state->scroll_offset;
}
void ta_ui_window_end()
{
    ui_frame_end(UI_WINDOW);
}
void ta_ui_panel_begin(const char *name, ta_ui_panel_state *state,
    u32 container_flags)
{
    DLB_ASSERT(state);
    ui_frame *frame = ui_frame_begin(UI_PANEL, name, container_flags | TA_UI_CONTAINER);
    frame->scroll_offset = &state->scroll_offset;
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
void ta_ui_button_begin(const char *name, u32 container_flags)
{
    ui_frame_begin(UI_BUTTON, name, container_flags | TA_UI_CONTAINER);
}
bool ta_ui_button_end()
{
    ui_frame *frame = ui_frame_end(UI_BUTTON);
    return frame->state.pressed;
}
bool ta_ui_button(const char *name)
{
    ui_frame_begin(UI_BUTTON, name, 0);
    return ta_ui_button_end();
}
void ta_ui_button_toggle_begin(const char *name, u32 container_flags)
{
    ui_frame_begin(UI_BUTTON, name, container_flags | TA_UI_CONTAINER);
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
    ui_frame_begin(UI_BUTTON, name, 0);
    return ta_ui_button_toggle_end(checked);
}
bool ta_ui_image(const char *name, const char *tex_name, int face)
{
#if UI_DEBUG_NO_TEXTURES
    tex = 0;
#endif

    ta_texture *texture = 0;
    if (tex_name) {
        texture = ta_scene_find_by_name(tg_game.scene, RES_TEXTURE, tex_name);
    }

    if (texture) {
        if (!next_frame_size.w && !next_frame_size.h) {
            ta_ui_next_size(texture->width, texture->height);
        }
    }

    ui_frame *frame = ui_frame_begin(UI_IMAGE, name, 0);
    frame->texture = texture;
    frame->texture_face = face;
    ui_frame_end(UI_IMAGE);

    return frame->state.pressed;
}
bool ta_ui_texture(const char *name, ta_texture *texture, int face)
{
#if UI_DEBUG_NO_TEXTURES
    tex = 0;
#endif

    if (texture) {
        if (!next_frame_size.w && !next_frame_size.h) {
            ta_ui_next_size(texture->width, texture->height);
        }
    }

    ui_frame *frame = ui_frame_begin(UI_IMAGE, name, 0);
    frame->texture = texture;
    frame->texture_face = face;
    ui_frame_end(UI_IMAGE);

    return frame->state.pressed;
}
bool ta_ui_label(const char *name, const char *text)
{
    DLB_ASSERT(text);

    ta_font *font = ta_scene_find_by_name(tg_game.scene, RES_FONT, tg_game.font);
    ta_rect_uv *text_rects = 0;
    ta_rectf text_rect = ta_font_push_text(&text_rects, font, text, 0, true, 0,
        0, 0, 0);

    // Auto-expand frame based on contents
    ta_ui_next_size(MAX(next_frame_size.w, (int)text_rect.w),
                    MAX(next_frame_size.h, (int)text_rect.h));

    ui_frame *frame = ui_frame_begin(UI_LABEL, name, 0);
    frame->text_rects = text_rects;
    ui_frame_end(UI_LABEL);

    return frame->state.pressed;
}
bool ta_ui_textbox(const char *name, ta_text_entry *text_entry)
{
    DLB_ASSERT(text_entry);

    ta_rect_uv *text_rects = 0;
    ta_vec2 cursor = { 0 };
    ta_rectf bounds = ta_text_entry_draw(text_entry, &text_rects, &cursor);

    // Auto-expand frame based on contents
    ta_ui_next_size(MAX(next_frame_size.w, (int)bounds.w),
                    MAX(next_frame_size.h, (int)bounds.h));

    ui_frame *frame = ui_frame_begin(UI_TEXTBOX, name, 0);

    if (frame->state.down) {
        // Activate textbox when clicked
        ta_text_entry_focus(text_entry);
    } else if (ta_key_pressed(SDL_SCANCODE_MOUSE_LEFT)) {
        // Deactivate textbox when elsewhere clicked
        ta_text_entry_unfocus(text_entry);
    }

    frame->state.focused = ta_text_entry_focused(text_entry);
    frame->text_rects = text_rects;
    frame->cursor = cursor;

    ui_frame_end(UI_TEXTBOX);
    return ta_text_entry_valid(text_entry);
}
void ta_ui_tooltip(const char *text, u32 text_len)
{
    ta_rect_uv *text_rects = 0;
    ta_font *font = ta_scene_find_by_name(tg_game.scene, RES_FONT, tg_game.font);
    ta_rectf text_rect = ta_font_push_text(&text_rects, font, text, text_len,
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
    ta_font *font = ta_scene_find_by_name(tg_game.scene, RES_FONT, tg_game.font);
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
        ta_font *font = ta_scene_find_by_name(tg_game.scene, RES_FONT,
            tg_game.font);
        dlb_vec_each(ta_rect_uv *, rect, text_rects) {
            ta_primitive_push_rect_uv(&quads_queue, *rect, TA_COLOR_WHITE, 0,
                true, false);
        }

        ta_font_render(quads_queue, font, x, y, UI_LAYER_EDIT_1, true, true);
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
        ta_font *font = ta_scene_find_by_name(tg_game.scene, RES_FONT,
            tg_game.font);
        ta_rect cursor_rect = { 0 };
        cursor_rect.x = x + (int)frame->cursor.x;
        cursor_rect.y = y + (int)frame->cursor.y + 1;
        cursor_rect.w = 1;
        cursor_rect.h = font->line_height - 2;
        ta_primitive_push_rect(cursor_rect, TA_COLOR_GRAY8, UI_LAYER_EDIT_2);
        ta_primitive_render_quads(quads_queue, tg_shader_quads, true, true);
    }
}

static bool scrollbar_dragging_y = false;
static int scrollbar_y_frame_idx = 0;
static ta_vec2i ui_render_scrollbars(ui_frame *frame)
{
    ta_vec2i scroll_offset = { 0 };

    if (frame->content_size.h <= frame->rect.h)
        return scroll_offset;

    // TODO: Can I remove 1 frame latency by checking scrollbar pressed logic
    // before rendering scrollbars? Just add acceleration to compensate.

    // TODO: Horizontal scrollbar
    // Render scrollbar widgets
    ta_rect widget_rect = { 0 };
    widget_rect.x = frame->rect.x + frame->content_size.w;
    widget_rect.w = SCROLL_WIDGET_THICKNESS;

    // rect: 100
    // content: 150
    // widget_pct = 0.66

    float widget_h_pct = (float)frame->rect.h / frame->content_size.h;
    //widget_rect.h = (int)MAX(SCROLL_WIDGET_H_MIN, frame->rect.h * widget_h_pct);
    widget_rect.h = (int)(frame->rect.h * widget_h_pct);
    int scroll_space_y = frame->rect.h - widget_rect.h;
    int scroll_y = (int)(frame->scroll_offset->y * scroll_space_y);
    widget_rect.y = frame->rect.y + scroll_y;

    scroll_offset.y = (int)(frame->scroll_offset->y * (frame->content_size.h - frame->rect.h));

    //ta_primitive_push_rect(widget_rect, (ta_rgba){ 0.6f, 0.0f, 0.0f, 0.4f },
    //    UI_LAYER_EDIT_1);
    ta_primitive_push_rect(widget_rect, (ta_rgba){ 0.6f, 0.0f, 0.0f, 1.0f },
        UI_LAYER_EDIT_1);
    ta_primitive_render_quads(quads_queue, tg_shader_quads, true, true);

    if (ta_mouse_captured())
        return scroll_offset;

#if 0
    int delta_y = 0;
    if (rect_contains_mouse(frame->rect)) {
        delta_y = ta_mouse_scroll_dy() * SCROLL_WHEEL_SPEED;
    } else if (!ta_mouse_captured() && rect_contains_mouse(widget_rect) &&
        ta_key_down(SDL_SCANCODE_MOUSE_LEFT))
    {
        delta_y = ta_mouse_dy();
    }
#else
    int delta_y = 0;
    if (ta_key_pressed(SDL_SCANCODE_MOUSE_LEFT) && rect_contains_mouse(widget_rect)) {
        scrollbar_y_frame_idx = frame->index;
        scrollbar_dragging_y = true;
        ta_mouse_drag_begin();
    } else if (!scrollbar_dragging_y && rect_contains_mouse(frame->rect)) {
        delta_y = ta_mouse_scroll_dy() * SCROLL_WHEEL_SPEED;
    } else if (!ta_key_down(SDL_SCANCODE_MOUSE_LEFT)) {
        if (scrollbar_dragging_y) {
            scrollbar_dragging_y = false;
            ta_mouse_drag_end();
        }
    }

    if (scrollbar_dragging_y) {
        delta_y = ta_mouse_dy();
    }
#endif

    if (delta_y) {
        frame->scroll_offset->y += (float)delta_y / scroll_space_y;
        frame->scroll_offset->y = clampf(frame->scroll_offset->y, 0.0f, 1.0f);
    }

    return scroll_offset;
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
        if (frame->flags & TA_UI_INVISIBLE || !ui_renderers[frame->type])
            continue;

#if 0
        if (frame->type == UI_PANEL && frame->index == 9) {
            // TODO: Scissor stack (sub-scissors can only shrink the box)
            ta_rect clip_rect = frame->rect;
            // TODO: Should clip rect contain pad or not? Not sure.. see how it looks.
            //ta_rect clip_rect = rect_shrink(frame->rect, frame->pad);
            int inv_y = WINDOW_H - (clip_rect.y + clip_rect.h);
            glScissor(clip_rect.x, inv_y, clip_rect.w, clip_rect.h);
        }
#endif

        // NOTE: Can't do this, because will be desync'd with all layout pass
        // (e.g. tooltip locations and mouse hover, etc.). Need to accumulate
        // scroll_offset in all containers during layout pass, then apply to
        // children controls.. somehow.
        //frame->rect.x -= scroll_offset.x;
        //frame->rect.y -= scroll_offset.y;
        ui_renderers[frame->type](frame);

        ta_vec2i scroll = ui_render_scrollbars(frame);
        scroll_offset.x += scroll.x;
        scroll_offset.y += scroll.y;

        // Free any per-frame memory
        dlb_vec_free(frame->text_rects);
    }

    glDisable(GL_SCISSOR_TEST);

    // Render tooltips
    ta_primitive_render_quads(tooltip_bg_queue, tg_shader_quads, true, true);
    ta_font *font = ta_scene_find_by_name(tg_game.scene, RES_FONT, tg_game.font);
    ta_font_render(tooltip_fg_queue, font, 0, 0, UI_LAYER_TIP, true, true);

    last_frame_state = 0;
    dlb_vec_zero(ui_frames);
    dlb_vec_alloc(ui_frames);  // reserve UI_ROOT
}