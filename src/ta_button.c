#include "ta_button.h"
#include "ta_game.h"
#include "ta_event.h"
#include "ta_rigid_body.h"
#include "ta_scene.h"
#include "ta_node.h"
#include "ta_entity.h"
#include "ta_audio.h"

void e_button_init(ta_e_button *button)
{
    UNUSED(button);
}

static bool button_activated(ta_e_button *button)
{
    bool activated =
        button->state == TA_BUTTON_ACTIVE &&
        button->state_prev == TA_BUTTON_INACTIVE;
    return activated;
}
static bool button_active(ta_e_button *button)
{
    bool active = button->state == TA_BUTTON_ACTIVE;
    return active;
}
static bool button_deactivated(ta_e_button *button)
{
    bool deactivated =
        button->state == TA_BUTTON_INACTIVE &&
        button->state_prev == TA_BUTTON_ACTIVE;
    return deactivated;
}
void e_button_update(ta_e_button *button)
{
    // TODO: Trigger event type = EVENT_BUTTON_ACTIVATED with uid = button.uid
    //       and have audio_buffer's play event subscribe to the button event
    //       Another example would be having a light subscribe to this event.
    //
    //       EVENT_BUTTON_ACTIVATED
    //       EVENT_BUTTON_DEACTIVATED
    //       EVENT_BUTTON_STATE_CHANGED

    ta_entity *e_button = ta_scene_find_by_id(tg_game.scene, RES_ENTITY, button->entity_id);
    ta_entity *e_player = ta_scene_find_by_id(tg_game.scene, RES_ENTITY, tg_game.player_one_id);
    ta_rigid_body *button_body = ta_scene_entity_component(tg_game.scene, e_button, RES_COMP_RIGID_BODY);
    ta_rigid_body *player_body = ta_scene_entity_component(tg_game.scene, e_player, RES_COMP_RIGID_BODY);
    DLB_ASSERT(button_body);
    DLB_ASSERT(player_body);

    button->state_prev = button->state;
    if (ta_rigid_body_intersect(player_body, button_body, 0)) {
        button->state = TA_BUTTON_ACTIVE;
    } else {
        button->state = TA_BUTTON_INACTIVE;
    }

    if (button_activated(button)) {
        ta_event event = { 0 };
        event.data.button.button_id = button->id;

        event.type = TA_EVENT_GAME_BUTTON_ACTIVATED;
        ta_event_push(&event);
        event.type = TA_EVENT_GAME_BUTTON_STATE_CHANGED;
        ta_event_push(&event);
    }

    if (button_deactivated(button)) {
        ta_event event = { 0 };
        event.data.button.button_id = button->id;

        event.type = TA_EVENT_GAME_BUTTON_DEACTIVATED;
        ta_event_push(&event);
        event.type = TA_EVENT_GAME_BUTTON_STATE_CHANGED;
        ta_event_push(&event);
    }

#if 0
    // TODO: Subscribe audio buffer to button events
    ta_audio_buffer *buffer = e_button_sfx_activated(button);
    if (buffer) {
        ta_audio_source *source = e_button_audio_source(button);
        ta_audio_source_set_buffer(source, buffer);
        ta_audio_source_play(source);
    }

    // TODO: Queue looping active sound so that it plays after non-looping
    //       activation sound finishes. E.g. to make button hum while active.
    if (button_active(button)) {
        ta_audio_buffer *buffer = e_button_sfx_active(button);
        if (buffer) {
            ta_audio_source *source = e_button_audio_source(button);
            ta_audio_source_set_buffer(source, buffer);
        }
    }

    // TODO: Subscribe audio buffer to button events
    ta_audio_buffer *buffer = e_button_sfx_deactivated(button);
    if (buffer) {
        ta_audio_source *source = e_button_audio_source(button);
        ta_audio_source_set_buffer(source, buffer);
        ta_audio_source_play(source);
    }
#endif
}