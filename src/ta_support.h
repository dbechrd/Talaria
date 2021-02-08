#pragma once

#include "ta_math.h"
#include "dlb/dlb_types.h"

ta_vec3 ta_support_obb(ta_obb *obb, ta_vec3 d);
bool ta_gjk_intersect_obb(ta_obb *a, ta_obb *b, int *gjk_step, int *gjk_steps);
