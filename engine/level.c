#include "level.h"
#include "dstuff/ds_dbvt.h"
#include "dstuff/ds_alloc.h"
#include "../editor/ed_level_defs.h"
#include "r_draw.h"
#include "ent.h"
#include "phys.h"

struct r_model_t *l_world_model;
struct ds_dbvn_t l_world_dbvt;
struct p_tmesh_shape_t *l_world_shape;
struct p_collider_t *l_world_collider;

struct l_player_record_t l_player_record;

extern struct ds_slist_t r_lights;

void l_Init()
{

}

void l_Shutdown()
{

}

void l_InitGeometry(struct r_model_geometry_t *geometry)
{
//    w_world_model = r_CreateModel(geometry, NULL);
}

void l_ClearGeometry()
{
//    r_DestroyModel(w_world_model);
//    w_world_model = NULL;
}

void l_ClearLevel()
{
    r_DestroyAllLighs();
    e_DestroyAllEntities();
}

void l_DeserializeLevel(void *level_buffer, size_t buffer_size)
{
    char *in_buffer = level_buffer;
    struct ed_level_section_t *level_section = (struct ed_level_section_t *)in_buffer;
    struct l_light_section_t *light_section = (struct l_light_section_t *)(in_buffer + level_section->light_section_start);
    struct l_light_record_t *light_records = (struct l_light_record_t *)(in_buffer + light_section->record_start);

    for(uint32_t record_index = 0; record_index < light_section->record_count; record_index++)
    {
        struct l_light_record_t *record = light_records + record_index;
        struct r_light_t *light = r_CreateLight(record->type, &record->position, &record->color, record->radius, record->energy);
        record->d_index = light->index;
    }



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









