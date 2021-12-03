#include "g_game.h"
#include "g_enemy.h"
#include "g_player.h"

struct ds_slist_t g_entities[G_ENTITY_TYPE_LAST];
struct ds_list_t g_spawn_points;


void g_GameInit()
{
    g_entities[G_ENTITY_TYPE_CAMERA] = ds_slist_create(sizeof(struct g_camera_t), 32);
    g_entities[G_ENTITY_TYPE_TURRET] = ds_slist_create(sizeof(struct g_turret_t), 32);
    g_entities[G_ENTITY_TYPE_INTEL] = ds_slist_create(sizeof(struct g_intel_t), 32);
    g_spawn_points = ds_list_create(sizeof(struct g_spawn_point_t), 8);

    g_PlayerInit();
    g_EnemyInit();
}

void g_StepGame(float delta_time)
{
//    g_StepPlayer(delta_time);
    g_StepEnemies(delta_time);
}
