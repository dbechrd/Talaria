#include "ta_button.h"
#include "ta_game.h"
#include "ta_event.h"
#include "ta_rigid_body.h"
#include "ta_scene.h"
#include "ta_node.h"

void e_button_init(e_button *button)
{
    UNUSED(button);
}

ta_audio_source *e_button_audio_source(e_button *button)
{
    if (!button->audio_source_uid) return 0;

    // NOTE: This could cache in button->audio_source
    ta_audio_source *audio_source = ta_scene_find(button->uid.scene,
        TYP_AUDIO_SOURCE, button->audio_source_uid);
    return audio_source;
}
ta_audio_buffer *e_button_sfx_activated(e_button *button)
{
    if (!button->sfx_activated_uid) return 0;

    // NOTE: This could cache in button->sfx_activated_buffer
    ta_audio_buffer *audio_buffer = ta_scene_find(button->uid.scene,
        TYP_AUDIO_BUFFER, button->sfx_activated_uid);
    return audio_buffer;
}
ta_audio_buffer *e_button_sfx_active(e_button *button)
{
    if (!button->sfx_active_uid) return 0;

    // NOTE: This could cache in button->sfx_active_buffer
    ta_audio_buffer *audio_buffer = ta_scene_find(button->uid.scene,
        TYP_AUDIO_BUFFER, button->sfx_active_uid);
    return audio_buffer;
}
ta_audio_buffer *e_button_sfx_deactivated(e_button *button)
{
    if (!button->sfx_deactivated_uid) return 0;

    // NOTE: This could cache in button->sfx_deactivated_buffer
    ta_audio_buffer *audio_buffer = ta_scene_find(button->uid.scene,
        TYP_AUDIO_BUFFER, button->sfx_deactivated_uid);
    return audio_buffer;
}

static bool button_activated(e_button *button)
{
    bool activated =
        button->state == TA_BUTTON_ACTIVE &&
        button->state_prev == TA_BUTTON_INACTIVE;
    return activated;
}
static bool button_active(e_button *button)
{
    bool active = button->state == TA_BUTTON_ACTIVE;
    return active;
}
static bool button_deactivated(e_button *button)
{
    bool deactivated =
        button->state == TA_BUTTON_INACTIVE &&
        button->state_prev == TA_BUTTON_ACTIVE;
    return deactivated;
}
void e_button_update(ta_node *node)
{
    // TODO: Trigger event type = EVENT_BUTTON_ACTIVATED with uid = button.uid
    //       and have audio_buffer's play event subscribe to the button event
    //       Another example would be having a light subscribe to this event.
    //
    //       EVENT_BUTTON_ACTIVATED
    //       EVENT_BUTTON_DEACTIVATED
    //       EVENT_BUTTON_STATE_CHANGED

    e_button *button = ta_scene_find(tg_game.scene, TYP_BUTTON, node->button_uid);
    DLB_ASSERT(button);

    ta_rigid_body *button_body = ta_node_rigid_body(node);
    ta_rigid_body *player_body = ta_node_rigid_body(tg_game.player);

    button->state_prev = button->state;
    if (ta_rigid_body_intersect(player_body, button_body, 0)) {
        button->state = TA_BUTTON_ACTIVE;
    } else {
        button->state = TA_BUTTON_INACTIVE;
    }

    if (button_activated(button)) {
        ta_event event = { 0 };
        event.data.button.button_uid = button->uid.uid;

        event.type = TA_EVENT_GAME_BUTTON_ACTIVATED;
        ta_event_push(&event);
        event.type = TA_EVENT_GAME_BUTTON_STATE_CHANGED;
        ta_event_push(&event);
    }

    if (button_deactivated(button)) {
        ta_event event = { 0 };
        event.data.button.button_uid = button->uid.uid;

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