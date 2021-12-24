#ifndef G_ENEMY_H
#define G_ENEMY_H

#include "g_defs.h"

void g_EnemyInit();

/*
========================================================

    common enemy stuff

========================================================
*/

struct g_enemy_t *g_SpawnEnemy(struct g_enemy_def_t *def, vec3_t *position, mat3_t *orientation);

struct g_enemy_t *g_GetEnemy(uint32_t type, uint32_t index);

void g_RemoveEnemy(struct g_enemy_t *enemy);

void g_RemoveAllEnemies();

void g_TranslateEnemy(struct g_enemy_t *enemy, vec3_t *translation);

void g_RotateEnemy(struct g_enemy_t *enemy, mat3_t *rotation);

void g_StepEnemies(float delta_time);

/*
========================================================

    camera stuff

========================================================
*/

void g_InitCamera(struct g_camera_t *camera, struct g_enemy_def_t *def, vec3_t *position, mat3_t *orientation);

void g_ShutdownCamera(struct g_camera_t *camera);

void g_CameraLookAt(struct g_camera_t *camera, vec3_t *target);

void g_StepCameras(float delta_time);

#endif // G_ENEMY_H
