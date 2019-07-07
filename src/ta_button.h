#pragma once
#include "ta_audio.h"

typedef enum ta_button_state {
    ta_button_INACTIVE,
    ta_button_ACTIVE,
} ta_button_state;

typedef struct ta_button_s {
	ta_scene_ref ref;
    ta_button_state state_prev;
    ta_button_state state;

    const char *audio_source_uid;
    const char *sfx_activated_uid;
    const char *sfx_active_uid;
    const char *sfx_deactivated_uid;
} ta_button;

void ta_button_init(ta_button *button);
ta_audio_source *ta_button_audio_source(ta_button *button);
ta_audio_buffer *ta_button_sfx_activated(ta_button *button);
ta_audio_buffer *ta_button_sfx_active(ta_button *button);
ta_audio_buffer *ta_button_sfx_deactivated(ta_button *button);
void ta_button_update(ta_button *button);