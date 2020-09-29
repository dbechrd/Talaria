#include "ta_audio.h"
#include "ta_log.h"
#include "ta_file.h"
#include "ta_scene.h"
#include "ta_game.h"
#include "ta_math.h"
#include "dlb/dlb_memory.h"
#include "dlb/dlb_vector.h"
#pragma warning(push)
#pragma warning(disable: 4201)  // nameless struct/union
#include "fmod/core/fmod.h"
#include "fmod/core/fmod_errors.h"
#include "fmod_strings.h"
#pragma warning(pop)
#include "AL/al.h"
#include "AL/alc.h"
#include "AL/alext.h"
#include <string.h>
#include <math.h>
#include <combaseapi.h>

#define AUDIO_ASSERT 0  // change to 1 to assert that states are valid
#define TA_AUDIO_SAMPLE_RATE 44100
#define TA_AUDIO_SAMPLES_PER_MS (TA_AUDIO_SAMPLE_RATE / 1000.0)

ALCdevice *audio_openal_device;
ALCcontext *audio_openal_context;

struct {
    FMOD_SYSTEM *fmod_system;
    FMOD_CHANNEL *fmod_channel;
    FMOD_SOUND **fmod_sounds;
} audio_state;

ta_audio_listener tg_audio_listener;

void audio_fmod_check(FMOD_RESULT result, const char *file, int line)
{
    if (result != FMOD_OK) {
        ta_log_write(&tg_debug_log, SRC_AUDIO, "%s(%d): FMOD Error %d: %s\n", file, line, result,
            FMOD_ErrorString(result));
    }
}
#define TA_FMOD(result) audio_fmod_check(result, __FILE__, __LINE__)

FMOD_RESULT F_CALLBACK audio_fmod_callback(FMOD_SYSTEM *system, FMOD_SYSTEM_CALLBACK_TYPE type, void *commanddata1,
    void *commanddata2, void *userdata)
{
    UNUSED(system);
    UNUSED(commanddata2);
    UNUSED(userdata);

    FMOD_RESULT result = FMOD_OK;
    if (type == FMOD_SYSTEM_CALLBACK_ERROR) {
        FMOD_ERRORCALLBACK_INFO *info = commanddata1;
        // Ignore channels that have stopped playing or were stolen
        switch (info->instancetype) {
            case FMOD_ERRORCALLBACK_INSTANCETYPE_CHANNELCONTROL:
                if (info->result == FMOD_ERR_INVALID_HANDLE ||
                    info->result == FMOD_ERR_CHANNEL_STOLEN) {
                    return result;
                }
            case FMOD_ERRORCALLBACK_INSTANCETYPE_CHANNEL:
                if (info->result == FMOD_ERR_INVALID_HANDLE ||
                    info->result == FMOD_ERR_CHANNEL_STOLEN) {
                    return result;
                }
            default:
                break;
        }
        ta_log_write(&tg_debug_log, SRC_AUDIO, "FMOD Error:\n");
        ta_log_write(&tg_debug_log, SRC_AUDIO, "  %d(%s)\n", info->result, FMOD_ErrorString(info->result));
        ta_log_write(&tg_debug_log, SRC_AUDIO, "  %s(%p)\n", FMOD_ErrorCallback_InstanceTypeString(info->instancetype), info->instance);
        ta_log_write(&tg_debug_log, SRC_AUDIO, "  %s(%s)\n", info->functionname, info->functionparams);
    }
    return result;
}

