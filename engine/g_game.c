#include "g_game.h"
#include "g_enemy.h"
#include "g_player.h"
#include "../engine/ent.h"

struct ds_list_t g_spawn_points;

void g_GameInit()
{
    g_PlayerInit();
    g_EnemyInit();
}

struct g_thing_t *g_SpawnThing(uint32_t type, struct ds_slist_t *list, struct e_ent_def_t *ent_def, vec3_t *position, vec3_t *scale, mat3_t *orientation)
{
    uint32_t index = ds_slist_add_element(list, NULL);
    struct g_thing_t *thing = ds_slist_get_element(list, index);

    thing->index = index;
    thing->type = type;
    thing->entity = e_SpawnEntity(ent_def, position, scale, orientation);
    thing->list = list;

    e_UpdateEntityNode(thing->entity->node, &mat4_t_c_id());

    return thing;
}

struct g_thing_t *g_GetThing(struct ds_slist_t *list, uint32_t index)
{
    struct g_thing_t *thing = ds_slist_get_element(list, index);
    if(thing && thing->index == 0xffffffff)
    {
        thing = NULL;
    }
    return thing;
}

void g_RemoveThing(struct g_thing_t *thing)
{
    if(thing && thing->index != 0xffffffff)
    {
        e_DestroyEntity(thing->entity);
        ds_slist_remove_element(thing->list, thing->index);
        thing->index = 0xffffffff;
    }
}

void g_StepGame(float delta_time)
{
    g_StepPlayer(delta_time);
    g_StepEnemies(delta_time);
}
