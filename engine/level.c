#include "level.h"
#include "dstuff/ds_dbvt.h"
#include "dstuff/ds_alloc.h"
#include "../editor/ed_level_defs.h"
#include "r_draw.h"

struct r_model_t *w_world_model;
struct ds_dbvn_t w_world_dbvt;

void l_Init()
{

}

void l_Shutdown()
{

}

void l_InitGeometry(struct r_model_geometry_t *geometry)
{
    w_world_model = r_CreateModel(geometry, NULL);
}

void l_ClearGeometry()
{
    r_DestroyModel(w_world_model);
    w_world_model = NULL;
}

void l_DeserializeLevel(void *level_buffer, size_t buffer_size)
{
    char *in_buffer = level_buffer;
    struct ed_level_section_t *level_section = (struct ed_level_section_t *)in_buffer;
    struct l_light_section_t *light_section = in_buffer + level_section->light_section_start;
    struct l_light_record_t *light_records = in_buffer + light_section->light_record_start;

    for(uint32_t record_index = 0; record_index < light_section->light_record_count; record_index++)
    {
        struct l_light_record_t *record = light_records + record_index;
        r_CreateLight(record->type, &record->position, &record->color, record->radius, record->energy);
    }
}
