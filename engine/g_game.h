#ifndef G_GAME_H
#define G_GAME_H

#include "g_defs.h"
#include "../lib/dstuff/ds_slist.h"

void g_GameInit();

struct g_entity_t *g_CreateEntity(struct ds_slist_t *list, struct e_ent_def_t *ent_def, vec3_t *position, vec3_t *scale, mat3_t *orientation);

void g_DestroyEntity(struct g_entity_t *entity);

void g_StepGame(float delta_time);

#endif
