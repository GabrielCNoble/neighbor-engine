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

void g_PlayerInit();

void g_StepPlayer(float delta_time);

struct g_spawn_point_t *g_CreateSpawnPoint(vec3_t *position);

void g_DestroySpawnPoint(struct g_spawn_point_t *spawn_point);

void g_SpawnPlayer(struct g_spawn_point_t *spawn_point);


#endif // G_PLAYER_H
