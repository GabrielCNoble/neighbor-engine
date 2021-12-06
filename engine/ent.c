#include <stdio.h>
#include "ent.h"
#include "phys.h"
#include "r_defs.h"
#include "r_draw.h"
#include "g_main.h"
#include "../lib/dstuff/ds_slist.h"
#include "../lib/dstuff/ds_path.h"
#include "log.h"

struct ds_list_t e_components[E_COMPONENT_TYPE_LAST];
struct ds_slist_t e_ent_defs[E_ENT_DEF_TYPE_LAST];
struct ds_slist_t e_entities;
struct ds_slist_t e_constraints;
struct ds_list_t e_root_transforms;
uint32_t e_valid_def_ents;

extern struct r_renderer_state_t r_renderer_state;


void e_Init()
{
    log_ScopedLogMessage(LOG_TYPE_NOTICE, "Initializing entities...");
    e_components[E_COMPONENT_TYPE_NODE] = ds_list_create(sizeof(struct e_node_t), 512);
    e_components[E_COMPONENT_TYPE_TRANSFORM] = ds_list_create(sizeof(struct e_transform_t), 512);
    e_components[E_COMPONENT_TYPE_COLLIDER] = ds_list_create(sizeof(struct e_collider_t), 512);
    e_components[E_COMPONENT_TYPE_MODEL] = ds_list_create(sizeof(struct e_model_t), 512);

    e_ent_defs[E_ENT_DEF_TYPE_ROOT] = ds_slist_create(sizeof(struct e_ent_def_t), 512);
    e_ent_defs[E_ENT_DEF_TYPE_CHILD] = ds_slist_create(sizeof(struct e_ent_def_t), 512);

    e_entities = ds_slist_create(sizeof(struct e_entity_t), 512);
    e_root_transforms = ds_list_create(sizeof(struct e_node_t *), 512);
    e_constraints = ds_slist_create(sizeof(struct e_constraint_t), 512);
    log_ScopedLogMessage(LOG_TYPE_NOTICE, "Entities initialized!");
}

void e_Shutdown()
{
    log_ScopedLogMessage(LOG_TYPE_NOTICE, "Shutting down entities...");
    log_ScopedLogMessage(LOG_TYPE_NOTICE, "Entities shut down!");
}

struct e_ent_def_t *e_AllocEntDef(uint32_t type)
{
    uint32_t index;
    struct e_ent_def_t *ent_def;

    index = ds_slist_add_element(&e_ent_defs[type], NULL);
    ent_def = ds_slist_get_element(&e_ent_defs[type], index);

    ent_def->index = index;
    ent_def->type = type;
    ent_def->collider.passive.shape_count = 0;
    ent_def->collider.passive.shape = NULL;
    ent_def->children = NULL;
    ent_def->node_count = 0;
    ent_def->constraint_count = 0;
    ent_def->collider_count = 0;
    ent_def->shape_count = 0;
    ent_def->next = NULL;
    ent_def->prev = NULL;
    ent_def->model = NULL;

    return ent_def;
}