void ta_audio_init()
{
#if 0
    HRESULT com_result = CoInitializeEx(0, COINIT_APARTMENTTHREADED);
    if (com_result != S_OK) {
        ta_log_write(&tg_debug_log, SRC_AUDIO, "CoInitializeEx returned unexpected code %d\n", com_result);
    }
    ta_log_write(&tg_debug_log, SRC_AUDIO, "FMOD_System_Create...\n");
    TA_FMOD(FMOD_System_Create(&audio_state.fmod_system));
    ta_log_write(&tg_debug_log, SRC_AUDIO, "FMOD_System_Create done.\n");
    TA_FMOD(FMOD_System_SetCallback(audio_state.fmod_system, audio_fmod_callback, FMOD_SYSTEM_CALLBACK_ERROR));

    unsigned int version;
    FMOD_System_GetVersion(audio_state.fmod_system, &version);
    if (version < FMOD_VERSION) {
        ta_log_write(&tg_debug_log, SRC_AUDIO, "FMOD lib version %08x doesn't match header version %08x\n", version,
            FMOD_VERSION);
    }
    ta_log_write(&tg_debug_log, SRC_AUDIO, "FMOD_System_Init...\n");
    FMOD_System_Init(audio_state.fmod_system, 32, FMOD_INIT_NORMAL, 0);
    ta_log_write(&tg_debug_log, SRC_AUDIO, "FMOD_System_Init done.\n");

    ta_log_write(&tg_debug_log, SRC_AUDIO, "FMOD creating sounds...\n");
    FMOD_SOUND **sound1 = dlb_vec_alloc(audio_state.fmod_sounds);
    FMOD_SOUND **sound2 = dlb_vec_alloc(audio_state.fmod_sounds);
    FMOD_SOUND **sound3 = dlb_vec_alloc(audio_state.fmod_sounds);
    // drumloop.wav has embedded loop points which automatically makes looping turn on, use FMOD_LOOP_OFF to disable
    FMOD_System_CreateSound(audio_state.fmod_system, "data/sfx/drumloop.wav", FMOD_DEFAULT, 0, sound1);
    FMOD_Sound_SetMode(*sound1, FMOD_LOOP_OFF);
    FMOD_System_CreateSound(audio_state.fmod_system, "data/sfx/jaguar.wav", FMOD_DEFAULT, 0, sound2);
    FMOD_System_CreateSound(audio_state.fmod_system, "data/sfx/swish.wav", FMOD_DEFAULT, 0, sound3);
#endif

#if 0
    ta_log_write(&tg_debug_log, SRC_AUDIO, "FMOD playing sounds...\n");
    FMOD_System_PlaySound(audio_state.fmod_system, *sound1, 0, false, &audio_state.fmod_channel);
    FMOD_System_PlaySound(audio_state.fmod_system, *sound2, 0, false, &audio_state.fmod_channel);
    FMOD_System_PlaySound(audio_state.fmod_system, *sound3, 0, false, &audio_state.fmod_channel);
#endif

    //ta_log_write(&tg_debug_log, SRC_AUDIO, "Enumerating devices...\n");
    const char *desired_device = 0;
    //const char *devices = alcGetString(NULL, ALC_ALL_DEVICES_SPECIFIER);
    //const char *s = devices;
    //while (*s)
    //{
    //    ta_log_write(&tg_debug_log, SRC_AUDIO, "  Device: %s\n", s);
    //    if (!strcmp(s, "OpenAL Soft on Speakers (High Definition Audio Device)")) {
    //        desired_device = s;
    //    }
    //    while (*s) { s++; }
    //    s++;
    //}

    // TODO: Allow user to set which device they want to use
    ta_log_write(&tg_debug_log, SRC_AUDIO, "alcOpenDevice...\n");
    audio_openal_device = alcOpenDevice(desired_device);
    if (!audio_openal_device) {
        DLB_ASSERT(!"Failed to open listener context");
        return;
    }

    // TODO: What other attributes can I specify on a context?
    ta_log_write(&tg_debug_log, SRC_AUDIO, "alcCreateContext...\n");
    ALCint attrlist[] = {
        ALC_FREQUENCY, TA_AUDIO_SAMPLE_RATE,
        ALC_HRTF_SOFT, ALC_TRUE,
        0
    };
    audio_openal_context = alcCreateContext(audio_openal_device, attrlist);
    if (!audio_openal_context) {
        DLB_ASSERT(!"Failed to create listener context");
        return;
    }

    // TODO: Can I have more than one context, is that useful?
    ta_log_write(&tg_debug_log, SRC_AUDIO, "alcMakeContextCurrent...\n");
    if (!alcMakeContextCurrent(audio_openal_context)) {
        DLB_ASSERT(!"Failed to activate listener context");
        return;
    }

    ta_log_write(&tg_debug_log, SRC_AUDIO, "Checking for OpenAL errors...\n");
    ALenum err = alGetError();
    if (err) {
        DLB_ASSERT(!"OpenAL error");
        //DLB_ASSERT(!"OpenAL error: %s\n", err);
    }

    // ALC_HRTF_DISABLED_SOFT                   0x0000
    // ALC_HRTF_ENABLED_SOFT                    0x0001
    // ALC_HRTF_DENIED_SOFT                     0x0002
    // ALC_HRTF_REQUIRED_SOFT                   0x0003
    // ALC_HRTF_HEADPHONES_DETECTED_SOFT        0x0004
    // ALC_HRTF_UNSUPPORTED_FORMAT_SOFT         0x0005
    int hrtf_value = 0;
    alcGetIntegerv(audio_openal_device, ALC_HRTF_STATUS_SOFT, 1, &hrtf_value);
}
void ta_audio_update()
{
#if 0
    ta_log_write(&tg_debug_log, SRC_AUDIO, "FMOD_System_Update...\n");
    FMOD_System_Update(audio_state.fmod_system);
    ta_log_write(&tg_debug_log, SRC_AUDIO, "FMOD_System_Update done.\n");

    if (audio_state.fmod_channel) {
        bool playing = 0;
        bool paused = 0;
        unsigned int ms = 0;
        FMOD_SOUND *currentsound = 0;
        unsigned int lenms = 0;

        FMOD_Channel_IsPlaying(audio_state.fmod_channel, &playing);
        FMOD_Channel_GetPaused(audio_state.fmod_channel, &paused);
        FMOD_Channel_GetPosition(audio_state.fmod_channel, &ms, FMOD_TIMEUNIT_MS);
        FMOD_Channel_GetCurrentSound(audio_state.fmod_channel, &currentsound);
        if (currentsound) {
            FMOD_Sound_GetLength(currentsound, &lenms, FMOD_TIMEUNIT_MS);
        }
        ta_log_write(&tg_debug_log, SRC_AUDIO, "Time %02d:%02d:%02d/%02d:%02d:%02d : %s\n",
            ms / 1000 / 60,
            ms / 1000 % 60,
            ms / 10 % 100,
            lenms / 1000 / 60,
            lenms / 1000 % 60,
            lenms / 10 % 100,
            paused ? "Paused " : playing ? "Playing" : "Stopped"
        );
    }

    int channelsplaying = 0;
    FMOD_System_GetChannelsPlaying(audio_state.fmod_system, &channelsplaying, 0);
    ta_log_write(&tg_debug_log, SRC_AUDIO, "FMOD channels playing: %d\n", channelsplaying);
#endif
}
void ta_audio_free()
{
#if 0
    ta_log_write(&tg_debug_log, SRC_AUDIO, "FMOD cleanup...\n");
    dlb_vec_each(FMOD_SOUND **, sound, audio_state.fmod_sounds) {
        FMOD_Sound_Release(*sound);
    }
    FMOD_System_Close(audio_state.fmod_system);
    FMOD_System_Release(audio_state.fmod_system);
    CoUninitialize();
    ta_log_write(&tg_debug_log, SRC_AUDIO, "FMOD cleanup done.\n");
#endif
}

