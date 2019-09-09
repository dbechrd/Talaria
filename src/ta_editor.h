#pragma once
#include "dlb/dlb_types.h"

struct ta_text_entry;
struct ta_node;
struct ta_event;

void ta_editor_init();
void ta_editor_set_active_text_entry(struct ta_text_entry *text_entry);
struct ta_text_entry *ta_editor_active_text_entry();
void ta_editor_select_node(struct ta_node *node);
struct ta_node *ta_editor_selected_node();
void ta_editor_draw();
void ta_editor_hotkeys();
void ta_editor_event(struct ta_event *event);