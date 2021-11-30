#ifndef PHYS_H
#define PHYS_H

#include "p_defs.h"

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

void p_Init();

void p_Shutdown();

struct p_shape_def_t *p_AllocShapeDef();

void p_FreeShapeDef(struct p_shape_def_t *shape_def);

void *p_CreateCollisionShape(struct p_shape_def_t *shape_def);

void *p_CreateColliderCollisionShape(struct p_col_def_t *collider_def);

void p_DestroyCollisionShape(void *collision_shape);



struct p_collider_t *p_CreateCollider(struct p_col_def_t *col_def, vec3_t *position, mat3_t *orientation);

void p_UpdateColliderTransform(struct p_collider_t *collider);

//struct p_character_collider_t *p_CreateCharacterCollider(vec3_t *position, float step_height, float height, float radius, float crouch_height);

void p_TranslateCollider(struct p_collider_t *collider, vec3_t *disp);

void p_RotateCollider(struct p_collider_t *collider, mat3_t *rot);

void p_SetColliderPosition(struct p_collider_t *collider, vec3_t *position);

void p_SetColliderOrientation(struct p_collider_t *collider, mat3_t *orientation);

void p_SetColliderTransform(struct p_collider_t *collider, vec3_t *position, mat3_t *orientation);

void p_DestroyCollider(struct p_collider_t *collider);

struct p_collider_t *p_GetCollider(uint32_t type, uint32_t index);

struct p_dynamic_collider_t *p_GetDynamicCollider(uint32_t index);

void p_FreezeCollider(struct p_collider_t *collider);

void p_UnfreezeCollider(struct p_collider_t *collider);


struct p_constraint_t *p_CreateConstraint(struct p_constraint_def_t *constraint_def, struct p_collider_t *collider_a, struct p_collider_t *collider_b);

void p_DestroyConstraint(struct p_constraint_t *constraint);




//void p_SetColliderOrientation(struct p_collider_t *collider, mat3_t *orientation);

void p_MoveCharacterCollider(struct p_character_collider_t *collider, vec3_t *direction);

void p_JumpCharacterCollider(struct p_character_collider_t *collider);

//void p_RotateColliderX(struct p_collider_t *collider, float angle);
//
//void p_RotateColliderY(struct p_collider_t *collider, float angle);
//
//void p_RotateColliderZ(struct p_collider_t *collider, float angle);

void p_UpdateColliders(float delta_time);

void p_FreezePhysics();

void p_UnfreezePhysics();

//void p_UpdateColliderNode(struct p_collider_t *collider);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // PHYSICS_H





