#pragma once
#include "dlb/dlb_types.h"

struct ta_event;
struct ta_ray;
struct ta_ui_textbox_state;

void ta_editor_init             ();
void ta_editor_select_entity    (const char *entity);
void ta_editor_selected_entity  (const char **out_entity);
bool ta_editor_textbox_editing  ();
void ta_editor_update_widgets   ();
void ta_editor_hotkeys          ();
void ta_editor_textbox_event    (struct ta_event *event);
void ta_editor_select_ray       (struct ta_ray *ray);
void ta_editor_draw_world       ();
void ta_editor_draw_screen      ();
