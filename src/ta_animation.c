#include "ta_animation.h"
#include "dlb/dlb_vector.h"

const char *ta_animation_track_key_kind_str(ta_animation_track_key_kind kind)
{
    switch (kind) {
        case TA_ANIMATION_TRACK_KEY_UNKNOWN    : return "TA_ANIMATION_TRACK_KEY_UNKNOWN"    ;
        case TA_ANIMATION_TRACK_KEY_VALUE      : return "TA_ANIMATION_TRACK_KEY_VALUE"      ;
        case TA_ANIMATION_TRACK_KEY_POS_CONTROL: return "TA_ANIMATION_TRACK_KEY_POS_CONTROL";
        case TA_ANIMATION_TRACK_KEY_NEG_CONTROL: return "TA_ANIMATION_TRACK_KEY_NEG_CONTROL";
        default: DLB_ASSERT(!"Invalid enum value"); return "<TA_ANIMATION_TRACK_KEY_UNKNOWN>";
    }
};

const char *ta_animation_track_curve_type_str(ta_animation_track_curve_type type)
{
    switch (type) {
        case TA_ANIMATION_TRACK_CURVE_UNKNOWN: return "TA_ANIMATION_TRACK_CURVE_UNKNOWN";
        case TA_ANIMATION_TRACK_CURVE_LINEAR : return "TA_ANIMATION_TRACK_CURVE_LINEAR" ;
        case TA_ANIMATION_TRACK_CURVE_BEZIER : return "TA_ANIMATION_TRACK_CURVE_BEZIER" ;
        default: DLB_ASSERT(!"Invalid enum value"); return "<TA_ANIMATION_TRACK_CURVE_UNKNOWN>";
    }
};

#if 0  // GLTF Animation data
const char *ta_animation_path_type_str(int type)
{
    switch (type) {
        case TA_ANIMATION_PATH_TRANSLATION: return "TA_ANIMATION_PATH_TRANSLATION";
        case TA_ANIMATION_PATH_ROTATION:    return "TA_ANIMATION_PATH_ROTATION";
        case TA_ANIMATION_PATH_SCALE:       return "TA_ANIMATION_PATH_SCALE";
        case TA_ANIMATION_PATH_WEIGHTS:     return "TA_ANIMATION_PATH_WEIGHTS";
        default: DLB_ASSERT(0);             return "TA_ANIMATION_PATH_???";
    }
}

static void ta_animation_sampler_free(ta_animation_sampler *sampler)
{
    dlb_vec_free(sampler->input);
#if 0
    if (sampler->target_path == TA_ANIMATION_PATH_WEIGHTS) {
        dlb_vec_free(sampler->output.weights);
    }
#else
    dlb_vec_free(sampler->output);
#endif
}

static void ta_animation_channel_free(ta_animation_channel *channel)
{
    // TODO(cleanup): Nothing to free here
    UNUSED(channel);
}

void ta_animation_free(ta_animation *animation)
{
    dlb_vec_each(ta_animation_sampler *, sampler, animation->samplers) {
        ta_animation_sampler_free(sampler);
    }
    dlb_vec_free(animation->samplers);
    dlb_vec_each(ta_animation_channel *, channel, animation->channels) {
        ta_animation_channel_free(channel);
    }
    dlb_vec_free(animation->channels);
}
void ta_animation_free_void(void *animation)
{
    ta_animation_free(animation);
}
#endif