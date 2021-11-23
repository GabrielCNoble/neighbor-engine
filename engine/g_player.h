#ifndef G_PLAYER_H
#define G_PLAYER_H

#include <stdint.h>
#include "e_defs.h"
#include "p_defs.h"

struct g_player_state_t
{
    uint32_t flags;
    float yaw;
    float pitch;
    struct e_entity_t *entity;
    struct p_character_collider_t *collider;
};

void g_InitPlayer();

void g_UpdatePlayer();


#endif // G_PLAYER_H
