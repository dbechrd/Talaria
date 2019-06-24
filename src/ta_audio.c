#include "ta_audio.h"
#include "ta_log.h"
#include "ta_file.h"
#include "dlb_memory.h"

#define TA_AUDIO_SAMPLE_RATE 44100

void ta_audio_listener_init(ta_audio_listener *listener)
{
    if (!listener->volume) {
        listener->volume = 1.0f;
    }

    const char *devices = alcGetString(NULL, ALC_ALL_DEVICES_SPECIFIER);
    const char *s = devices;
    while (*s)
    {
        ta_log_write(tg_debug_log, "[Audio] Device: %s\n", s);
        while (*s) { s++; }
        s++;
    }

    // TODO: Allow user to set which device they want to use
    listener->al_device = alcOpenDevice(NULL);
    if (!listener->al_device)
    {
        DLB_ASSERT(!"Failed to open listener context");
        return;
    }

    // TODO: What other attributes can I specify on a context?
    ALCint attrlist[] = { ALC_FREQUENCY, TA_AUDIO_SAMPLE_RATE, 0 };
    listener->al_context = alcCreateContext(listener->al_device, attrlist);
    if (!listener->al_context)
    {
        DLB_ASSERT(!"Failed to create listener context");
        return;
    }

    // TODO: Can I have more than one context, is that useful?
    if (!alcMakeContextCurrent(listener->al_context))
    {
        DLB_ASSERT(!"Failed to activate listener context");
        return;
    }

    ALenum err = alGetError();
    if (err) {
        DLB_ASSERT(!"OpenAL error");
        //DLB_ASSERT(!"OpenAL error: %s\n", err);
    }

    ta_audio_listener_set_volume(listener, listener->volume);
}

void ta_audio_listener_set_volume(ta_audio_listener *listener,
    float volume)
{
    listener->volume = volume;
    if (!listener->muted) {
        alListenerf(AL_GAIN, listener->volume);
    }
}
bool ta_audio_listener_muted(ta_audio_listener *listener)
{
    return listener->muted;
}
void ta_audio_listener_mute(ta_audio_listener *listener)
{
    listener->muted = true;
    alListenerf(AL_GAIN, 0.0f);
}
void ta_audio_listener_unmute(ta_audio_listener *listener)
{
    listener->muted = false;
    alListenerf(AL_GAIN, listener->volume);
}
void ta_audio_listener_toggle(ta_audio_listener *listener)
{
    if (listener->muted) {
        ta_audio_listener_unmute(listener);
    } else {
        ta_audio_listener_mute(listener);
    }
}
#if 0
void ta_audio_listener_set_position(ta_audio_listener *listener, ta_vec3 pos)
{
    alListenerfv(AL_ORIENTATION, (float *)fwd_up);
    alListenerfv(AL_POSITION, (float *)&pos);
    alListenerfv(AL_VELOCITY, (float *)&vel);
}
#endif

void ta_audio_buffer_init(ta_audio_buffer *buffer)
{
    if (buffer->path) {
        ta_audio_buffer_load_path(buffer, buffer->path);
    } else if(buffer->samples->data) {
        DLB_ASSERT(buffer->samples->length);
        ta_audio_buffer_load(buffer);
    }
}
void ta_audio_buffer_load_path(ta_audio_buffer *buffer, const char *path)
{
    buffer->path = path;

    // Load sample data from file
    ta_buffer *samples = ta_file_read_all(buffer->path);
    ta_audio_buffer_set_samples(buffer, samples);
    ta_audio_buffer_load(buffer);
}
void ta_audio_buffer_set_samples(ta_audio_buffer *buffer, ta_buffer *samples)
{
    DLB_ASSERT(!buffer->samples);
    buffer->samples = samples;
}

#include <math.h>
void ta_audio_buffer_load(ta_audio_buffer *buffer)
{
    DLB_ASSERT(buffer->samples);
    DLB_ASSERT(buffer->samples->data);
    DLB_ASSERT(buffer->samples->length);

    alGenBuffers(1, &buffer->al_buffer_id);
#if 0
    const u32 AMPLITUDE = 2000;
    s16 buf[TA_AUDIO_SAMPLE_RATE];

    //const double TWO_PI = 6.28318;
    const double ring1 = 350.0 / TA_AUDIO_SAMPLE_RATE;
    const double ring2 = 440.0 / TA_AUDIO_SAMPLE_RATE;
    double x1 = 0;
    double x2 = 0;

    for (unsigned i = 0; i < ARRAY_COUNT(buf); ++i)
    {
        buf[i] = (s16)(AMPLITUDE / 2 * (sin(x1 * M_2PI) + sin(x2 * M_2PI)));
        x1 += ring1;
        x2 += ring2;
    }
    alBufferData(buffer->al_buffer_id, AL_FORMAT_MONO16, buf, sizeof(buf), TA_AUDIO_SAMPLE_RATE);
#else
    // TODO: Allow caller to specify format (and maybe sample rate, but that's
    //       specified in the context attributes as well so I'm not sure if
    //       we can set a different value here).
    alBufferData(buffer->al_buffer_id, AL_FORMAT_MONO16, buffer->samples->data,
        buffer->samples->length - 1, TA_AUDIO_SAMPLE_RATE);
#endif
}
void ta_audio_buffer_free(ta_audio_buffer *buffer)
{
    ta_buffer_free(buffer->samples);
    alDeleteBuffers(1, &buffer->al_buffer_id);
}

