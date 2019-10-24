#pragma once
#include "ta_uid.h"
#include "dlb/dlb_types.h"

typedef enum e_button_state {
    TA_BUTTON_INACTIVE,
    TA_BUTTON_ACTIVE,
} e_button_state;

typedef struct ta_e_button {
    u32 id;
    u32 entity_id;
    e_button_state state;
    e_button_state state_prev;
    u32 sfx_activated_id;  // TODO: Some sort of event-based sound effects handling?
    u32 sfx_active_id;
    u32 sfx_deactivated_id;
} ta_e_button;

void e_button_init(ta_e_button *button);
void e_button_update(ta_e_button *button);