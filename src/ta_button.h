#pragma once
#include "ta_uid.h"
#include "dlb/dlb_types.h"

typedef enum e_button_state {
    TA_BUTTON_INACTIVE,
    TA_BUTTON_ACTIVE,
} e_button_state;

typedef struct e_button {
    e_button_state state;
    e_button_state state_prev;

    // TODO: Some sort of event-based sound effects handling?
    ta_handle sfx_activated;
    ta_handle sfx_active;
    ta_handle sfx_deactivated;
} e_button;

void e_button_init(e_button *button);
void e_button_update(e_button *button);