#include "level.h"
#include "dstuff/ds_dbvt.h"
#include "dstuff/ds_alloc.h"
#include "../editor/ed_level_defs.h"
#include "r_draw.h"
#include "ent.h"
#include "phys.h"

struct r_model_t *l_world_model;
struct ds_dbvn_t l_world_dbvt;
struct p_shape_def_t *l_world_shape;
struct p_collider_t *l_world_collider;
struct p_col_def_t l_world_col_def;

struct l_player_record_t l_player_record;

//extern struct ds_slist_t r_lights;

void l_Init()
{
    l_world_shape = p_AllocShapeDef();
    l_world_shape->type = P_COL_SHAPE_TYPE_ITRI_MESH;

    l_world_col_def.shape_count = 1;
    l_world_col_def.shape = l_world_shape;
    l_world_col_def.type = P_COLLIDER_TYPE_STATIC;
    l_world_col_def.mass = 0.0;
}

void l_Shutdown()
{

}

void l_DestroyWorld()
{
    if(l_world_collider)
    {
        p_DestroyCollider(l_world_collider);
        mem_Free(l_world_shape->itri_mesh.indices);
        mem_Free(l_world_shape->itri_mesh.verts);
        l_world_collider = NULL;
    }

    if(l_world_model)
    {
        r_DestroyModel(l_world_model);
        l_world_model = NULL;
    }
}

void l_ClearLevel()
{
    r_DestroyAllLighs();
    e_DestroyAllEntities();
    l_DestroyWorld();
}

void l_DeserializeLevel(void *level_buffer, size_t buffer_size, uint32_t data_flags)
{
    char *in_buffer = level_buffer;
    struct ed_level_section_t *level_section = (struct ed_level_section_t *)in_buffer;

    if(data_flags & L_LEVEL_DATA_LIGHTS)
    {
        struct l_light_section_t *light_section = (struct l_light_section_t *)(in_buffer + level_section->light_section_start);
        struct l_light_record_t *light_records = (struct l_light_record_t *)(in_buffer + light_section->record_start);

        for(uint32_t record_index = 0; record_index < light_section->record_count; record_index++)
        {
            struct l_light_record_t *record = light_records + record_index;
            struct r_light_t *light = r_CreateLight(record->type, &record->position, &record->color, record->radius, record->energy);
            record->d_index = light->index;
        }
    }


    if(data_flags & L_LEVEL_DATA_ENTITIES)
    {
        struct l_ent_def_section_t *ent_def_section = (struct l_ent_def_section_t *)(in_buffer + level_section->ent_def_section_start);
        struct l_ent_def_record_t *ent_def_records = (struct l_ent_def_record_t *)(in_buffer + ent_def_section->record_start);

        for(uint32_t record_index = 0; record_index < ent_def_section->record_count; record_index++)
        {
            struct l_ent_def_record_t *record = ent_def_records + record_index;

            record->def = e_FindEntDef(E_ENT_DEF_TYPE_ROOT, record->name);

            if(!record->def)
            {
                record->def = e_LoadEntDef(record->file);
            }
        }

        struct l_entity_section_t *entity_section = (struct l_entity_section_t *)(in_buffer + level_section->entity_section_start);
        struct l_entity_record_t *entity_records = (struct l_entity_record_t *)(in_buffer + entity_section->record_start);

        for(uint32_t record_index = 0; record_index < entity_section->record_count; record_index++)
        {
            struct l_entity_record_t *record = entity_records + record_index;
            struct e_ent_def_t *ent_def = ent_def_records[record->ent_def].def;
            struct e_entity_t *entity = e_SpawnEntity(ent_def, &record->position, &record->scale, &record->orientation);
            record->d_index = entity->index;
        }
    }

    if(data_flags & L_LEVEL_DATA_WORLD)
    {
        struct l_world_section_t *world_section = (struct l_world_section_t *)(in_buffer + level_section->world_section_start);
    }
}









