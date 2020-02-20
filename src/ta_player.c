#include "ta_player.h"

void ta_player_free(ta_player *player)
{
    dlb_vec_free(player->e_guns);
}