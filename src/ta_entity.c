#include "ta_entity.h"
#include "ta_game.h"
#include "ta_scene.h"
#include "ta_symbol.h"

void ta_entity_free(ta_entity *entity)
{
    // Delete entity components
    for (u32 type = 0; type < RES_COMP_COUNT; ++type) {
        u32 component_id = entity->components[type];
        if (component_id) {
            const char *component_name = dlb_pool_by_id(
                &tg_game.scene->resource_names[type], component_id);
            dlb_hash_delete(&tg_game.scene->id_by_name[type], SYM(component_name));
            dlb_pool_delete(&tg_game.scene->resource_names[type], component_id);
            dlb_pool_delete(&tg_game.scene->resource_data[type], component_id);
        }
    }
}