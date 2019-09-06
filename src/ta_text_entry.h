#pragma once
#include "ta_math.h"
#include "dlb/dlb_types.h"

struct ta_event;

typedef struct ta_text_entry ta_text_entry;
typedef bool ta_text_entry_filter(char c);

ta_text_entry_filter *ta_text_entry_filter_default;

ta_text_entry *ta_text_entry_init();
void ta_text_entry_free(ta_text_entry **text_entry);
void ta_text_entry_set_filter(ta_text_entry *text_entry,
    ta_text_entry_filter *filter);
void ta_text_entry_focus(ta_text_entry *text_entry);
void ta_text_entry_unfocus(ta_text_entry *text_entry);
bool ta_text_entry_focused(ta_text_entry *text_entry);
void ta_text_entry_validate(ta_text_entry *text_entry);
bool ta_text_entry_valid(ta_text_entry *text_entry);
void ta_text_entry_submit(ta_text_entry *text_entry);
bool ta_text_entry_submitted(ta_text_entry *text_entry);
void ta_text_entry_reject(ta_text_entry *text_entry);
void ta_text_entry_cancel(ta_text_entry *text_entry);
bool ta_text_entry_canceled(ta_text_entry *text_entry);
char *ta_text_entry_text(ta_text_entry *text_entry, u32 *len);
void ta_text_entry_set_text(ta_text_entry *text_entry, const char *str, u32 len);
ta_rectf ta_text_entry_draw(ta_text_entry *text_entry, ta_rect_uv **text_rects,
    ta_vec2 *cursor);
void ta_text_entry_event(struct ta_event *event);