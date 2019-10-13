#pragma once
#include "ta_uid.h"
#include "ta_math.h"
#include "dlb/dlb_types.h"

struct ta_camera;
struct ta_shader;

void ta_node_init(ta_entity *node);
void ta_node_update(ta_entity *node);
void ta_node_shadow_pass(ta_entity *node, struct ta_shader *shader,
    ta_mat4 *light_pv, float alpha);
void ta_node_render(ta_entity *node, struct ta_camera *camera, float alpha);
void ta_node_render_shader(ta_entity *node, struct ta_camera *camera,
    struct ta_shader *shader, float alpha, float scale);