void ta_audio_listener_init(ta_audio_listener *listener)
{
    if (!listener->volume) {
        listener->volume = 1.0f;
    }
    ta_audio_listener_set_volume(listener, listener->volume);
}
float ta_audio_listener_get_volume(ta_audio_listener *listener)
{
    return listener->volume;
}
void ta_audio_listener_set_volume(ta_audio_listener *listener, float volume)
{
    listener->volume = volume;
    if (!listener->mute) {
        alListenerf(AL_GAIN, listener->volume);
    }
}
bool ta_audio_listener_muted(ta_audio_listener *listener)
{
    return listener->mute;
}
void ta_audio_listener_mute(ta_audio_listener *listener)
{
    listener->mute = true;
    alListenerf(AL_GAIN, 0.0f);
}
void ta_audio_listener_unmute(ta_audio_listener *listener)
{
    listener->mute = false;
    alListenerf(AL_GAIN, listener->volume);
}
void ta_audio_listener_toggle(ta_audio_listener *listener)
{
    if (listener->mute) {
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
    } else if (buffer->samples) {
        ta_audio_buffer_load(buffer);
    }
}
void ta_audio_buffer_init_void(void *buffer)
{
    ta_audio_buffer_init(buffer);
}
void ta_audio_buffer_load_path(ta_audio_buffer *buffer, const char *path)
{
    buffer->path = path;

    // Load sample data from file
    char *samples = ta_file_read_all(buffer->path);
    // NOTE: Ignore nil character that ta_file_read_all() appends
    size_t samples_len = dlb_vec_len(samples) - 1;
    // Ensure samples_len can fit in an ALsizei (int)
    DLB_ASSERT(samples_len <= INT_MAX);
    ALsizei samples_bytes = (ALsizei)samples_len;
    ta_audio_buffer_set_samples(buffer, samples, samples_bytes);
    ta_audio_buffer_load(buffer);
}
void ta_audio_buffer_set_samples(ta_audio_buffer *buffer, char *samples, ALsizei samples_bytes)
{
    buffer->samples = samples;
    buffer->samples_bytes = samples_bytes;
}
void ta_audio_buffer_load(ta_audio_buffer *buffer)
{
    DLB_ASSERT(buffer->samples);
    DLB_ASSERT(buffer->samples_bytes);

    ALenum err = AL_NO_ERROR;

    alGenBuffers(1, &buffer->al_buffer_id);
    err = alGetError();
    DLB_ASSERT(err == AL_NO_ERROR);
#if 0
    const u32 AMPLITUDE = 2000;
    s16 buf[TA_AUDIO_SAMPLE_RATE];

    //const double TWO_PI = 6.28318;
    const double ring1 = 350.0 / TA_AUDIO_SAMPLE_RATE;
    const double ring2 = 440.0 / TA_AUDIO_SAMPLE_RATE;
    double x1 = 0;
    double x2 = 0;

    for (unsigned i = 0; i < ARRAY_SIZE(buf); ++i) {
        buf[i] = (s16)(AMPLITUDE / 2 * (sin(x1 * 6.28) + sin(x2 * 6.28)));
        x1 += ring1;
        x2 += ring2;
    }
    alBufferData(buffer->al_buffer_id, AL_FORMAT_MONO16, buf, sizeof(buf),
        TA_AUDIO_SAMPLE_RATE);
#else
    // TODO: Allow caller to specify format (and maybe sample rate, but that's
    //       specified in the context attributes as well so I'm not sure if
    //       we can set a different value here).
    alBufferData(buffer->al_buffer_id, AL_FORMAT_MONO16, buffer->samples, buffer->samples_bytes, TA_AUDIO_SAMPLE_RATE);
    err = alGetError();
    DLB_ASSERT(err == AL_NO_ERROR);
#endif
}
double ta_audio_buffer_duration_ms(ta_audio_buffer *buffer)
{
    // TODO: This is wrong for formats where sample size > 8-bit (e.g. AL_FORMAT_MONO16)
    double duration_ms = dlb_vec_len(buffer->samples) / TA_AUDIO_SAMPLES_PER_MS;
    return duration_ms;
}
void ta_audio_buffer_free(ta_audio_buffer *buffer)
{
    dlb_vec_free(buffer->samples);
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

    // TODO: Add "bool relative" to audio_source and update this based on
    // transform component's current position each frame when relative = true
#if 0
    vec3 src_pos = VEC3(0.0f, 1.5f, 0.0f);
    alSourcefv(source->al_source_id, AL_POSITION, (float *)&src_pos);
    alSourcefv(source->al_source_id, AL_VELOCITY, (float *)&VEC3_ZERO);
#else
    alSourcei(source->al_source_id, AL_SOURCE_RELATIVE, AL_FALSE);
    alSourcefv(source->al_source_id, AL_POSITION, (float *)&VEC3_ZERO);
    alSourcefv(source->al_source_id, AL_VELOCITY, (float *)&VEC3_ZERO);
#endif

    if (source->audio_buffer) {
        ta_audio_buffer *buffer = ta_game_by_sym(RES_AUDIO_BUFFER,
            source->audio_buffer);
        DLB_ASSERT(buffer->al_buffer_id);
        alSourcei(source->al_source_id, AL_BUFFER, buffer->al_buffer_id);
    }
}
void ta_audio_source_init_void(void *source)
{
    ta_audio_source_init(source);
}
void ta_audio_source_free(ta_audio_source *source)
{
    alDeleteSources(1, &source->al_source_id);
}
void ta_audio_source_free_void(void *source)
{
    ta_audio_source_free(source);
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
void ta_audio_source_set_position(ta_audio_source *source, ta_vec3 position)
{
    alSourcefv(source->al_source_id, AL_POSITION, (float *)&position);
}
void ta_audio_source_set_velocity(ta_audio_source *source, ta_vec3 velocity)
{
    alSourcefv(source->al_source_id, AL_VELOCITY, (float *)&velocity);
}
ta_result ta_audio_source_set_buffer(ta_audio_source *source, const char *audio_buffer)
{
    ta_audio_source_state state = 0;
    ta_audio_source_get_state(source, &state);
    if (state != TA_AUDIO_STOPPED) {
        ta_audio_source_stop(source);
    }
    if (source->audio_buffer != audio_buffer) {
        source->audio_buffer = audio_buffer;
        ta_audio_buffer *buffer = ta_game_by_sym_try(RES_AUDIO_BUFFER, source->audio_buffer);
        if (buffer) {
            //alSourceQueueBuffers(audio_source, 1, &audio_buffer);
            alSourcei(source->al_source_id, AL_BUFFER, buffer->al_buffer_id);
        } else {
            ta_log_write(&tg_debug_log, SRC_AUDIO, "Audio buffer '%s' missing.\n", audio_buffer);
            return TA_MISSING_RESOURCE;
        }
    }
    return TA_OK;
}
static ALenum al_source_state(ta_audio_source *source)
{
    DLB_ASSERT(source->al_source_id);
    ALenum state;
    alGetSourcei(source->al_source_id, AL_SOURCE_STATE, &state);
    return state;
}
void ta_audio_source_get_state(ta_audio_source *source, ta_audio_source_state *state)
{
    *state = (ta_audio_source_state)al_source_state(source);
}
void ta_audio_source_play(ta_audio_source *source)
{
    DLB_ASSERT(source->al_source_id);
    float gain;
    alGetSourcef(source->al_source_id, AL_GAIN, &gain);
    DLB_ASSERT(gain == source->gain);
    float pitch;
    alGetSourcef(source->al_source_id, AL_PITCH, &pitch);
    DLB_ASSERT(pitch == source->pitch);

    alSourcei(source->al_source_id, AL_LOOPING, AL_FALSE);
    alSourcePlay(source->al_source_id);
    ta_log_write(&tg_debug_log, SRC_AUDIO,
        "Playing source name=%s id=%d pitch=%f gain=%f loop=%d\n",
        source->name, source->al_source_id, source->pitch, source->gain,
        false);
#if AUDIO_ASSERT
    DLB_ASSERT(al_source_state(source) == AL_PLAYING);
#endif
}
void ta_audio_source_play_name(ta_audio_source *source, const char *audio_buffer)
{
    ta_audio_source_set_buffer(source, audio_buffer);
    ta_audio_source_play(source);
}
void ta_audio_source_play_loop(ta_audio_source *source)
{
    DLB_ASSERT(source->al_source_id);
    float gain;
    alGetSourcef(source->al_source_id, AL_GAIN, &gain);
    DLB_ASSERT(gain == source->gain);
    float pitch;
    alGetSourcef(source->al_source_id, AL_PITCH, &pitch);
    DLB_ASSERT(pitch == source->pitch);

    alSourcei(source->al_source_id, AL_LOOPING, AL_TRUE);
    alSourcePlay(source->al_source_id);
    ta_log_write(&tg_debug_log, SRC_AUDIO,
        "Playing source name=%s id=%d pitch=%f gain=%f loop=%d\n",
        source->name, source->al_source_id, source->pitch, source->gain,
        true);
#if AUDIO_ASSERT
    DLB_ASSERT(al_source_state(source) == AL_PLAYING);
#endif
}
void ta_audio_source_play_loop_name(ta_audio_source *source, const char *audio_buffer)
{
    ta_audio_source_set_buffer(source, audio_buffer);
    ta_audio_source_play_loop(source);
}
void ta_audio_source_pause(ta_audio_source *source)
{
    alSourcePause(source->al_source_id);
#if AUDIO_ASSERT
    DLB_ASSERT(al_source_state(source) == AL_PAUSED);
#endif
}
void ta_audio_source_resume(ta_audio_source *source)
{
    alSourcePlay(source->al_source_id);
#if AUDIO_ASSERT
    DLB_ASSERT(al_source_state(source) == AL_PLAYING);
#endif
}
void ta_audio_source_stop(ta_audio_source *source)
{
    alSourceStop(source->al_source_id);
#if AUDIO_ASSERT
    DLB_ASSERT(al_source_state(source) == AL_STOPPED);
#endif
}