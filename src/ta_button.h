#pragma once
#include "ta_uid.h"
#include "dlb/dlb_types.h"

typedef enum e_button_state {
    TA_BUTTON_INACTIVE,
    TA_BUTTON_ACTIVE,
} e_button_state;

typedef struct ta_e_button {
    u32 index;
    const char *name;
    const char *entity_name;
    e_button_state state;
    e_button_state state_prev;
    u32 sfx_activated_name;  // TODO: Some sort of event-based sound effects handling?
    u32 sfx_active_name;
    u32 sfx_deactivated_name;
} ta_e_button;

void e_button_init(ta_e_button *button);
void e_button_update(ta_e_button *button);