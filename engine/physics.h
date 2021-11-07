#ifndef PHYSICS_H
#define PHYSICS_H

#include "p_col.h"

void p_Init();

void p_Shutdown();

struct p_collider_t *p_CreateCollider(uint32_t type, vec3_t *position, mat3_t *orientation, struct p_col_shape_t *col_shape);

void p_DestroyCollider(struct p_collider_t *collider);

struct p_collider_t *p_GetCollider(uint32_t type, uint32_t index);

struct p_collider_t *p_GetCollision(struct p_collider_t *collider, uint32_t collision_index);

void p_TranslateCollider(struct p_collider_t *collider, vec3_t *disp);

void p_RotateCollider(struct p_collider_t *collider, mat3_t *rot);

//void p_RotateColliderX(struct p_collider_t *collider, float angle);
//
//void p_RotateColliderY(struct p_collider_t *collider, float angle);
//
//void p_RotateColliderZ(struct p_collider_t *collider, float angle);

void p_UpdateColliders(float delta_time);

void p_UpdateColliderNode(struct p_collider_t *collider);

#endif // PHYSICS_H





