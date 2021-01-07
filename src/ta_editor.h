#pragma once
#include "dlb/dlb_types.h"

struct ta_event;
struct ta_ray;
struct ta_rgba;

void ta_editor_init                 ();
void ta_editor_reset_frame          ();
void ta_editor_select_entity        (const char *entity);
void ta_editor_selected_entity      (const char **out_entity);
bool ta_editor_textbox_editing      ();
void ta_editor_select_ray           (struct ta_ray *ray);
const char *ta_editor_closest_entity();
void ta_editor_textbox_event        (struct ta_event *event);
void ta_editor_update_widgets       ();
void ta_editor_draw_entity_wireframe(const char *entity, struct ta_rgba color, bool pulse_color);
void ta_editor_draw_world           ();
void ta_editor_draw_screen          ();
