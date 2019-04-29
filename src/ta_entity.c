#include "ta_entity.h"
#include "ta_schema.h"
#include "ta_scene.h"
#include "dlb_hash.h"
#include <stdio.h>

ta_material *entity_material(ta_entity *e)
{
    scene_ref *ref = dlb_hash_search(&e->scene->refs_by_name, CSTR("material_name"));
    DLB_ASSERT(ref);
    DLB_ASSERT(ref->type == F_TA_MATERIAL);
    ta_schema_print(stdout, ref->type, ref->ptr, 0);
    return ref->ptr;
}