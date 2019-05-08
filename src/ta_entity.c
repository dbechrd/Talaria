#include "ta_entity.h"
#include "ta_schema.h"
#include "ta_scene.h"
#include "ta_symbol.h"
#include "dlb_hash.h"
#include <stdio.h>

ta_material *entity_material(ta_entity *e)
{
    ta_material *mat = ta_scene_find(e->scene, F_TA_MATERIAL, e->material);
    return mat;
}