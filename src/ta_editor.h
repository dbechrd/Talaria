#pragma once

struct ta_text_entry;
struct ta_node;

typedef struct ta_editor ta_editor;
extern ta_editor *tg_editor;

void ta_editor_set_active_text_entry(struct ta_text_entry *text_entry);
struct ta_text_entry *ta_editor_active_text_entry();
void ta_editor_select_node(struct ta_node *node);
struct ta_node *ta_editor_selected_node();

void ta_editor_draw();