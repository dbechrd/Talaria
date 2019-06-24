#pragma once
#include "ta_entity.h"
#include "ta_audio.h"

typedef enum ta_ent_button_state {
    TA_ENT_BUTTON_INACTIVE,
    TA_ENT_BUTTON_ACTIVE,
} ta_ent_button_state;

typedef struct ta_ent_button_s {
    ta_entity base;
    ta_ent_button_state state_prev;
    ta_ent_button_state state;

    const char *audio_source_uid;
    const char *sfx_activated_uid;
    const char *sfx_active_uid;
    const char *sfx_deactivated_uid;
} ta_ent_button;

void ta_ent_button_init(ta_ent_button *button);
ta_audio_source *ta_ent_button_audio_source(ta_ent_button *button);
ta_audio_buffer *ta_ent_button_sfx_activated(ta_ent_button *button);
ta_audio_buffer *ta_ent_button_sfx_active(ta_ent_button *button);
ta_audio_buffer *ta_ent_button_sfx_deactivated(ta_ent_button *button);
void ta_ent_button_update(ta_entity *base);