#include "g_game.h"
#include "g_enemy.h"
#include "g_player.h"
#include "ent.h"

struct ds_list_t g_spawn_points;

void g_GameInit()
{
//    g_spawn_points = ds_list_create(sizeof(struct g_spawn_point_t), 8);
    g_PlayerInit();
    g_EnemyInit();
    g_CreateCamera(&vec3_t_c(0.0, 2.0, 0.0), 0.5, -0.5, -0.6, 0.6, 0.3, 10.0);
}

struct g_entity_t *g_CreateEntity(struct ds_slist_t *list, struct e_ent_def_t *ent_def, vec3_t *position, vec3_t *scale, mat3_t *orientation)
{
    uint32_t index = ds_slist_add_element(list, NULL);
    struct g_entity_t *entity = ds_slist_get_element(list, index);

    entity->index = index;
    entity->entity = e_SpawnEntity(ent_def, position, scale, orientation);

    return entity;
}

void g_StepGame(float delta_time)
{
    g_StepPlayer(delta_time);
    g_StepEnemies(delta_time);
}
