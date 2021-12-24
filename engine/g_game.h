#ifndef G_GAME_H
#define G_GAME_H

#include "g_defs.h"
#include "../lib/dstuff/ds_slist.h"

void g_GameInit();

struct g_thing_t *g_SpawnThing(uint32_t type, struct ds_slist_t *list, struct e_ent_def_t *ent_def, vec3_t *position, vec3_t *scale, mat3_t *orientation);

struct g_thing_t *g_GetThing(struct ds_slist_t *list, uint32_t index);

void g_RemoveThing(struct g_thing_t *thing);

void g_StepGame(float delta_time);

#endif