void e_DeserializeEntDefRecursive(char *start_in_buffer, struct e_ent_def_t *ent_def, struct e_ent_def_record_t *record)
{
    ent_def->node_count = 1;
    ent_def->position = record->position;
    ent_def->orientation = record->orientation;
    ent_def->scale = record->scale;

    if(record->model[0])
    {
        ent_def->model = r_FindModel(record->model);
        if(!ent_def->model)
        {
            ent_def->model = r_LoadModel(record->model);
        }
    }

    if(record->collider_start)
    {
        struct p_col_def_record_t *collider_record = (struct p_col_def_record_t *)(start_in_buffer + record->collider_start);

        ent_def->collider.type = collider_record->type;
        ent_def->collider_count = 1;
        if(collider_record->type == P_COLLIDER_TYPE_CHARACTER)
        {
            ent_def->collider.character.step_height = collider_record->character.step_height;
            ent_def->collider.character.crouch_height = collider_record->character.crouch_height;
            ent_def->collider.character.radius = collider_record->character.radius;
            ent_def->collider.character.height = collider_record->character.height;
        }
        else
        {
            ent_def->collider.passive.mass = collider_record->passive.mass;
            ent_def->collider.passive.shape_count = collider_record->passive.shape_count;

            for(uint32_t shape_index = 0; shape_index < collider_record->passive.shape_count; shape_index++)
            {
                struct p_shape_def_fields_t *shape_fields = collider_record->passive.shape + shape_index;
                struct p_shape_def_t *shape_def = p_AllocShapeDef();
                shape_def->fields = *shape_fields;
                shape_def->next = ent_def->collider.passive.shape;
                ent_def->collider.passive.shape = shape_def;
            }

            ent_def->shape_count = ent_def->collider.passive.shape_count;
        }
    }
    else
    {
        ent_def->collider.type = P_COLLIDER_TYPE_LAST;
    }

    if(record->child_count)
    {
        char *children = start_in_buffer + record->child_start;

        if(record->constraint_count)
        {
            struct e_constraint_record_t *constraint_records = (struct e_constraint_record_t *)(start_in_buffer + record->constraint_start);
            ent_def->constraint_count = record->constraint_count;

            for(uint32_t child_index = 0; child_index < record->child_count; child_index++)
            {
                struct e_ent_def_record_t *child_record = (struct e_ent_def_record_t *)children;
                struct e_ent_def_t *child_def = e_AddChildEntDef(ent_def);
                e_DeserializeEntDefRecursive(start_in_buffer, child_def, child_record);
                children += child_record->record_size;
                ent_def->node_count += child_def->node_count;
                ent_def->constraint_count += child_def->constraint_count;
                ent_def->shape_count += child_def->shape_count;
                ent_def->collider_count += child_def->collider_count;

                for(uint32_t constraint_index = 0; constraint_index < record->constraint_count; constraint_index++)
                {
                    struct e_constraint_record_t *constraint_record = constraint_records + constraint_index;

                    if(constraint_record->child_index == child_index)
                    {
                        struct e_constraint_t *constraint = e_AllocConstraint();
                        constraint->child_def = child_def;
                        constraint->constraint.fields = constraint_record->fields;

                        constraint->next = ent_def->constraints;
                        if(ent_def->constraints)
                        {
                            ent_def->constraints->prev = constraint;
                        }
                        ent_def->constraints = constraint;
//                        ent_def->constraint_count++;

                        break;
                    }
                }
            }
        }
        else
        {
            for(uint32_t child_index = 0; child_index < record->child_count; child_index++)
            {
                struct e_ent_def_record_t *child_record = (struct e_ent_def_record_t *)children;
                struct e_ent_def_t *child_def = e_AddChildEntDef(ent_def);
                e_DeserializeEntDefRecursive(start_in_buffer, child_def, child_record);
                children += child_record->record_size;
                ent_def->node_count += child_def->node_count;
                ent_def->constraint_count += child_def->constraint_count;
                ent_def->shape_count += child_def->shape_count;
                ent_def->collider_count += child_def->collider_count;
            }
        }
    }
}

struct e_ent_def_t *e_DeserializeEntDef(void *buffer, size_t buffer_size)
{
    char *in_buffer = buffer;
    struct e_ent_def_section_t *section = (struct e_ent_def_section_t *)in_buffer;
    struct e_ent_def_t *ent_def = e_AllocEntDef(E_ENT_DEF_TYPE_ROOT);
    struct e_ent_def_record_t *record = (struct e_ent_def_record_t *)(in_buffer + section->data_start);

    e_DeserializeEntDefRecursive(in_buffer, ent_def, record);

    return ent_def;
}

struct e_ent_def_t *e_LoadEntDef(char *file_name)
{
    void *buffer;
    size_t buffer_size;
    struct e_ent_def_t *ent_def;
    char full_path[PATH_MAX];

    strcpy(full_path, file_name);

