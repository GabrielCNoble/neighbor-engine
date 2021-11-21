#include "ed_ent.h"
#include "../engine/ent.h"
#include "../lib/dstuff/ds_vector.h"
#include "../lib/dstuff/ds_matrix.h"
#include <stddef.h>

//struct e_ent_def_t *ed_EntDefFromEntityRecursive(struct e_entity_t *entity, struct e_entity_t *parent)
//{
//    struct e_ent_def_t *ent_def = e_AllocEntDef();
//
//    struct e_local_transform_component_t *local_transform = entity->local_transform_component;
//    ent_def->local_scale = local_transform->local_scale;
//
//    if(parent)
//    {
//        ent_def->local_position = local_transform->local_position;
//        ent_def->local_orientation = local_transform->local_orientation;
//    }
//    else
//    {
//        ent_def->local_position = vec3_t_c(0.0, 0.0, 0.0);
//        ent_def->local_orientation = mat3_t_c_id();
//    }
//
//    if(entity->model_component)
//    {
//        struct e_model_component_t *model = entity->model_component;
//        ent_def->model = model->model;
//    }
//
//    if(entity->physics_component)
//    {
//
//    }
//}
//
//struct e_ent_def_t *ed_EntDefFromEntity(struct e_entity_t *entity)
//{
//    return ed_EntDefFromEntityRecursive(entity, NULL);
//}
