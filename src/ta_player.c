#include "ta_player.h"
#include "dlb/dlb_vector.h"

void ta_player_free(ta_player *player)
{
    dlb_vec_free(player->e_guns);
}
void ta_player_free_void(void *player)
{
    ta_player_free(player);
}