    if(!strstr(full_path, "entities"))
    {
        ds_path_append_end("entities", full_path, full_path, PATH_MAX);
    }

    if(!strstr(full_path, ".ent"))
    {
        ds_path_set_ext(full_path, "ent", full_path, PATH_MAX);
    }

    FILE *file = fopen(full_path, "rb");

    if(!file)
    {
        log_ScopedLogMessage(LOG_TYPE_ERROR, "Couldn't load ent def [%s]!", full_path);
        return NULL;
    }

    read_file(file, &buffer, &buffer_size);
    ent_def = e_DeserializeEntDef(buffer, buffer_size);
    free(buffer);

    ds_path_get_end(file_name, ent_def->name, sizeof(ent_def->name));
    ds_path_drop_ext(ent_def->name, ent_def->name, sizeof(ent_def->name));

    log_ScopedLogMessage(LOG_TYPE_NOTICE, "Ent def [%s] loaded!", full_path);

    return ent_def;
}

struct e_ent_def_t *e_GetEntDef(uint32_t type, uint32_t index)
{
    struct e_ent_def_t *ent_def = ds_slist_get_element(&e_ent_defs[type], index);
    if(ent_def && ent_def->index == 0xffffffff)
    {
        ent_def = NULL;
    }

    return ent_def;
}

struct e_ent_def_t *e_FindEntDef(uint32_t type, char *name)
{
    for(uint32_t index = 0; index < e_ent_defs[type].cursor; index++)
    {
        struct e_ent_def_t *ent_def = e_GetEntDef(type, index);

        if(!strcmp(name, ent_def->name))
        {
            return ent_def;
        }
    }

    return NULL;
}

void e_DeallocEntDef(struct e_ent_def_t *ent_def)
{
    if(ent_def && ent_def->index != 0xffffffff)
    {
        struct e_ent_def_t *child = ent_def->children;

        while(child)
        {
            struct e_ent_def_t *next_child = child->next;
            e_DeallocEntDef(child);
            child = next_child;
        }

        ds_slist_remove_element(&e_ent_defs[ent_def->type], ent_def->index);
        ent_def->index = 0xffffffff;
    }
}

struct e_ent_def_t *e_AddChildEntDef(struct e_ent_def_t *parent_def)
{
    struct e_ent_def_t *child_def = NULL;

    if(parent_def)
    {
        child_def = e_AllocEntDef(E_ENT_DEF_TYPE_CHILD);
        child_def->collider.type = P_COLLIDER_TYPE_LAST;
        child_def->collider.passive.shape_count = 0;

        child_def->next = parent_def->children;
        if(parent_def->children)
        {
            parent_def->children->prev = child_def;
        }
        parent_def->children = child_def;
    }

    return child_def;
}

struct e_constraint_t *e_AllocConstraint()
{
    uint32_t index = ds_slist_add_element(&e_constraints, NULL);
    struct e_constraint_t *constraint = ds_slist_get_element(&e_constraints, index);
    constraint->index = index;
    constraint->next = NULL;
    constraint->prev = NULL;
    constraint->child_def = NULL;
    constraint->constraint.type = P_CONSTRAINT_TYPE_LAST;

    return constraint;
}

void e_DeallocConstraint(struct e_constraint_t *constraint)
{
    if(constraint && constraint->index != 0xffffffff)
    {
        ds_slist_remove_element(&e_constraints, constraint->index);
        constraint->index = 0xffffffff;
    }
}

struct e_component_t *e_AllocComponent(uint32_t type, struct e_entity_t *entity)
{
    uint32_t index;
    struct e_component_t *component;

    index = ds_list_add_element(&e_components[type], NULL);
    component = ds_list_get_element(&e_components[type], index);
    component->type = type;
    component->index = index;
    component->entity = entity;

    return component;
}

struct e_component_t *e_GetComponent(uint32_t type, uint32_t index)
{
    struct e_component_t *component = ds_list_get_element(&e_components[type], index);
    if(component && component->index == 0xffffffff)
    {
        component = NULL;
    }

