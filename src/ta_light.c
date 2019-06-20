#include "ta_light.h"

const char *ta_light_type_str(int type)
{
    switch(type) {
        case TA_LIGHT_AMBIENT:     return "TA_LIGHT_AMBIENT";
        case TA_LIGHT_DIRECTIONAL: return "TA_LIGHT_DIRECTIONAL";
        case TA_LIGHT_POINT:       return "TA_LIGHT_POINT";
        case TA_LIGHT_SPOT:        return "TA_LIGHT_SPOT";
        default:
            DLB_ASSERT(!"<UNKNOWN_TA_LIGHT_TYPE>");
            return 0;
    }
}

void ta_light_init(ta_light *light)
{
    if (!light->intensity) {
        light->intensity = 1.0f;
    }
    switch (light->type) {
        case TA_LIGHT_AMBIENT:
            break;
        case TA_LIGHT_DIRECTIONAL:
            light->data.directional.direction =
                vec3_normalize(light->data.directional.direction);
            break;
        case TA_LIGHT_POINT:
            break;
        case TA_LIGHT_SPOT:
            light->data.directional.direction =
                vec3_normalize(light->data.directional.direction);
            break;
    }
}