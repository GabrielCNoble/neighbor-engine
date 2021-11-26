#include "ent.h"
#include "phys.h"
#include "r_defs.h"
#include "r_draw.h"
#include "../lib/dstuff/ds_slist.h"

struct ds_list_t e_components[E_COMPONENT_TYPE_LAST];
struct ds_slist_t e_ent_defs[E_ENT_DEF_TYPE_LAST];
struct ds_slist_t e_entities;
struct ds_list_t e_root_transforms;
uint32_t e_valid_def_ents;

extern struct r_renderer_state_t r_renderer_state;


void e_Init()
{
    e_components[E_COMPONENT_TYPE_NODE] = ds_list_create(sizeof(struct e_node_t), 512);
    e_components[E_COMPONENT_TYPE_TRANSFORM] = ds_list_create(sizeof(struct e_transform_t), 512);
    e_components[E_COMPONENT_TYPE_COLLIDER] = ds_list_create(sizeof(struct e_collider_t), 512);
    e_components[E_COMPONENT_TYPE_MODEL] = ds_list_create(sizeof(struct e_model_t), 512);

    e_ent_defs[E_ENT_DEF_TYPE_ROOT] = ds_slist_create(sizeof(struct e_ent_def_t), 512);
    e_ent_defs[E_ENT_DEF_TYPE_CHILD] = ds_slist_create(sizeof(struct e_ent_def_t), 512);

    e_entities = ds_slist_create(sizeof(struct e_entity_t), 512);
    e_root_transforms = ds_list_create(sizeof(struct e_node_t *), 512);
}

void e_Shutdown()
{

}

struct e_ent_def_t *e_AllocEntDef(uint32_t type)
{
    uint32_t index;
    struct e_ent_def_t *ent_def;

    index = ds_slist_add_element(&e_ent_defs[type], NULL);
    ent_def = ds_slist_get_element(&e_ent_defs[type], index);

    ent_def->index = index;
    ent_def->type = type;
    ent_def->collider.shape_count = 0;
    ent_def->children = NULL;
    ent_def->next = NULL;
    ent_def->prev = NULL;
    ent_def->model = NULL;


    return ent_def;
}

struct e_ent_def_t *e_LoadEntDef(char *file_name)
{

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

struct e_node_t *e_AllocNode(vec3_t *position, vec3_t *scale, mat3_t *orientation, struct e_entity_t *entity)
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

    if(entity)
    {
        struct e_node_t *transform = entity->node;
        component->collider = p_CreateCollider(col_def, &transform->position, &transform->orientation);
    }
    else
    {
        component->collider = p_CreateCollider(col_def, &vec3_t_c(0.0, 0.0, 0.0), &mat3_t_c_id());
    }

    return component;
}

struct e_model_t *e_AllocModel(struct r_model_t *model, struct e_entity_t *entity)
{
    struct e_model_t *component = (struct e_model_t *)e_AllocComponent(E_COMPONENT_TYPE_MODEL, entity);
    component->model = model;
    return component;
}

struct e_entity_t *e_SpawnEntityRecursive(struct e_ent_def_t *ent_def, vec3_t *position, vec3_t *scale, mat3_t *orientation, struct e_entity_t *parent)
{
    uint32_t index;
    struct e_entity_t *entity;

    index = ds_slist_add_element(&e_entities, NULL);
    entity = ds_slist_get_element(&e_entities, index);

    entity->node = e_AllocNode(position, scale, orientation, entity);
    entity->transform = e_AllocTransform(entity);
    entity->index = index;
    entity->def = NULL;

    if(ent_def->model)
    {
        entity->model = e_AllocModel(ent_def->model, entity);
    }

    if(ent_def->children)
    {
        struct e_ent_def_t *child_def = ent_def->children;

        while(child_def)
        {
            struct e_entity_t *child_entity = e_SpawnEntityRecursive(child_def, &child_def->position, &child_def->scale, &child_def->orientation, entity);
            struct e_node_t *child_transform = child_entity->node;
            child_transform->parent = entity->node;

            child_transform->next = entity->node->children;
            if(entity->node->children)
            {
                entity->node->prev = child_transform;
            }
            entity->node->children = child_transform;
            child_def = child_def->next;
//            entity->node->child_count += 1 + child_transform->child_count;
        }
    }

    return entity;
}

struct e_entity_t *e_SpawnEntity(struct e_ent_def_t *ent_def, vec3_t *position, vec3_t *scale, mat3_t *orientation)
{
    struct e_entity_t *entity = e_SpawnEntityRecursive(ent_def, position, scale, orientation, NULL);
    entity->node->root_index = ds_list_add_element(&e_root_transforms, &entity->node);

    entity->node->scale.x *= ent_def->scale.x;
    entity->node->scale.y *= ent_def->scale.y;
    entity->node->scale.z *= ent_def->scale.z;

    if(ent_def->type == E_ENT_DEF_TYPE_ROOT)
    {
        entity->def = ent_def;
    }

    if(ent_def->collider.shape_count)
    {
        entity->collider = e_AllocCollider(&ent_def->collider, entity);
    }

    e_UpdateEntityNode(entity->node, &mat4_t_c_id());

    return entity;
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

void e_TranslateEntity(struct e_entity_t *entity, vec3_t *translation)
{
    if(entity && entity->index != 0xffffffff)
    {
        if(entity->collider)
        {
            p_TranslateCollider(entity->collider->collider, translation);
        }

        vec3_t_add(&entity->node->position, &entity->node->position, translation);
    }
}

void e_RotateEntity(struct e_entity_t *entity, mat3_t *rotation)
{
    if(entity && entity->index != 0xffffffff)
    {
        if(entity->collider)
        {
            p_RotateCollider(entity->collider->collider, rotation);
        }

        mat3_t_mul(&entity->node->orientation, &entity->node->orientation, rotation);
    }
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
    mat4_t_mul(&transform->transform, &transform->transform, parent_transform);

    struct e_node_t *child = local_transform->children;

    while(child)
    {
        e_UpdateEntityNode(child, &transform->transform);
        child = child->next;
    }
}

void e_UpdateEntities()
{
    for(uint32_t collider_index = 0; collider_index < e_components[E_COMPONENT_TYPE_COLLIDER].cursor; collider_index++)
    {
        struct e_collider_t *collider = (struct e_collider_t *)e_GetComponent(E_COMPONENT_TYPE_COLLIDER, collider_index);
        struct e_node_t *transform = collider->entity->node;
        transform->orientation = collider->collider->orientation;
        transform->position = collider->collider->position;
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















