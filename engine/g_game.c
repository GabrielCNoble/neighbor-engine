#include "g_game.h"
#include "g_enemy.h"
#include "g_player.h"
#include "ent.h"

struct ds_list_t g_spawn_points;

void g_GameInit()
{
    g_PlayerInit();
    g_EnemyInit();
}

struct g_entity_t *g_CreateEntity(struct ds_slist_t *list, struct e_ent_def_t *ent_def, vec3_t *position, vec3_t *scale, mat3_t *orientation)
{
    uint32_t index = ds_slist_add_element(list, NULL);
    struct g_entity_t *entity = ds_slist_get_element(list, index);

    entity->index = index;
    entity->entity = e_SpawnEntity(ent_def, position, scale, orientation);
    entity->list = list;

    e_UpdateEntityNode(entity->entity->node, &mat4_t_c_id());

    return entity;
}

void g_DestroyEntity(struct g_entity_t *entity)
{
    if(entity && entity->index != 0xffffffff)
    {
        e_DestroyEntity(entity->entity);
        ds_slist_remove_element(entity->list, entity->index);
        entity->index = 0xffffffff;
    }
}

void g_StepGame(float delta_time)
{
    g_StepPlayer(delta_time);
    g_StepEnemies(delta_time);
}
