#pragma once
#include "dlb/dlb_types.h"
#include "ta_math.h"

struct ta_font;

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
typedef struct ta_ui_panel_state {
    ta_ui_scroll_state scroll;
} ta_ui_panel_state;

typedef bool ta_textbox_filter(char c);
ta_textbox_filter *ta_textbox_filter_default;
typedef struct ta_ui_textbox_state {
    char *buffer;           // vector of text data
    ta_rect_uv *text_rects; // vector of calculated text rect/uvs
    size_t cursor;          // index of next character, 0 = before first char, len = after last char
    size_t selection_start;
    size_t selection_len;
    //bool multiline;
    bool mouse_down;
    bool double_clicked;
    ta_vec2i mouse_coords;
    double last_clicked_ms;
    bool focus_changed;     // HACK: Don't render cursor first frame, wrong index
    bool focused;           // has focus
    bool submit;            // user requested save
    ta_textbox_filter *filter;
    ta_ui_scroll_state scroll;
} ta_ui_textbox_state;
typedef struct ta_ui_textbox_vec2_state {
    ta_ui_panel_state panel_state;
    ta_ui_textbox_state textbox_states[2];
} ta_ui_textbox_vec2_state;
typedef struct ta_ui_textbox_vec3_state {
    ta_ui_panel_state panel_state;
    ta_ui_textbox_state textbox_states[3];
} ta_ui_textbox_vec3_state;
typedef struct ta_ui_textbox_vec4_state {
    ta_ui_panel_state panel_state;
    ta_ui_textbox_state textbox_states[4];
} ta_ui_textbox_vec4_state;

// container flags
#define TA_UI_AUTOSIZE_W            0x00000001  // auto-grow container to fit contents
#define TA_UI_AUTOSIZE_H            0x00000002  // auto-grow container to fit contents
#define TA_UI_AUTOSIZE              (TA_UI_AUTOSIZE_W | TA_UI_AUTOSIZE_H)

// Initialization
void ta_ui_init                     (struct ta_font *font, ta_ui_textbox_state **textbox_editing, ta_ui_textbox_state **textbox_dragging);
void ta_ui_set_font                 (struct ta_font *font);
void ta_ui_set_cursor               (ta_cursor_type cursor_type);
void ta_ui_flags_reset              ();  // call this at the beginning of every game frame
bool ta_ui_flag_hovered             ();  // returns true if anything was hovered since last flag reset

// Styles
void ta_ui_next_margin_left         (int margin);
void ta_ui_next_margin_top          (int margin);
void ta_ui_next_margin_right        (int margin);
void ta_ui_next_margin_bottom       (int margin);
void ta_ui_next_margin              (int left, int top, int right, int bottom);
void ta_ui_next_pad_left            (int pad);
void ta_ui_next_pad_top             (int pad);
void ta_ui_next_pad_right           (int pad);
void ta_ui_next_pad_bottom          (int pad);
void ta_ui_next_pad                 (int left, int top, int right, int bottom);
void ta_ui_next_offset              (int x, int y);
void ta_ui_next_size                (int w, int h);
void ta_ui_next_invisible           ();
void ta_ui_next_bg_color            (ui_state_type state, float r, float g, float b, float a);
void ta_ui_next_fg_color            (ui_state_type state, float r, float g, float b, float a);
ta_ui_state ta_ui_last_state        ();

// Controls
void ta_ui_row_begin                ();
void ta_ui_row_end                  ();
void ta_ui_spacer                   (int w, int h);
void ta_ui_window_begin             (ta_ui_window_state *window, u32 flags);
void ta_ui_window_end               ();
void ta_ui_panel_begin              (ta_ui_panel_state *panel, u32 flags);
void ta_ui_panel_end                ();
void ta_ui_button_begin             (u32 flags);
bool ta_ui_button_end               ();
bool ta_ui_button                   (const char *text, size_t text_len);
bool ta_ui_reset_button             ();
void ta_ui_toggle_button_begin      (u32 flags);
bool ta_ui_toggle_button_end        (bool *checked);
bool ta_ui_toggle_button            (const char *false_text, size_t false_text_len, const char *true_text, size_t true_text_len, bool *checked);
bool ta_ui_image                    (const char *texture);
void ta_ui_label                    (const char *text, size_t text_len);
void ta_ui_label_float              (float value);

void textbox_command_cursor_right   ();
void textbox_command_cursor_left    ();
void textbox_command_cursor_down    ();
void textbox_command_cursor_up      ();
void textbox_command_cursor_bol     ();
void textbox_command_cursor_eol     ();
void textbox_command_cursor_bof     ();
void textbox_command_cursor_eof     ();
void textbox_command_delete         ();
void textbox_command_backspace      ();
void textbox_command_submit         ();
void textbox_command_cancel         ();
void ta_ui_textbox_set_text         (ta_ui_textbox_state *textbox, const char *text, size_t text_len);

bool ta_ui_textbox                      (const char *text, size_t text_len, ta_ui_textbox_state *textbox, u32 flags);
bool ta_ui_textbox_float                (float *value, ta_ui_textbox_state *textbox, u32 flags);
bool ta_ui_textbox_float_reset          (float *value, ta_ui_textbox_state *textbox, u32 flags, float reset_value);
void ta_ui_textbox_vec2                 (ta_vec2 *vec, ta_ui_textbox_vec2_state* vec_state);
void ta_ui_textbox_vec2_normalize       (ta_vec2 *vec, ta_ui_textbox_vec2_state* vec_state);
void ta_ui_textbox_vec2_reset           (ta_vec2 *vec, ta_ui_textbox_vec2_state* vec_state, ta_vec2 reset_value);
void ta_ui_textbox_vec2_reset_normalize (ta_vec2 *vec, ta_ui_textbox_vec2_state* vec_state, ta_vec2 reset_value);
void ta_ui_textbox_vec3                 (ta_vec3 *vec, ta_ui_textbox_vec3_state* vec_state, bool normalize, bool reset_button);
void ta_ui_textbox_vec4                 (ta_vec4 *vec, ta_ui_textbox_vec4_state* vec_state, bool normalize, bool reset_button);
void ta_ui_textbox_focus                (ta_ui_textbox_state *textbox);
bool ta_ui_textbox_insert               (ta_ui_textbox_state *textbox, char c);
void ta_ui_textbox_clear                (ta_ui_textbox_state *textbox);
void ta_ui_textbox_submit               (ta_ui_textbox_state *textbox);
void ta_ui_textbox_cancel               (ta_ui_textbox_state *textbox);
void ta_ui_tooltip_begin                (const char *name);
void ta_ui_tooltip_end                  (const char *name);
void ta_ui_tooltip                      (const char *text, size_t text_len);
//void ta_ui_statusbar();

// Actions
void ta_ui_render();