#pragma once
#include "ta_uid.h"
#include "dlb/dlb_types.h"

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

typedef struct ta_audio_buffer ta_audio_buffer;
typedef struct ta_audio_source ta_audio_source;
typedef struct ta_node ta_node;

void e_button_init(e_button *button);
ta_audio_source *e_button_audio_source(e_button *button);
ta_audio_buffer *e_button_sfx_activated(e_button *button);
ta_audio_buffer *e_button_sfx_active(e_button *button);
ta_audio_buffer *e_button_sfx_deactivated(e_button *button);
void e_button_update(ta_node *node);