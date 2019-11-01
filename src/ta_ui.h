#pragma once
#include "dlb/dlb_types.h"

struct ta_texture;
struct ta_text_entry;

typedef enum ui_state_type {
    UI_STATE_NONE,
    UI_STATE_HOVER,
    UI_STATE_DOWN,
    UI_STATE_ACTIVE,
    UI_STATE_COUNT,
    UI_STATE_ALL
} ui_state_type;

typedef struct ta_ui_state {
    bool hover;
    bool down;
    bool pressed;
    bool released;
    bool unfocused;
} ta_ui_state;

void ta_ui_init();
ta_ui_state ta_ui_last_frame_state();
void ta_ui_next_margin(int left, int top, int right, int bottom);
void ta_ui_next_pad(int left, int top, int right, int bottom);
void ta_ui_next_size(int w, int h);
void ta_ui_next_invisible();
void ta_ui_next_bg_color(ui_state_type state, float r, float g, float b, float a);
void ta_ui_next_fg_color(ui_state_type state, float r, float g, float b, float a);

void ta_ui_window_begin(const char *name, int *scroll_v);
void ta_ui_window_end();

void ta_ui_panel_begin(const char *name, u32 *index);
void ta_ui_panel_end(u32 index);

void ta_ui_row_end();
void ta_ui_row_begin();

void ta_ui_spacer(int w, int h);
void ta_ui_tooltip(const char *text, u32 text_len);
void ta_ui_statusbar();
bool ta_ui_button(const char *name, const struct ta_texture *tex, int face);
bool ta_ui_button_toggle(const char *name, const struct ta_texture *tex, bool *active);
bool ta_ui_label(const char *name, const char *text);
bool ta_ui_textbox(const char *name, struct ta_text_entry *text_entry);