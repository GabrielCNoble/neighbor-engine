#include "e_defs.h"


void e_Init();

void e_Shutdown();

struct e_ent_def_t *e_AllocEntDef();

struct e_ent_def_t *e_GetEntDef(uint32_t index);

struct e_ent_def_t *e_FindEntDef(char *name);

void e_DeallocEntDef(struct e_ent_def_t *ent_def);

struct e_component_t *e_AllocComponent(uint32_t type, struct e_entity_t *entity);

struct e_component_t *e_GetComponent(uint32_t type, uint32_t index);

void e_DeallocComponent(struct e_component_t *component);

struct e_local_transform_component_t *e_AllocLocalTransformComponent(vec3_t *position, vec3_t *scale, mat3_t *orientation, struct e_entity_t *entity);

struct e_transform_component_t *e_AllocTransformComponent(struct e_entity_t *entity);

struct e_physics_component_t *e_AllocPhysicsComponent(struct p_col_def_t *col_def, struct e_entity_t *entity);

struct e_model_component_t *e_AllocModelComponent(struct r_model_t *model, struct e_entity_t *entity);

struct e_entity_t *e_SpawnEntity(struct e_ent_def_t *ent_def, vec3_t *position, vec3_t *scale, mat3_t *orientation);

struct e_entity_t *e_GetEntity(uint32_t index);

void e_DestroyEntity(struct e_entity_t *entity);

void e_DestroyAllEntities();




void e_UpdateEntityLocalTransform(struct e_local_transform_component_t *local_transform, mat4_t *parent_transform);

void e_UpdateEntities();