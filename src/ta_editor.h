#pragma once
#include "ta_text_entry.h"

typedef struct ta_node ta_node;

typedef struct ta_editor ta_editor;
extern ta_editor *tg_editor;

void ta_editor_set_active_text_entry(ta_text_entry *text_entry);
ta_text_entry *ta_editor_active_text_entry();
void ta_editor_select_node(ta_node *node);
ta_node *ta_editor_selected_node();