    return component;
}

void e_DeallocComponent(struct e_component_t *component)
{
    if(component && component->index != 0xffffffff)
    {

        switch(component->type)
        {
            case E_COMPONENT_TYPE_NODE:
            {
                struct e_node_t *transform = (struct e_node_t *)component;
                struct e_node_t *child = transform->children;

                while(child)
                {
                    struct e_node_t *next_child = child->next;
                    e_DeallocComponent((struct e_component_t *)child);
                    child = next_child;
                }

                if(transform->parent)
                {
                    if(transform->prev)
                    {
                        transform->prev->next = transform->next;
                    }
                    else
                    {
                        transform->parent->children = transform->next;
                    }

                    if(transform->next)
                    {
                        transform->next->prev = transform->prev;
                    }
                }
                else
                {
                    uint32_t root_index = transform->root_index;
                    ds_list_remove_element(&e_root_transforms, root_index);

                    if(root_index < e_root_transforms.cursor)
                    {
                        struct e_node_t *moved_transform;
                        moved_transform = *(struct e_node_t **)ds_list_get_element(&e_root_transforms, root_index);
                        moved_transform->root_index = root_index;
                    }
                }
            }
            break;

            case E_COMPONENT_TYPE_COLLIDER:
            {
                struct e_collider_t *collider = (struct e_collider_t *)component;
                p_DestroyCollider(collider->collider);
            }
            break;
        }

        uint32_t index = component->index;
        component->index = 0xffffffff;
        ds_list_remove_element(&e_components[component->type], index);

        /* lists move the last element of the list to fill in the blank. If
        the last element of the list got moved into the slot we just freed we
        need to update some stuff to keep things running */
        if(index < e_components[component->type].cursor)
        {
            /* make the entity point at the new component */
            component->entity->components[component->type] = component;

            if(component->type == E_COMPONENT_TYPE_NODE)
            {
                /* local transform components take part in a hierarchy, so we need to update its parent child list
                to make it point to the new component */
                struct e_node_t *transform = (struct e_node_t *)component;
                struct e_node_t *parent = transform->parent;
                struct e_node_t *child = transform->children;

                if(child)
                {
                    while(child)
                    {
                        child->parent = transform;
                        child = child->next;
                    }
                }

                if(parent)
                {
                    struct e_node_t *child_transform = parent->children;

                    while(child_transform)
                    {
                        if(child_transform->index == transform->index)
                        {
                            if(child_transform->prev)
                            {
                                child_transform->prev->next = transform;
                            }
                            else
                            {
                                parent->children = transform;
                            }

                            if(child_transform->next)
                            {
                                child_transform->next->prev = transform;
                            }

                            break;
                        }

                        child_transform = child_transform->next;
                    }
                }
                else
                {
                    struct e_node_t **transform_ptr = ds_list_get_element(&e_root_transforms, transform->root_index);
                    *transform_ptr = transform;
                }
            }

            component->index = index;
        }
    }
}

struct e_node_t *e_AllocNode(vec3_t *position, vec3_t *scale, vec3_t *local_scale, mat3_t *orientation, struct e_entity_t *entity)
{
    struct e_node_t *component;
    component = (struct e_node_t *)e_AllocComponent(E_COMPONENT_TYPE_NODE, entity);

    component->parent = NULL;
    component->children = NULL;
    component->next = NULL;
    component->prev = NULL;
    component->position = *position;
    component->orientation = *orientation;
    component->scale = *scale;
    component->local_scale = *local_scale;
    component->root_index = 0xffffffff;

    return component;
}

struct e_transform_t *e_AllocTransform(struct e_entity_t *entity)
{
    struct e_transform_t *component = (struct e_transform_t *)e_AllocComponent(E_COMPONENT_TYPE_TRANSFORM, entity);
    component->extents = vec3_t_c(0.0, 0.0, 0.0);
    return component;
}

