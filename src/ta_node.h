#pragma once
#include "ta_uid.h"
#include "ta_math.h"
#include "dlb/dlb_types.h"

struct ta_entity;
struct ta_camera;
struct ta_shader;

// TODO: Move all this to ta_entity.h or get rid of it entirely (via "systems")
void ta_node_shadow_pass(struct ta_entity *entity, struct ta_shader *shader,
    ta_mat4 *light_pv, float alpha);
void ta_node_render(struct ta_entity *entity, struct ta_camera *camera, float alpha);
void ta_node_render_shader(struct ta_entity *entity, struct ta_camera *camera,
    struct ta_shader *shader, float alpha, float scale);