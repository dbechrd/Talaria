#pragma once
#include "ta_schema.h"
#include "dlb/dlb_types.h"
#include "AL/al.h"
#include "AL/alc.h"

// TODO: Move to somewhere more general
typedef enum ta_result {
    TA_OK,
    TA_MISSING_RESOURCE,
} ta_result;

typedef struct ta_audio_listener {
    ALfloat volume; // Net gain for this listener
    bool    mute;   // If true, mute this listener
} ta_audio_listener;

typedef struct ta_audio_buffer {
    TA_RESOURCE_HEADER
    const char  *path;          // Relative path of audio file loaded into this buffer
    char        *samples;       // Audio data (if inlined instead of via path)
    ALsizei     samples_bytes;  // Length of samples buffer in bytes
    ALuint      al_buffer_id;   // [OpenAL] ALBuffer id
} ta_audio_buffer;

typedef enum ta_audio_source_state {
    TA_AUDIO_STOPPED = AL_STOPPED,
    TA_AUDIO_PLAYING = AL_PLAYING,
    TA_AUDIO_PAUSED  = AL_PAUSED
} ta_audio_source_state;

typedef struct ta_audio_source {
    TA_COMPONENT_HEADER
    const char  *audio_buffer;  // [SYM] Name of audio buffer currently attached to this audio source
    float       pitch;          // Audio pitch
    float       gain;           // Audio gain
    bool        loop;           // If true, play in looping mode
    ALuint      al_source_id;   // [OpenAL] ALSource id
} ta_audio_source;

struct ta_buffer;

extern ta_audio_listener tg_audio_listener;

void ta_audio_init();
void ta_audio_update();
void ta_audio_free();

void ta_audio_listener_init         (ta_audio_listener *listener);
float ta_audio_listener_get_volume  (ta_audio_listener *listener);
void ta_audio_listener_set_volume   (ta_audio_listener *listener, float volume);
bool ta_audio_listener_muted        (ta_audio_listener *listener);
void ta_audio_listener_mute         (ta_audio_listener *listener);
void ta_audio_listener_unmute       (ta_audio_listener *listener);
void ta_audio_listener_toggle       (ta_audio_listener *listener);

void ta_audio_buffer_init           (ta_audio_buffer *buffer);
void ta_audio_buffer_init_void      (void *buffer);
void ta_audio_buffer_load_path      (ta_audio_buffer *buffer, const char *path);
void ta_audio_buffer_set_samples    (ta_audio_buffer *buffer, char *samples, ALsizei samples_bytes);
void ta_audio_buffer_load           (ta_audio_buffer *buffer);
double ta_audio_buffer_duration_ms  (ta_audio_buffer *buffer);
void ta_audio_buffer_free           (ta_audio_buffer *buffer);

void ta_audio_source_init           (ta_audio_source *source);
void ta_audio_source_init_void      (void *source);
void ta_audio_source_free           (ta_audio_source *source);
void ta_audio_source_free_void      (void *source);
void ta_audio_source_set_pitch      (ta_audio_source *source, float pitch);
void ta_audio_source_set_gain       (ta_audio_source *source, float gain);
ta_result ta_audio_source_set_buffer(ta_audio_source *source, const char *audio_buffer);
void ta_audio_source_get_state      (ta_audio_source *source, ta_audio_source_state *state);
void ta_audio_source_play           (ta_audio_source *source);
void ta_audio_source_play_name      (ta_audio_source *source, const char *audio_buffer);
void ta_audio_source_play_loop      (ta_audio_source *source);
void ta_audio_source_play_loop_name (ta_audio_source *source, const char *audio_buffer);
void ta_audio_source_pause          (ta_audio_source *source);
void ta_audio_source_resume         (ta_audio_source *source);
void ta_audio_source_stop           (ta_audio_source *source);