#pragma once
#include "dlb/dlb_types.h"
#include "ta_uid.h"
#include "ta_audio.h"

typedef enum e_button_state {
    TA_BUTTON_INACTIVE,
    TA_BUTTON_ACTIVE,
} e_button_state;

typedef struct e_button {
    ta_uid uid;
    e_button_state state_prev;
    e_button_state state;

    const char *audio_source_uid;
    const char *sfx_activated_uid;
    const char *sfx_active_uid;
    const char *sfx_deactivated_uid;
} e_button;

struct ta_node;

void e_button_init(e_button *button);
ta_audio_source *e_button_audio_source(e_button *button);
ta_audio_buffer *e_button_sfx_activated(e_button *button);
ta_audio_buffer *e_button_sfx_active(e_button *button);
ta_audio_buffer *e_button_sfx_deactivated(e_button *button);
void e_button_update(struct ta_node *node);