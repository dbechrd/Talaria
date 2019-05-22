#include "ta_light.h"

const char *ta_light_type_str(int type)
{
    switch(type) {
        case TA_LIGHT_SUN:   return "TA_LIGHT_SUN";
        case TA_LIGHT_POINT: return "TA_LIGHT_POINT";
        default:
            DLB_ASSERT(!"<UNKNOWN_TA_LIGHT_TYPE>");
            return 0;
    }
}

void ta_light_init(ta_light *light)
{
    switch (light->type) {
        case TA_LIGHT_SUN:
            light->data.sun.direction = vec3_normalize(light->data.sun.direction);
            break;
        case TA_LIGHT_POINT:
            break;
    }
}