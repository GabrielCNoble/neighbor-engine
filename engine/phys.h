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

void *p_CreateSimpleCollisionShape(struct p_shape_def_t *shape_def);

void *p_CreateCollisionShape(struct p_col_def_t *collider_def, vec3_t *shape_scale);

//void p_DestroyCollisionShape(void *collision_shape);



struct p_collider_t *p_CreateCollider(struct p_col_def_t *col_def, vec3_t *scale, vec3_t *position, mat3_t *orientation);

//void p_UpdateColliderTransform(struct p_collider_t *collider);

void p_SetColliderTransform(struct p_collider_t *collider, vec3_t *position, mat3_t *orientation);

void p_SetColliderVelocity(struct p_collider_t *collider, vec3_t *linear_velocity, vec3_t *angular_velocity);

//void p_SetColliderMass(struct p_collider_t *collider, float mass);

//void p_DisableColliderGravity(struct p_collider_t *collider);

void p_ApplyForce(struct p_collider_t *collider, vec3_t *force, vec3_t *relative_pos);

void p_ApplyImpulse(struct p_collider_t *collider, vec3_t *impulse, vec3_t *relative_pos, float delta_time);

void p_TransformCollider(struct p_collider_t *collider, vec3_t *translation, mat3_t *rotation);

void p_DestroyCollider(struct p_collider_t *collider);

struct p_collider_t *p_GetCollider(uint32_t type, uint32_t index);

struct ds_list_t *p_GetActiveColliders();

//struct p_dynamic_collider_t *p_GetDynamicCollider(uint32_t index);

void p_FreezeCollider(struct p_collider_t *collider);

void p_UnfreezeCollider(struct p_collider_t *collider);


struct p_constraint_t *p_CreateConstraint(struct p_constraint_def_t *constraint_def, struct p_collider_t *collider_a, struct p_collider_t *collider_b);

void p_DestroyConstraint(struct p_constraint_t *constraint);

void p_SetConstraintLinearLimits(struct p_constraint_t *constraint, float min, float max);

void p_SetConstraintAngularLimits(struct p_constraint_t *constraint, float min, float max);



void p_MoveCharacterCollider(struct p_character_collider_t *collider, vec3_t *direction);

void p_JumpCharacterCollider(struct p_character_collider_t *collider);

struct p_collider_t *p_Raycast(vec3_t *from, vec3_t *to, float *time, struct p_collider_t *ignore);

void p_StepPhysics(float delta_time);

void p_DebugDrawPhysics();

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // PHYSICS_H





