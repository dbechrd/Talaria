#pragma once
#include "dlb/dlb_types.h"
#include "dlb/dlb_index.h"
#include "AL/al.h"
#include "AL/alc.h"

typedef struct ta_audio_listener {
    ALfloat volume;
    bool muted;
    ALCdevice *al_device;
    ALCcontext *al_context;
} ta_audio_listener;

typedef struct ta_audio_buffer {
    u32 index;
    const char *name;
    const char *path;           // File path
    struct ta_buffer *samples;  // Audio data (if inlined instead of via path)
    ALuint al_buffer_id;        // OpenAL buffer id
} ta_audio_buffer;

typedef enum ta_audio_source_state {
    TA_AUDIO_STOPPED = AL_STOPPED,
    TA_AUDIO_PLAYING = AL_PLAYING,
    TA_AUDIO_PAUSED = AL_PAUSED
} ta_audio_source_state;

typedef struct ta_audio_source {
    u32 index;
    const char *name;
    const char *entity_name;
    const char *audio_buffer_name;
    float pitch;
    float gain;
    bool loop;
    ALuint al_source_id;
} ta_audio_source;

typedef struct ta_buffer ta_buffer;

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
double ta_audio_buffer_duration_ms(ta_audio_buffer *buffer);
void ta_audio_buffer_free(ta_audio_buffer *buffer);

void ta_audio_source_init(ta_audio_source *source);
void ta_audio_source_free(ta_audio_source *source);
void ta_audio_source_set_pitch(ta_audio_source *source, float pitch);
void ta_audio_source_set_gain(ta_audio_source *source, float gain);
void ta_audio_source_set_buffer(ta_audio_source *source,
    const char *audio_buffer_name);
ta_audio_source_state ta_audio_source_get_state(ta_audio_source *source);
void ta_audio_source_play(ta_audio_source *source);
void ta_audio_source_play_name(ta_audio_source *source,
    const char *audio_buffer_name);
void ta_audio_source_play_loop(ta_audio_source *source);
void ta_audio_source_play_loop_name(ta_audio_source *source,
    const char *audio_buffer_name);
void ta_audio_source_pause(ta_audio_source *source);
void ta_audio_source_resume(ta_audio_source *source);
void ta_audio_source_stop(ta_audio_source *source);