struct e_collider_t *e_AllocCollider(struct p_col_def_t *col_def, struct e_entity_t *entity)
{
    struct e_collider_t *component = (struct e_collider_t *)e_AllocComponent(E_COMPONENT_TYPE_COLLIDER, entity);

    if(col_def->type != P_COLLIDER_TYPE_CHARACTER && col_def->passive.shape_count == 1)
    {
        component->offset_position = col_def->passive.shape->position;
        component->offset_rotation = col_def->passive.shape->orientation;
    }
    else
    {
        component->offset_position = vec3_t_c(0.0, 0.0, 0.0);
        component->offset_rotation = mat3_t_c_id();
    }

    vec3_t offset_position = component->offset_position;
    mat3_t offset_orientation = component->offset_rotation;
    vec3_t entity_position;
    mat3_t entity_orientation;

    if(entity)
    {
        struct e_transform_t *transform = entity->transform;
        entity_position = transform->transform.rows[3].xyz;
        entity_orientation.rows[0] = transform->transform.rows[0].xyz;
        entity_orientation.rows[1] = transform->transform.rows[1].xyz;
        entity_orientation.rows[2] = transform->transform.rows[2].xyz;
    }
    else
    {
        entity_position = vec3_t_c(0.0, 0.0, 0.0);
        entity_orientation = mat3_t_c_id();
    }

    mat3_t_vec3_t_mul(&offset_position, &offset_position, &entity_orientation);
    vec3_t_add(&entity_position, &entity_position, &offset_position);
    mat3_t_mul(&entity_orientation, &offset_orientation, &entity_orientation);

    component->collider = p_CreateCollider(col_def, &entity_position, &entity_orientation);
    component->collider->user_pointer = component;
    mat3_t_transpose(&component->offset_rotation, &component->offset_rotation);

    return component;
}

struct e_model_t *e_AllocModel(struct r_model_t *model, struct e_entity_t *entity)
{
    struct e_model_t *component = (struct e_model_t *)e_AllocComponent(E_COMPONENT_TYPE_MODEL, entity);
    component->model = model;
    return component;
}

struct e_entity_t *e_SpawnEntityRecursive(struct e_ent_def_t *ent_def, vec3_t *position, vec3_t *scale, vec3_t *local_scale, mat3_t *orientation, struct e_entity_t *parent)
{
    uint32_t index;
    struct e_entity_t *entity;

    index = ds_slist_add_element(&e_entities, NULL);
    entity = ds_slist_get_element(&e_entities, index);

    entity->node = e_AllocNode(position, scale, local_scale, orientation, entity);
    entity->transform = e_AllocTransform(entity);
    entity->index = index;
    entity->def = NULL;

    mat4_t scale_transform = mat4_t_c_id();
    scale_transform.rows[0].x = entity->node->scale.x;
    scale_transform.rows[1].y = entity->node->scale.y;
    scale_transform.rows[2].z = entity->node->scale.z;

    mat4_t_comp(&entity->transform->transform, &entity->node->orientation, &entity->node->position);
    mat4_t_mul(&entity->transform->transform, &scale_transform, &entity->transform->transform);

    if(parent)
    {
        mat4_t_mul(&entity->transform->transform, &entity->transform->transform, &parent->transform->transform);
    }

    if(ent_def->model)
    {
        entity->model = e_AllocModel(ent_def->model, entity);
    }

    if(ent_def->collider.type != P_COLLIDER_TYPE_LAST)
    {
        entity->collider = e_AllocCollider(&ent_def->collider, entity);
        ent_def->entity = entity;
    }

    if(ent_def->children)
    {
        struct e_ent_def_t *child_def = ent_def->children;

        while(child_def)
        {
            struct e_entity_t *child_entity = e_SpawnEntityRecursive(child_def, &child_def->position, &vec3_t_c(1.0, 1.0, 1.0), &child_def->scale, &child_def->orientation, entity);
            struct e_node_t *child_transform = child_entity->node;
            child_transform->parent = entity->node;

            child_transform->next = entity->node->children;
            if(entity->node->children)
            {
                entity->node->children->prev = child_transform;
            }
            entity->node->children = child_transform;

            child_def = child_def->next;
        }
    }

