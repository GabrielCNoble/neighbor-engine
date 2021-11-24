#include "ed_ent.h"
#include "../engine/ent.h"
#include "../lib/dstuff/ds_vector.h"
#include "../lib/dstuff/ds_matrix.h"
#include "../lib/dstuff/ds_mem.h"
#include "../engine/r_draw.h"
#include "../engine/input.h"
#include "ed_level.h"
#include <stddef.h>
#include <math.h>

struct ed_entity_state_t ed_entity_state;
extern mat4_t r_camera_matrix;

void ed_EntityEditorInit(struct ed_editor_t *editor)
{
    editor->next_state = ed_EntityEditorIdle;

    ed_entity_state.camera_pitch = -0.1;
    ed_entity_state.camera_yaw = 0.2;
    ed_entity_state.camera_zoom = 15.0;
    ed_entity_state.camera_offset = vec3_t_c(0.0, 0.0, 0.0);
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
    r_SetViewPitchYaw(ed_entity_state.camera_pitch, ed_entity_state.camera_yaw);
    vec3_t forward_vec = r_camera_matrix.rows[2].xyz;
    vec3_t_mul(&forward_vec, &forward_vec, ed_entity_state.camera_zoom);
    vec3_t_add(&forward_vec, &forward_vec, &ed_entity_state.camera_offset);
    r_SetViewPos(&forward_vec);
    ed_w_DrawGrid();
}

void ed_EntityEditorReset()
{

}

void ed_EntityEditorIdle(uint32_t just_changed)
{
    if(in_GetMouseButtonState(SDL_BUTTON_RIGHT) & IN_KEY_STATE_PRESSED)
    {
        ed_SetNextState(ed_EntityEditorFlyCamera);
    }
}

void ed_EntityEditorFlyCamera(uint32_t just_changed)
{
    if(!(in_GetMouseButtonState(SDL_BUTTON_RIGHT) & IN_KEY_STATE_PRESSED))
    {
        in_SetMouseWarp(0);
        ed_SetNextState(ed_EntityEditorIdle);
    }
    else
    {
        in_SetMouseWarp(1);
        float dx;
        float dy;
        in_GetMouseDelta(&dx, &dy);

        if(in_GetKeyState(SDL_SCANCODE_LCTRL) & IN_KEY_STATE_PRESSED)
        {
            if(ed_entity_state.camera_zoom)
            {
                ed_entity_state.camera_zoom -= dy * ed_entity_state.camera_zoom;
            }
            else
            {
                ed_entity_state.camera_zoom -= dy;
            }

            if(ed_entity_state.camera_zoom < 0.05)
            {
                ed_entity_state.camera_zoom = 0.05;
            }
        }
        else if(in_GetKeyState(SDL_SCANCODE_LSHIFT) & IN_KEY_STATE_PRESSED)
        {
            float zoom = ed_entity_state.camera_zoom;
            vec3_t_fmadd(&ed_entity_state.camera_offset, &ed_entity_state.camera_offset, &r_camera_matrix.rows[0].xyz, -dx * zoom);
            vec3_t_fmadd(&ed_entity_state.camera_offset, &ed_entity_state.camera_offset, &r_camera_matrix.rows[1].xyz, -dy * zoom);
        }
        else
        {
            ed_entity_state.camera_yaw -= dx;
            ed_entity_state.camera_pitch += dy;

            ed_entity_state.camera_pitch = fminf(fmaxf(ed_entity_state.camera_pitch, -0.5), 0.5);
        }
    }
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
