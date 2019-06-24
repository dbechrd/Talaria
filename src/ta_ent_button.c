#include "ta_ent_button.h"
#include "ta_game.h"

void ta_ent_button_init(ta_ent_button *button)
{
    UNUSED(button);
}

ta_audio_source *ta_ent_button_audio_source(ta_ent_button *button) {
    if (!button->audio_source_uid) return 0;

    // NOTE: This could cache in button->audio_source
    ta_audio_source *audio_source = ta_scene_find_by_ref(&button->base.ref,
        F_TA_AUDIO_SOURCE, button->audio_source_uid);
    return audio_source;
}
ta_audio_buffer *ta_ent_button_sfx_activated(ta_ent_button *button) {
    if (!button->sfx_activated_uid) return 0;

    // NOTE: This could cache in button->sfx_activated_buffer
    ta_audio_buffer *audio_buffer = ta_scene_find_by_ref(&button->base.ref,
        F_TA_AUDIO_BUFFER, button->sfx_activated_uid);
    return audio_buffer;
}
ta_audio_buffer *ta_ent_button_sfx_active(ta_ent_button *button) {
    if (!button->sfx_active_uid) return 0;

    // NOTE: This could cache in button->sfx_active_buffer
    ta_audio_buffer *audio_buffer = ta_scene_find_by_ref(&button->base.ref,
        F_TA_AUDIO_BUFFER, button->sfx_active_uid);
    return audio_buffer;
}
ta_audio_buffer *ta_ent_button_sfx_deactivated(ta_ent_button *button) {
    if (!button->sfx_deactivated_uid) return 0;

    // NOTE: This could cache in button->sfx_deactivated_buffer
    ta_audio_buffer *audio_buffer = ta_scene_find_by_ref(&button->base.ref,
        F_TA_AUDIO_BUFFER, button->sfx_deactivated_uid);
    return audio_buffer;
}

static bool button_activated(ta_ent_button *button) {
    bool activated =
        button->state == TA_ENT_BUTTON_ACTIVE &&
        button->state_prev == TA_ENT_BUTTON_INACTIVE;
    return activated;
}
static bool button_active(ta_ent_button *button) {
    bool active = button->state == TA_ENT_BUTTON_ACTIVE;
    return active;
}
static bool button_deactivated(ta_ent_button *button) {
    bool deactivated =
        button->state == TA_ENT_BUTTON_INACTIVE &&
        button->state_prev == TA_ENT_BUTTON_ACTIVE;
    return deactivated;
}
void ta_ent_button_update(ta_entity *base) {
    ta_ent_button *button = (ta_ent_button *)base;
    ta_rigid_body *button_body = ta_entity_rigid_body(base);
    ta_rigid_body *player_body = ta_entity_rigid_body(tg_game.player);

    button->state_prev = button->state;
    if (ta_rigid_body_intersect(player_body, button_body, 0)) {
        button->state = TA_ENT_BUTTON_ACTIVE;
    } else {
        button->state = TA_ENT_BUTTON_INACTIVE;
    }

    if (button_activated(button)) {
        ta_audio_buffer *buffer = ta_ent_button_sfx_activated(button);
        if (buffer) {
            ta_audio_source *source = ta_ent_button_audio_source(button);
            ta_audio_source_set_buffer(source, buffer);
            ta_audio_source_play(source);
        }
    }

#if 0
    // TODO: Queue looping active sound so that it plays after non-looping
    //       activation sound finishes.
    if (button_active(button)) {
        ta_audio_buffer *buffer = ta_ent_button_sfx_active(button);
        if (buffer) {
            ta_audio_source *source = ta_ent_button_audio_source(button);
            ta_audio_source_set_buffer(source, buffer);
        }
    }
#endif

    if (button_deactivated(button)) {
        ta_audio_buffer *buffer = ta_ent_button_sfx_deactivated(button);
        if (buffer) {
            ta_audio_source *source = ta_ent_button_audio_source(button);
            ta_audio_source_set_buffer(source, buffer);
            ta_audio_source_play(source);
        }
    }
}