    if(ent_def->constraints)
    {
        struct e_constraint_t *constraint = ent_def->constraints;

        while(constraint)
        {
            struct e_entity_t *child = constraint->child_def->entity;
            p_CreateConstraint(&constraint->constraint, entity->collider->collider, child->collider->collider);
            constraint->child_def->entity = NULL;
            constraint = constraint->next;
        }
    }

    return entity;
}

struct e_entity_t *e_SpawnEntity(struct e_ent_def_t *ent_def, vec3_t *position, vec3_t *scale, mat3_t *orientation)
{
    struct e_entity_t *entity = e_SpawnEntityRecursive(ent_def, position, scale, &ent_def->scale, orientation, NULL);
    entity->node->root_index = ds_list_add_element(&e_root_transforms, &entity->node);

    if(ent_def->type == E_ENT_DEF_TYPE_ROOT)
    {
        entity->def = ent_def;
    }

    return entity;
}

struct e_entity_t *e_CopyEntity(struct e_entity_t *entity)
{
    struct e_node_t *node = entity->node;
    struct e_entity_t *copy = e_SpawnEntity(entity->def, &node->position, &node->scale, &node->orientation);
    return copy;
}

struct e_entity_t *e_GetEntity(uint32_t index)
{
    struct e_entity_t *entity = ds_slist_get_element(&e_entities, index);

    if(entity && entity->index == 0xffffffff)
    {
        entity = NULL;
    }

    return entity;
}

void e_DestroyEntity(struct e_entity_t *entity)
{
    if(entity && entity->index != 0xffffffff)
    {
        struct e_node_t *transform = entity->node;

        if(transform->children)
        {
            struct e_node_t *child_transfom = transform->children;

            while(child_transfom)
            {
                struct e_node_t *next_child_transform = child_transfom->next;
                e_DestroyEntity(child_transfom->entity);
                child_transfom = next_child_transform;
            }
        }

        for(uint32_t component = 0; component < E_COMPONENT_TYPE_LAST; component++)
        {
            e_DeallocComponent(entity->components[component]);
            entity->components[component] = NULL;
        }

        ds_slist_remove_element(&e_entities, entity->index);
        entity->index = 0xffffffff;
    }
}

void e_DestroyAllEntities()
{
    for(uint32_t entity_index = 0; entity_index < e_entities.cursor; entity_index++)
    {
        struct e_entity_t *entity = e_GetEntity(entity_index);

        if(entity)
        {
            e_DestroyEntity(entity);
        }
    }
}

void e_SetEntityPosition(struct e_entity_t *entity, vec3_t *position)
{
    vec3_t *translation;
    struct e_node_t *node = entity->node;
}

void e_TranslateEntity(struct e_entity_t *entity, vec3_t *translation)
{
    if(entity && entity->index != 0xffffffff)
    {
        vec3_t_add(&entity->node->position, &entity->node->position, translation);

        if(entity->collider)
        {
            p_SetColliderTransform(entity->collider->collider, &entity->node->position, &entity->node->orientation);
//            p_TransformCollider(entity->collider->collider, translation, &mat3_t_c_id());
        }
    }
}

void e_RotateEntity(struct e_entity_t *entity, mat3_t *rotation)
{
    if(entity && entity->index != 0xffffffff)
    {
        if(entity->collider)
        {
            vec3_t start_pos = entity->collider->offset_position;
            vec3_t collider_pos;
            mat3_t_vec3_t_mul(&start_pos, &start_pos, &entity->node->orientation);
            mat3_t_vec3_t_mul(&collider_pos, &start_pos, rotation);
            vec3_t_add(&collider_pos, &collider_pos, &entity->node->position);
            mat3_t_mul(&entity->node->orientation, &entity->node->orientation, rotation);
            p_SetColliderTransform(entity->collider->collider, &entity->node->position, &entity->node->orientation);
        }
        else
        {
            mat3_t_mul(&entity->node->orientation, &entity->node->orientation, rotation);
        }
    }
}

