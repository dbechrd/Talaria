#include "ta_collide.h"

const char *ta_collider_type_str(int type) {
    switch(type) {
        case TA_COLLIDER_AABB: return "TA_COLLIDER_AABB";
        case TA_COLLIDER_OBB:  return "TA_COLLIDER_OBB";
        default:
            DLB_ASSERT(!"<UNKNOWN_TA_COLLIDER_TYPE>");
            return 0;
    }
};