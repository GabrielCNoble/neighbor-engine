#include "g_barrier.h"
#include "ent.h"
#include "g_game.h"

struct e_ent_def_t *g_barrier_defs[G_BARRIER_TYPE_LAST][G_MAX_BARRIER_SUBTYPES];
struct ds_slist_t g_barriers;


void g_BarrierInit()
{
    g_barriers = ds_slist_create(sizeof(struct g_barrier_t), 64);
    g_barrier_defs[G_BARRIER_TYPE_HINGE_DOOR][G_HINGE_DOOR_TYPE_NORMAL] = e_LoadEntDef("normal_door");
    g_barrier_defs[G_BARRIER_TYPE_HINGE_DOOR][G_HINGE_DOOR_TYPE_TRAP_DOOR] = e_LoadEntDef("trap_door");
}

struct g_barrier_t *g_CreateBarrier(uint32_t type, uint32_t sub_type, vec3_t *position, mat3_t *orientation)
{
    struct e_ent_def_t *ent_def = g_barrier_defs[type][sub_type];
    struct g_barrier_t *barrier = (struct g_barrier_t *)g_SpawnThing(G_THING_TYPE_BARRIER, &g_barriers, ent_def, position, &vec3_t_c(1.0, 1.0, 1.1), orientation);
    barrier->type = type;
    barrier->sub_type = sub_type;
    return barrier;
}

struct g_barrier_t *g_GetBarrier(uint32_t index)
{
    return (struct g_barrier_t *)g_GetThing(&g_barriers, index);
}

void g_DestroyBarrier(struct g_barrier_t *barrier)
{
    if(barrier && barrier->thing.index != 0xffffffff)
    {
        g_RemoveThing(&barrier->thing);
    }
}

void g_UnlockBarrier(struct g_barrier_t *barrier, struct g_key_t *key)
{
    if(barrier && key && key->barrier == barrier)
    {
        switch(barrier->type)
        {
            case G_BARRIER_TYPE_HINGE_DOOR:
            {
                struct g_hinge_door_t *door = (struct g_hinge_door_t *)barrier;
                door->locked = 0;
            }
            break;
        }
    }
}


