#pragma once
#include "ta_scene.h"
#include "dlb_types.h"
#include "AL/al.h"
#include "AL/alc.h"

typedef struct ta_audio_listener_s {
    ALfloat volume;
    bool muted;

    ALCdevice *al_device;
    ALCcontext *al_context;
} ta_audio_listener;

typedef struct ta_audio_buffer_s {
    ta_uid uid;
    const char *path;     // File path
    ta_buffer *samples;   // Audio data        (if inlined instead of via path)
    ALuint al_buffer_id;  // OpenAL buffer id
} ta_audio_buffer;

typedef enum {
    TA_AUDIO_STOPPED,
    TA_AUDIO_PLAYING,
    TA_AUDIO_PAUSED
} ta_audio_source_state;

typedef struct ta_audio_source_s {
    ta_uid uid;
    ta_audio_source_state state;
    float pitch;
    float gain;
    bool loop;
    ALuint al_source_id;
    const char *audio_buffer_uid;
} ta_audio_source;

void ta_audio_listener_init(ta_audio_listener *audio);
void ta_audio_listener_set_volume(ta_audio_listener *audio, float volume);
bool ta_audio_listener_muted(ta_audio_listener *audio);
void ta_audio_listener_mute(ta_audio_listener *audio);
void ta_audio_listener_unmute(ta_audio_listener *audio);
void ta_audio_listener_toggle(ta_audio_listener *audio);

void ta_audio_buffer_init(ta_audio_buffer *buffer);
void ta_audio_buffer_load_path(ta_audio_buffer *buffer, const char *path);
void ta_audio_buffer_set_samples(ta_audio_buffer *buffer, ta_buffer *samples);
void ta_audio_buffer_load(ta_audio_buffer *buffer);
void ta_audio_buffer_free(ta_audio_buffer *buffer);

void ta_audio_source_init(ta_audio_source *source);
void ta_audio_source_free(ta_audio_source *source);
void ta_audio_source_set_pitch(ta_audio_source *source, float pitch);
void ta_audio_source_set_gain(ta_audio_source *source, float gain);
void ta_audio_source_set_buffer(ta_audio_source *source, ta_audio_buffer *buffer);
void ta_audio_source_play(ta_audio_source *source);
void ta_audio_source_play_loop(ta_audio_source *source);
void ta_audio_source_pause(ta_audio_source *source);
void ta_audio_source_resume(ta_audio_source *source);
void ta_audio_source_stop(ta_audio_source *source);