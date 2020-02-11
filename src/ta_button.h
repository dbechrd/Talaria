#pragma once
#include "ta_schema.h"
#include "dlb/dlb_types.h"

typedef enum e_button_state {
    TA_BUTTON_INACTIVE,
    TA_BUTTON_ACTIVE,
} e_button_state;

typedef struct ta_e_button {
    TA_COMPONENT_HEADER
    e_button_state  state;              // current button state
    e_button_state  state_prev;         // previous button state

    // TODO: Some sort of event-based sound effects handling?
    const char      *sfx_activated;     // sound effect when button activated
    const char      *sfx_active;        // sound effect while button active
    const char      *sfx_deactivated;   // sound effect when button deactivated
} ta_e_button;

void e_button_init      (ta_e_button *button);
void e_button_update    (ta_e_button *button);