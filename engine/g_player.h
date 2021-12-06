#ifndef G_PLAYER_H
#define G_PLAYER_H

#include <stdint.h>
#include "g_defs.h"

void g_PlayerInit();

void g_StepPlayer(float delta_time);

struct g_spawn_point_t *g_CreateSpawnPoint(vec3_t *position);

void g_DestroySpawnPoint(struct g_spawn_point_t *spawn_point);

void g_SpawnPlayer(struct g_spawn_point_t *spawn_point);


#endif // G_PLAYER_H
