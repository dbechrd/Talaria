#pragma once
#include "dlb/dlb_types.h"
#include "ta_math.h"

struct ta_font;
struct ta_texture;

typedef enum ui_state_type {
    UI_STATE_NONE,
    UI_STATE_HOVER,
    UI_STATE_DOWN,
    UI_STATE_ACTIVE,
    UI_STATE_COUNT,

    // Special flags
    UI_STATE_INTERACT,  // any state except NONE
    UI_STATE_ALL        // all states
} ui_state_type;

typedef enum ui_frame_type {
    UI_ROOT,
    UI_WINDOW,
    UI_PANEL,
    UI_BUTTON,
    UI_TOGGLE_BUTTON,
    UI_IMAGE,
    UI_LABEL,
    UI_TEXTBOX,
    UI_COUNT
} ui_frame_type;

// TODO: Make this a flags enum
typedef struct ta_ui_state {
    bool hover;
    bool down;
    bool pressed;
    bool released;
    bool active;
} ta_ui_state;

typedef struct ta_ui_scroll_state {
    ta_vec2 percent;  // scroll as percentage of scrollable area (0.0 - 1.0)
    ta_vec2i pixels;  // amount of overflow in pixels last frame
} ta_ui_scroll_state;

typedef struct ta_ui_window_state {
    // TODO: Allow window move. (If you can nest windows inside other controls,
    // this needs to be a relative offset, rather than absolute location.
    ta_vec2i location;

    // TODO: Allow window resize. Once this has been set, it should override
    // AUTOSIZE flag.
    ta_vec2i size;

    ta_ui_scroll_state scroll;
} ta_ui_window_state;

typedef bool ta_textbox_filter(char c);
ta_textbox_filter *ta_textbox_filter_default;

typedef struct ta_ui_panel_state {
    ta_ui_scroll_state scroll;
} ta_ui_panel_state;

typedef struct ta_ui_textbox_state {
    char *buffer;  // vector
    u32 cursor;    // index of next character, 0 = before first char, len = after last char
    u32 selection_start;
    u32 selection_len;
    //bool multiline;
    bool clicked;
    bool double_clicked;
    ta_vec2i clicked_coords;
    double last_clicked_ms;
    bool focus_changed;  // HACK: Don't render cursor first frame, wrong index
    bool focused;   // has focus
    bool submit;    // user requested save
    ta_textbox_filter *filter;
    ta_ui_scroll_state scroll;
} ta_ui_textbox_state;

// container flags
#define TA_UI_AUTOSIZE_W      0x00000001  // auto-grow container to fit contents
#define TA_UI_AUTOSIZE_H      0x00000002  // auto-grow container to fit contents
#define TA_UI_AUTOSIZE        (TA_UI_AUTOSIZE_W | TA_UI_AUTOSIZE_H)

void ta_ui_init(struct ta_font *font, ta_ui_textbox_state **active_textbox);
void ta_ui_set_font(struct ta_font *font);

void ta_ui_next_margin(int left, int top, int right, int bottom);
void ta_ui_next_pad(int left, int top, int right, int bottom);
void ta_ui_next_offset(int x, int y);
void ta_ui_next_size(int w, int h);
void ta_ui_next_invisible();
void ta_ui_next_bg_color(ui_state_type state, float r, float g, float b, float a);
void ta_ui_next_fg_color(ui_state_type state, float r, float g, float b, float a);

ta_ui_state ta_ui_last_frame_state();
void ta_ui_row_begin();
void ta_ui_row_end();
void ta_ui_spacer(int w, int h);
void ta_ui_window_begin(const char *name, ta_ui_window_state *window, u32 flags);
void ta_ui_window_end();
void ta_ui_panel_begin(const char *name, ta_ui_panel_state *panel, u32 flags);
void ta_ui_panel_end();
void ta_ui_button_begin(const char *name, u32 flags);
bool ta_ui_button_end();
bool ta_ui_button(const char *name, const char *text, u32 text_len);
void ta_ui_toggle_button_begin(const char *name, u32 flags);
bool ta_ui_toggle_button_end(bool *checked);
bool ta_ui_toggle_button(const char *name, bool *checked);
bool ta_ui_image(const char *name, struct ta_texture *texture, int face);
//bool ta_ui_label(const char *name, const char *text, u32 text_len);
void ta_ui_label(const char *name, const char *text, u32 text_len);
bool ta_ui_textbox(const char *name, const char *text, u32 text_len,
    ta_ui_textbox_state *textbox, u32 flags);
bool ta_ui_textbox_insert(ta_ui_textbox_state *textbox, char c);
void ta_ui_textbox_submit(ta_ui_textbox_state *textbox);
void ta_ui_textbox_clear(ta_ui_textbox_state *textbox);
void ta_ui_tooltip_begin(const char *name);
void ta_ui_tooltip_end(const char *name);
void ta_ui_tooltip(const char *text, u32 text_len);
//void ta_ui_statusbar();

void ta_ui_render();