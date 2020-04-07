#pragma once
#include "ta_schema.h"

typedef struct ta_skin {
    TA_RESOURCE_HEADER
    const char **joints;
    const char *skeleton;
    void *inverse_bind_matrices; // ???
} ta_skin;
