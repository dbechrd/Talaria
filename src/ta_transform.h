#pragma once
#include "ta_schema.h"
#include "ta_math.h"

typedef struct ta_transform {
    TA_COMPONENT_HEADER
    ta_xform    xform;      // current transform
    //ta_xform    xform_prev; // xform last frame (for interpolation)
    ta_xform    xform_world;// xform in world space (all parents included)
    ta_mat4     local;      // cached model matrix (local space, relative to parent)
    ta_mat4     world;      // cached model matrix (world space, all parents included)
    const char  *parent;    // parent node
    const char  **children; // array of children nodes
    bool dirty_flag;        // double-buffered dirty flag (flip-flops between true and false meaning dirty)
} ta_transform;

extern bool ta_transform_dirty_flag;

void ta_transform_init          (ta_transform *transform);
void ta_transform_init_void     (void *transform);
void ta_transform_free          (ta_transform *transform);
void ta_transform_free_void     (void *transform);
void ta_transform_update        (ta_transform *transform, float alpha, bool dirty_flag);
void ta_transform_update_all    (ta_transform *transforms, float alpha);