void ta_audio_source_init(ta_audio_source *source)
{
    if (!source->pitch) {
        source->pitch = 1.0f;
    }
    if (!source->gain) {
        source->gain = 1.0f;
    }

    alGenSources(1, &source->al_source_id);
    alSourcef(source->al_source_id, AL_PITCH, source->pitch);
    alSourcef(source->al_source_id, AL_GAIN, source->gain);

    // TODO: Attach rico_audio to objects which auto-update this stuff
#if 0
    vec3 src_pos = VEC3(0.0f, 1.5f, 0.0f);
    alSourcefv(audio_source, AL_POSITION, (float *)&src_pos);
    alSourcefv(audio_source, AL_VELOCITY, (float *)&VEC3_ZERO);
#else
    alSourcei(source->al_source_id, AL_SOURCE_RELATIVE, AL_TRUE);
    //alSourcefv(source->al_source_id, AL_POSITION, (float *)&VEC3_ZERO);
    //alSourcefv(source->al_source_id, AL_VELOCITY, (float *)&VEC3_ZERO);
#endif

    if (source->audio_buffer_uid) {
        ta_audio_buffer *buffer = ta_scene_find_by_ref(&source->ref,
            F_TA_AUDIO_BUFFER, source->audio_buffer_uid);
        ta_audio_source_set_buffer(source, buffer);
    }
}
void ta_audio_source_free(ta_audio_source *source)
{
    alDeleteSources(1, &source->al_source_id);
}
void ta_audio_source_set_pitch(ta_audio_source *source, float pitch)
{
    source->pitch = pitch;
    alSourcef(source->al_source_id, AL_PITCH, source->pitch);
}
void ta_audio_source_set_gain(ta_audio_source *source, float gain)
{
    source->gain = gain;
    alSourcef(source->al_source_id, AL_GAIN, source->gain);
}
void ta_audio_source_set_buffer(ta_audio_source *source, ta_audio_buffer *buffer)
{
    if (source->state != TA_AUDIO_STOPPED) {
        ta_audio_source_stop(source);
    }
    source->audio_buffer_uid = buffer->ref.uid;
    //alSourceQueueBuffers(audio_source, 1, &audio_buffer);
    alSourcei(source->al_source_id, AL_BUFFER, buffer->al_buffer_id);
}
static ALenum al_source_state(ta_audio_source *source)
{
    DLB_ASSERT(source->al_source_id);
    ALenum state;
    alGetSourcei(source->al_source_id, AL_SOURCE_STATE, &state);
    return state;
}
#define AUDIO_ASSERT 0
void ta_audio_source_play(ta_audio_source *source)
{
    DLB_ASSERT(source->al_source_id);
    float gain;
    alGetSourcef(source->al_source_id, AL_GAIN, &gain);
    DLB_ASSERT(gain == source->gain);
    float pitch;
    alGetSourcef(source->al_source_id, AL_GAIN, &pitch);
    DLB_ASSERT(pitch == source->pitch);

    alSourcei(source->al_source_id, AL_LOOPING, AL_FALSE);
    alSourcePlay(source->al_source_id);
    ta_log_write(tg_debug_log,
        "[Audio] Playing source uid=%s id=%d pitch=%f gain=%f loop=%d\n",
        source->ref.uid, source->al_source_id, source->pitch, source->gain,
        false);
#if AUDIO_ASSERT
    DLB_ASSERT(al_source_state(source) == AL_PLAYING);
#endif
    source->state = TA_AUDIO_PLAYING;
}
void ta_audio_source_play_loop(ta_audio_source *source)
{
    DLB_ASSERT(source->al_source_id);
    float gain;
    alGetSourcef(source->al_source_id, AL_GAIN, &gain);
    DLB_ASSERT(gain == source->gain);
    float pitch;
    alGetSourcef(source->al_source_id, AL_GAIN, &pitch);
    DLB_ASSERT(pitch == source->pitch);

    alSourcei(source->al_source_id, AL_LOOPING, AL_TRUE);
    alSourcePlay(source->al_source_id);
    ta_log_write(tg_debug_log,
        "[Audio] Playing source uid=%s id=%d pitch=%f gain=%f loop=%d\n",
        source->ref.uid, source->al_source_id, source->pitch, source->gain,
        true);
#if AUDIO_ASSERT
    DLB_ASSERT(al_source_state(source) == AL_PLAYING);
#endif
    source->state = TA_AUDIO_PLAYING;
}
void ta_audio_source_pause(ta_audio_source *source)
{
    alSourcePause(source->al_source_id);
#if AUDIO_ASSERT
    DLB_ASSERT(al_source_state(source) == AL_PAUSED);
#endif
    source->state = TA_AUDIO_PAUSED;
}
void ta_audio_source_resume(ta_audio_source *source)
{
    alSourcePlay(source->al_source_id);
#if AUDIO_ASSERT
    DLB_ASSERT(al_source_state(source) == AL_PLAYING);
#endif
    source->state = TA_AUDIO_PLAYING;
}
void ta_audio_source_stop(ta_audio_source *source)
{
    alSourceStop(source->al_source_id);
#if AUDIO_ASSERT
    DLB_ASSERT(al_source_state(source) == AL_STOPPED);
#endif
    source->state = TA_AUDIO_STOPPED;
}