struct e_entity_t *e_Raycast(vec3_t *from, vec3_t *to, float *time)
{
    struct p_collider_t *collider = p_Raycast(from, to, time);

    if(collider && collider->user_pointer)
    {
        struct e_collider_t *entity_collider = (struct e_collider_t *)collider->user_pointer;
        return entity_collider->entity;
    }

    return NULL;
}

void e_UpdateEntityNode(struct e_node_t *local_transform, mat4_t *parent_transform)
{
    struct e_transform_t *transform = local_transform->entity->transform;
    mat3_t local_orientation = mat3_t_c_id();

    local_orientation.rows[0].x = local_transform->scale.x;
    local_orientation.rows[1].y = local_transform->scale.y;
    local_orientation.rows[2].z = local_transform->scale.z;
    mat3_t_mul(&local_orientation, &local_orientation, &local_transform->orientation);
    mat4_t_comp(&transform->transform, &local_orientation, &local_transform->position);

    if(!transform->entity->collider)
    {
        mat4_t_mul(&transform->transform, &transform->transform, parent_transform);
    }

    struct e_node_t *child = local_transform->children;

    while(child)
    {
        e_UpdateEntityNode(child, &transform->transform);
        child = child->next;
    }

    mat4_t local_scale = mat4_t_c_id();
    local_scale.rows[0].x = local_transform->local_scale.x;
    local_scale.rows[1].y = local_transform->local_scale.y;
    local_scale.rows[2].z = local_transform->local_scale.z;

    mat4_t_mul(&transform->transform, &local_scale, &transform->transform);
}

