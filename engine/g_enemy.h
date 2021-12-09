#ifndef G_ENEMY_H
#define G_ENEMY_H

#include "g_defs.h"

void g_EnemyInit();

struct g_enemy_t *g_CreateEnemy(uint32_t type, vec3_t *position, mat3_t *orientation);

void g_DestroyEnemy(struct g_enemy_t *enemy);

void g_DestroyAllEnemies();

struct g_enemy_t *g_GetEnemy(uint32_t type, uint32_t index);

void g_TranslateEnemy(struct g_enemy_t *enemy, vec3_t *translation);

void g_RotateEnemy(struct g_enemy_t *enemy, mat3_t *rotation);

struct g_camera_t *g_CreateCamera(vec3_t *position, mat3_t *orientation, struct g_camera_fields_t *fields);

void g_StepCameras(float delta_time);

void g_StepEnemies(float delta_time);

#endif // G_ENEMY_H
