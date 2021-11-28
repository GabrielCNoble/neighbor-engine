#include "e_defs.h"


void e_Init();

void e_Shutdown();

struct e_ent_def_t *e_AllocEntDef(uint32_t type);

struct e_ent_def_t *e_LoadEntDef(char *file_name);

struct e_ent_def_t *e_GetEntDef(uint32_t type, uint32_t index);

struct e_ent_def_t *e_FindEntDef(uint32_t type, char *name);

void e_DeallocEntDef(struct e_ent_def_t *ent_def);

struct e_constraint_t *e_AllocConstraint();

void e_DeallocConstraint(struct e_constraint_t *constraint);

struct e_component_t *e_AllocComponent(uint32_t type, struct e_entity_t *entity);

struct e_component_t *e_GetComponent(uint32_t type, uint32_t index);

void e_DeallocComponent(struct e_component_t *component);

struct e_node_t *e_AllocNode(vec3_t *position, vec3_t *scale, vec3_t *local_scale, mat3_t *orientation, struct e_entity_t *entity);

struct e_transform_t *e_AllocTransform(struct e_entity_t *entity);

struct e_collider_t *e_AllocCollider(struct p_col_def_t *col_def, struct e_entity_t *entity);

struct e_model_t *e_AllocModel(struct r_model_t *model, struct e_entity_t *entity);

struct e_entity_t *e_SpawnEntity(struct e_ent_def_t *ent_def, vec3_t *position, vec3_t *scale, mat3_t *orientation);

struct e_entity_t *e_GetEntity(uint32_t index);

void e_DestroyEntity(struct e_entity_t *entity);

void e_DestroyAllEntities();

void e_TranslateEntity(struct e_entity_t *entity, vec3_t *translation);

void e_RotateEntity(struct e_entity_t *entity, mat3_t *rotation);


void e_UpdateEntityNode(struct e_node_t *local_transform, mat4_t *parent_transform);

void e_UpdateEntities();
