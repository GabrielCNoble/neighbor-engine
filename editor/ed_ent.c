#include "ed_ent.h"
#include "../engine/ent.h"
#include "../lib/dstuff/ds_vector.h"
#include "../lib/dstuff/ds_matrix.h"
#include "../lib/dstuff/ds_mem.h"
#include "../engine/r_draw.h"
#include <stddef.h>

void ed_EntityEditorInit(struct ed_editor_t *editor)
{
    editor->next_state = ed_EntityEditorIdle;
}

void ed_EntityEditorShutdown()
{

}

void ed_EntityEditorSuspend()
{

}

void ed_EntityEditorResume()
{
    r_SetClearColor(0.4, 0.4, 0.8, 1.0);
}

void ed_EntityEditorUpdate()
{

}

void ed_EntityEditorReset()
{

}

void ed_EntityEditorIdle(uint32_t just_changed)
{

}

uint32_t ed_EntDefRecordCount(struct e_ent_def_t *ent_def)
{
    uint32_t count = 1;
    struct e_ent_def_t *child = ent_def->children;

    while(child)
    {
        count += ed_EntDefRecordCount(child);
        child = child->next;
    }

    return count;
}

void ed_SerializeEntDefRecursive(struct e_ent_def_t *ent_def, struct e_ent_def_section_t *section, struct e_ent_def_record_t *records)
{
    struct e_ent_def_record_t *record = records + section->record_count;
    section->record_count++;

    record->orientation = ent_def->orientation;
    record->position = ent_def->position;
    record->scale = ent_def->scale;
    record->collider_start = 0;
    record->child_start = 0;

    if(ent_def->model)
    {
        strncpy(record->model, ent_def->model->name, sizeof(record->model));
    }

    struct e_ent_def_t *child_def = ent_def->children;
    while(child_def)
    {
        ed_SerializeEntDefRecursive(child_def, section, records);
        child_def = child_def->next;
        record->child_count++;
    }
}

void ed_SerializeEntDef(void **buffer, size_t *buffer_size, struct e_ent_def_t *ent_def)
{
    char *start_out_buffer;
    size_t out_size = 0;
    uint32_t record_count = ed_EntDefRecordCount(ent_def);

    out_size = sizeof(struct e_ent_def_section_t);
    out_size += sizeof(struct e_ent_def_record_t) * record_count;

    if(ent_def->collider.shape_count)
    {
        out_size += sizeof(struct p_col_def_record_t);
        out_size += sizeof(struct p_shape_data_t) * ent_def->collider.shape_count;
    }

    start_out_buffer = mem_Calloc(1, out_size);
    *buffer = start_out_buffer;
    *buffer_size = out_size;

    char *cur_out_buffer = start_out_buffer;

    struct e_ent_def_section_t *ent_def_section = (struct e_ent_def_section_t *)cur_out_buffer;
    cur_out_buffer += sizeof(struct e_ent_def_section_t);
    ent_def_section->record_start = cur_out_buffer - start_out_buffer;

    struct e_ent_def_record_t *ent_def_records = (struct e_ent_def_record_t *)cur_out_buffer;
    cur_out_buffer += sizeof(struct e_ent_def_record_t) * record_count;

    ed_SerializeEntDefRecursive(ent_def, ent_def_section, ent_def_records);

    if(ent_def->collider.shape_count)
    {
        ent_def_records->collider_start = cur_out_buffer - start_out_buffer;

    }
}

void ed_SaveEntDef(char *file_name, struct e_ent_def_t *ent_def)
{




}
