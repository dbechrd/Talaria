#pragma once
#include "ta_scene.h"
#include "ta_math.h"
#include "ta_material.h"
#include "ta_mesh_group.h"
#include "ta_rigid_body.h"
#include "ta_button.h"
#include "ta_camera.h"

typedef struct ta_node_s ta_node;
typedef struct ta_node_s {
	ta_scene_ref ref;

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
    ta_node *children;
} ta_node;

void ta_node_init(ta_node *node);
ta_material *ta_node_material(ta_node *node);
ta_mesh_group *ta_node_mesh_group(ta_node *node);
ta_rigid_body *ta_node_rigid_body(ta_node *node);
ta_button *ta_node_button(ta_node *node);

void ta_node_update(ta_node *node);
void ta_node_shadow_pass(ta_node *node, ta_shader *shader, ta_mat4 *light_pv,
    float alpha);
void ta_node_render(ta_node *node, ta_camera *camera, float alpha);