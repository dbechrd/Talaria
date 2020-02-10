#ifndef _TA_FMOD_STRING_H
#define _TA_FMOD_STRING_H

#include "fmod/core/fmod.h"

#ifdef __GNUC__
static const char *FMOD_ErrorString(FMOD_RESULT errcode) __attribute__((unused));
#endif

static const char *FMOD_ErrorCallback_InstanceTypeString(FMOD_ERRORCALLBACK_INSTANCETYPE instancetype)
{
    switch (instancetype)
    {
        case FMOD_ERRORCALLBACK_INSTANCETYPE_NONE:                     return "NONE";
        case FMOD_ERRORCALLBACK_INSTANCETYPE_SYSTEM:                   return "SYSTEM";
        case FMOD_ERRORCALLBACK_INSTANCETYPE_CHANNEL:                  return "CHANNEL";
        case FMOD_ERRORCALLBACK_INSTANCETYPE_CHANNELGROUP:             return "CHANNELGROUP";
        case FMOD_ERRORCALLBACK_INSTANCETYPE_CHANNELCONTROL:           return "CHANNELCONTROL";
        case FMOD_ERRORCALLBACK_INSTANCETYPE_SOUND:                    return "SOUND";
        case FMOD_ERRORCALLBACK_INSTANCETYPE_SOUNDGROUP:               return "SOUNDGROUP";
        case FMOD_ERRORCALLBACK_INSTANCETYPE_DSP:                      return "DSP";
        case FMOD_ERRORCALLBACK_INSTANCETYPE_DSPCONNECTION:            return "DSPCONNECTION";
        case FMOD_ERRORCALLBACK_INSTANCETYPE_GEOMETRY:                 return "GEOMETRY";
        case FMOD_ERRORCALLBACK_INSTANCETYPE_REVERB3D:                 return "REVERB3D";
        case FMOD_ERRORCALLBACK_INSTANCETYPE_STUDIO_SYSTEM:            return "STUDIO_SYSTEM";
        case FMOD_ERRORCALLBACK_INSTANCETYPE_STUDIO_EVENTDESCRIPTION:  return "STUDIO_EVENTDESCRIPTION:";
        case FMOD_ERRORCALLBACK_INSTANCETYPE_STUDIO_EVENTINSTANCE:     return "STUDIO_EVENTINSTANCE";
        case FMOD_ERRORCALLBACK_INSTANCETYPE_STUDIO_PARAMETERINSTANCE: return "STUDIO_PARAMETERINSTANCE";
        case FMOD_ERRORCALLBACK_INSTANCETYPE_STUDIO_BUS:               return "STUDIO_BUS";
        case FMOD_ERRORCALLBACK_INSTANCETYPE_STUDIO_VCA:               return "STUDIO_VCA";
        case FMOD_ERRORCALLBACK_INSTANCETYPE_STUDIO_BANK:              return "STUDIO_BANK";
        case FMOD_ERRORCALLBACK_INSTANCETYPE_STUDIO_COMMANDREPLAY:     return "STUDIO_COMMANDREPLAY";
        default :                                                      return "UNKNOWN";
    };
}

#endif
