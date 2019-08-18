#pragma once
#include "ta_uid.h"
#include "ta_math.h"
#include "ta_collider.h"
#include "ta_node.h"
#include "dlb/dlb_types.h"

typedef struct ta_node {
    ta_uid uid;

	ta_transform transform;
    ta_transform transform_prev;
    ta_mat4 model;

    const char *material_uid;
    const char *mesh_group_uid;
    const char *rigid_body_uid;

	// Entity types
    const char *button_uid;

    ta_aabb aabb;  // AABB in local space

    bool invisible;
    bool cast_shadows;
    bool receive_shadows;  // TODO: Pass as flag to PBR shader, skip shadows if false
    struct ta_node *children;
} ta_node;

struct ta_shader;
struct ta_camera;

void ta_node_init(ta_node *node);
struct ta_material *ta_node_material(ta_node *node);
struct ta_mesh_group *ta_node_mesh_group(ta_node *node);
struct ta_rigid_body *ta_node_rigid_body(ta_node *node);
struct e_button *ta_node_button(ta_node *node);

void ta_node_update(ta_node *node);
void ta_node_shadow_pass(ta_node *node, struct ta_shader *shader,
    ta_mat4 *light_pv, float alpha);
void ta_node_render(ta_node *node, struct ta_camera *camera, float alpha);