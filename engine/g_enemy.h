#ifndef G_ENEMY_H
#define G_ENEMY_H

#include "g_defs.h"

void g_EnemyInit();

struct g_enemy_t *g_CreateEnemy(uint32_t type, vec3_t *position, mat3_t *orientation);

struct g_camera_t *g_CreateCamera(vec3_t *position, float min_pitch, float max_pitch, float min_yaw, float max_yaw, float idle_pitch, float range);

void g_StepCameras(float delta_time);

void g_StepEnemies(float delta_time);

#endif // G_ENEMY_H
