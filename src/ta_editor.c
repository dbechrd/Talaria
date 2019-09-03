#include "ta_editor.h"
#include "ta_game.h"
#include "ta_scene.h"
#include "ta_node.h"

typedef struct ta_editor {
    const char *selected_node_uid;
    ta_text_entry *text_entry;
} ta_editor;

static ta_editor editor;
ta_editor *tg_editor = &editor;

void ta_editor_set_active_text_entry(ta_text_entry *text_entry)
{
    tg_editor->text_entry = text_entry;
    if (tg_editor->text_entry) {
        ta_game_state_set(&tg_game, TA_GAME_STATE_TEXT_ENTRY);
    } else {
        ta_game_state_set(&tg_game, tg_game.state_prev);
    }
}

ta_text_entry *ta_editor_active_text_entry()
{
    return tg_editor->text_entry;
}

void ta_editor_select_node(ta_node *node)
{
    tg_editor->selected_node_uid = node->uid.uid;
}

ta_node *ta_editor_selected_node()
{
    ta_node *node = 0;
    if (tg_editor->selected_node_uid) {
#if 1
        node = ta_scene_exists(tg_game.scene, TYP_NODE, tg_editor->selected_node_uid, 0);
#else
        // TODO: Store selected_node_idx. If generation doesn't match,
        //       ta_scene_find() should return zero.
        node = ta_scene_find(tg_game.scene, TYP_NODE, tg_editor->selected_node_idx);
        if (!node) {
            // Node has been deleted
            tg_editor->selected_node_idx = 0;
        }
#endif
    }
    return node;
}