#pragma once
#include "dlb/dlb_types.h"

typedef struct ta_texture ta_texture;
typedef struct ta_text_entry ta_text_entry;

typedef struct ta_ui_state {
    bool hover;
    bool down;
    bool pressed;
    bool released;
    bool unfocused;
} ta_ui_state;

ta_ui_state ta_ui_last_frame_state();

void ta_ui_next_margin(int left, int top, int right, int bottom);
void ta_ui_next_pad(int left, int top, int right, int bottom);
void ta_ui_next_size(int w, int h);
void ta_ui_next_bg_color(float r, float g, float b, float a);
void ta_ui_next_fg_color(float r, float g, float b, float a);

void ta_ui_window_begin(const char *name, int *scroll_v);
void ta_ui_window_end();

void ta_ui_panel_begin(const char *name, u32 *index);
void ta_ui_panel_end(u32 index);

void ta_ui_row_end();
void ta_ui_row_begin();

void ta_ui_tooltip(const char *text, u32 text_len);
void ta_ui_statusbar();
bool ta_ui_button(const char *name, const ta_texture *tex);
bool ta_ui_button_toggle(const char *name, const ta_texture *tex, bool *active);
bool ta_ui_label(const char *name, const char *text);
bool ta_ui_textbox(const char *name, ta_text_entry *text_entry);