#pragma once
#include "dlb/dlb_types.h"

typedef struct ta_string {
    const char *data;
    u32 length;
    u32 hash;
} ta_string;