void e_UpdateEntities()
{
    for(uint32_t collider_index = 0; collider_index < e_components[E_COMPONENT_TYPE_COLLIDER].cursor; collider_index++)
    {
        struct e_collider_t *collider = (struct e_collider_t *)e_GetComponent(E_COMPONENT_TYPE_COLLIDER, collider_index);
        struct e_node_t *transform = collider->entity->node;
        mat3_t_mul(&transform->orientation, &collider->offset_rotation, &collider->collider->orientation);
        vec3_t offset_position = collider->offset_position;
        mat3_t_vec3_t_mul(&offset_position, &offset_position, &transform->orientation);
        vec3_t_sub(&transform->position, &collider->collider->position, &offset_position);
    }

    for(uint32_t root_index = 0; root_index < e_root_transforms.cursor; root_index++)
    {
        struct e_node_t *local_transform = *(struct e_node_t **)ds_list_get_element(&e_root_transforms, root_index);
        e_UpdateEntityNode(local_transform, &mat4_t_c_id());
    }

    for(uint32_t model_index = 0; model_index < e_components[E_COMPONENT_TYPE_MODEL].cursor; model_index++)
    {
        struct e_model_t *model = (struct e_model_t *)e_GetComponent(E_COMPONENT_TYPE_MODEL, model_index);

        vec3_t corners[8];
        vec3_t min;
        vec3_t max;

        struct e_transform_t *transform = model->entity->transform;

        max = model->model->max;
        min = model->model->min;

        corners[0] = max;

        corners[1].x = max.x;
        corners[1].y = min.y;
        corners[1].z = max.z;

        corners[2].x = min.x;
        corners[2].y = min.y;
        corners[2].z = max.z;

        corners[3].x = min.x;
        corners[3].y = max.y;
        corners[3].z = max.z;

        corners[4].x = max.x;
        corners[4].y = max.y;
        corners[4].z = min.z;

        corners[5].x = max.x;
        corners[5].y = min.y;
        corners[5].z = min.z;

        corners[6].x = min.x;
        corners[6].y = min.y;
        corners[6].z = min.z;

        corners[7] = min;

        mat3_t rot_scale;

        rot_scale.rows[0] = transform->transform.rows[0].xyz;
        rot_scale.rows[1] = transform->transform.rows[1].xyz;
        rot_scale.rows[2] = transform->transform.rows[2].xyz;

        max = vec3_t_c(-FLT_MAX, -FLT_MAX, -FLT_MAX);
        min = vec3_t_c(FLT_MAX, FLT_MAX, FLT_MAX);

        for(uint32_t corner_index = 0; corner_index < 8; corner_index++)
        {
            vec3_t *corner = corners + corner_index;
            mat3_t_vec3_t_mul(corner, corner, &rot_scale);

            if(max.x < corner->x) max.x = corner->x;
            if(max.y < corner->y) max.y = corner->y;
            if(max.z < corner->z) max.z = corner->z;

            if(min.x > corner->x) min.x = corner->x;
            if(min.y > corner->y) min.y = corner->y;
            if(min.z > corner->z) min.z = corner->z;
        }

        transform->extents.x = max.x - min.x;
        transform->extents.y = max.y - min.y;
        transform->extents.z = max.z - min.z;
    }

    if(r_renderer_state.draw_entities)
    {
        r_i_SetViewProjectionMatrix(NULL);
        r_i_SetModelMatrix(NULL);
        r_i_SetShader(NULL);

        vec3_t min;
        vec3_t max;

        for(uint32_t model_index = 0; model_index < e_components[E_COMPONENT_TYPE_MODEL].cursor; model_index++)
        {
            struct e_model_t *model = (struct e_model_t *)e_GetComponent(E_COMPONENT_TYPE_MODEL, model_index);
            struct e_transform_t *transform = model->entity->transform;

            vec3_t_mul(&max, &transform->extents, 0.5);
            vec3_t_mul(&min, &transform->extents, -0.5);

            vec3_t_add(&max, &max, &transform->transform.rows[3].xyz);
            vec3_t_add(&min, &min, &transform->transform.rows[3].xyz);

            vec4_t color = vec4_t_c(0.0, 1.0, 0.0, 1.0);

            r_i_DrawLine(&vec3_t_c(min.x, max.y, max.z), &vec3_t_c(min.x, min.y, max.z), &color, 1.0);
            r_i_DrawLine(&vec3_t_c(max.x, max.y, max.z), &vec3_t_c(max.x, min.y, max.z), &color, 1.0);
            r_i_DrawLine(&vec3_t_c(min.x, max.y, max.z), &vec3_t_c(max.x, max.y, max.z), &color, 1.0);
            r_i_DrawLine(&vec3_t_c(min.x, min.y, max.z), &vec3_t_c(max.x, min.y, max.z), &color, 1.0);

            r_i_DrawLine(&vec3_t_c(min.x, max.y, min.z), &vec3_t_c(min.x, min.y, min.z), &color, 1.0);
            r_i_DrawLine(&vec3_t_c(max.x, max.y, min.z), &vec3_t_c(max.x, min.y, min.z), &color, 1.0);
            r_i_DrawLine(&vec3_t_c(min.x, max.y, min.z), &vec3_t_c(max.x, max.y, min.z), &color, 1.0);
            r_i_DrawLine(&vec3_t_c(min.x, min.y, min.z), &vec3_t_c(max.x, min.y, min.z), &color, 1.0);


            r_i_DrawLine(&vec3_t_c(min.x, max.y, min.z), &vec3_t_c(min.x, max.y, max.z), &color, 1.0);
            r_i_DrawLine(&vec3_t_c(min.x, min.y, min.z), &vec3_t_c(min.x, min.y, max.z), &color, 1.0);
            r_i_DrawLine(&vec3_t_c(max.x, max.y, min.z), &vec3_t_c(max.x, max.y, max.z), &color, 1.0);
            r_i_DrawLine(&vec3_t_c(max.x, min.y, min.z), &vec3_t_c(max.x, min.y, max.z), &color, 1.0);
        }
    }
}















