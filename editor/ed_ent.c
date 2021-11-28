#include "ed_ent.h"
#include "../engine/ent.h"
#include "../engine/p_defs.h"
#include "../lib/dstuff/ds_vector.h"
#include "../lib/dstuff/ds_matrix.h"
#include "../lib/dstuff/ds_mem.h"
#include "../engine/r_draw.h"
#include "../engine/input.h"
#include "../engine/gui.h"
#include "../engine/phys.h"
#include "ed_level.h"
#include "ed_main.h"
#include <stddef.h>
#include <math.h>

struct ed_entity_state_t ed_entity_state;
extern mat4_t r_camera_matrix;

extern uint32_t r_width;
extern uint32_t r_height;
extern struct ds_slist_t e_ent_defs[];
extern struct ds_slist_t r_models;

extern char *p_col_shape_names[];

void ed_e_Init(struct ed_editor_t *editor)
{
    editor->next_state = ed_EntityEditorIdle;

    ed_entity_state.camera_pitch = -0.1;
    ed_entity_state.camera_yaw = 0.2;
    ed_entity_state.camera_zoom = 15.0;
    ed_entity_state.camera_offset = vec3_t_c(0.0, 0.0, 0.0);
}

void ed_e_Shutdown()
{

}

void ed_e_Suspend()
{
    if(ed_entity_state.cur_entity)
    {
        e_DestroyEntity(ed_entity_state.cur_entity);
    }

    r_DestroyLight((struct r_light_t *)ed_entity_state.light);
}

void ed_e_Resume()
{
    r_SetClearColor(0.4, 0.4, 0.8, 1.0);

    if(ed_entity_state.cur_ent_def && ed_entity_state.cur_ent_def->index != 0xffffffff)
    {
        struct e_ent_def_t *ent_def = ed_entity_state.cur_ent_def;
        ed_entity_state.cur_entity = e_SpawnEntity(ent_def, &vec3_t_c(0.0, 0.0, 0.0), &vec3_t_c(1.0, 1.0, 1.0), &mat3_t_c_id());
    }

    ed_entity_state.light = r_CreatePointLight(&vec3_t_c(0.0, 0.0, 0.0), &vec3_t_c(1.0, 1.0, 1.0), 20.0, 5.0);
}

uint32_t ed_e_SaveEntDef(char *path, char *file)
{
    return 0;
}

uint32_t ed_e_LoadEntDef(char *path, char *file)
{
    return 0;
}

uint32_t ed_e_EntDefHierarchyUI(struct e_ent_def_t *ent_def)
{
    uint32_t refresh_entity = 0;

    if(igTreeNode_Ptr(ent_def, ""))
    {
        igSeparator();
        igText("Transform");

        refresh_entity |= igInputFloat3("Position", ent_def->position.comps, "%0.2f", 0);

        refresh_entity |= igInputFloat3("Scale", ent_def->scale.comps, "%0.2f", 0);

        igSeparator();
        igText("Model");
        char *model_name = "None";
        if(ent_def->model)
        {
            struct r_model_t *ent_model = ent_def->model;
            igSameLine(0, -1.0);
            if(igButton("Remove##model", (ImVec2){0, 0}))
            {
                ent_def->model = NULL;
                refresh_entity = 1;
            }
            model_name = ent_model->name;
        }

        if(igBeginCombo("##model", model_name, 0))
        {
            for(uint32_t model_index = 0; model_index < r_models.cursor; model_index++)
            {
                struct r_model_t *model = r_GetModel(model_index);

                if(model)
                {
                    if(igSelectable_Bool(model->name, 0, 0, (ImVec2){0, 0}))
                    {
                        ent_def->model = model;
                        refresh_entity = 1;
                    }
                }
            }
            igEndCombo();
        }

        igSeparator();
        igText("Collider");

        if(ent_def->collider.shape_count)
        {
            igSameLine(0, -1);
            if(igButton("Remove##collider", (ImVec2){0, 0}))
            {
                refresh_entity = 1;
                struct p_shape_def_t *shape = ent_def->collider.shape;
                ent_def->collider.shape_count = 0;
                ent_def->collider.shape = NULL;

                while(shape)
                {
                    struct p_shape_def_t *next_shape = shape->next;
                    p_FreeShapeDef(shape);
                    shape = next_shape;
                }
            }
        }

        if(igButton("Add shape", (ImVec2){0, 0}))
        {
            struct p_shape_def_t *new_shape = p_AllocShapeDef(0);
            new_shape->type = P_COL_SHAPE_TYPE_BOX;
            new_shape->box.size = vec3_t_c(1.0, 1.0, 1.0);
            new_shape->position = vec3_t_c(0.0, 0.0, 0.0);
            new_shape->orientation = mat3_t_c_id();

            new_shape->next = ent_def->collider.shape;
            ent_def->collider.shape = new_shape;
            ent_def->collider.shape_count++;

            refresh_entity = 1;
        }

        refresh_entity |= ed_e_CollisionShapeUI(&ent_def->collider);
//        if(ent_def->collider.shape_count)
//        {
//            refresh_entity |= ed_e_CollisionShapeUI(&ent_def->collider);
//        }

        struct e_ent_def_t *child = ent_def->children;

        while(child)
        {
            refresh_entity |= ed_e_EntDefHierarchyUI(child);
            child = child->next;
        }

        igTreePop();
    }

    return refresh_entity;
}

uint32_t ed_e_CollisionShapeUI(struct p_col_def_t *col_def)
{
    uint32_t refresh_shape = 0;
    struct p_shape_def_t *shape = col_def->shape;

    mat4_t shape_transform;

    while(shape)
    {
        igPushID_Ptr(shape);

        if(igBeginCombo("Shape type", p_col_shape_names[shape->type], 0))
        {
            for(uint32_t shape_type = P_COL_SHAPE_TYPE_CAPSULE; shape_type < P_COL_SHAPE_TYPE_LAST; shape_type++)
            {
                if(igSelectable_Bool(p_col_shape_names[shape_type], 0, 0, (ImVec2){0, 0}))
                {
                    shape->type = shape_type;
                    refresh_shape = 1;
                }
            }
            igEndCombo();
        }

        refresh_shape |= igInputFloat3("Position offset", shape->position.comps, "%.2f", 0);
        mat4_t_comp(&shape_transform, &shape->orientation, &shape->position);
        r_i_SetModelMatrix(&shape_transform);

        switch(shape->type)
        {
            case P_COL_SHAPE_TYPE_BOX:
                r_i_DrawBox(&shape->box.size);
                refresh_shape |= igInputFloat3("Half-size", shape->box.size.comps, "%0.2f", 0);
            break;

            case P_COL_SHAPE_TYPE_CYLINDER:
                r_i_DrawCylinder(shape->cylinder.radius, shape->cylinder.height);
            break;

            case P_COL_SHAPE_TYPE_CAPSULE:
                refresh_shape |= igInputFloat("Height", &shape->capsule.height, 0.0, 0.0, "%0.2f", 0);
                refresh_shape |= igInputFloat("Radius", &shape->capsule.radius, 0.0, 0.0, "%0.2f", 0);
                r_i_DrawCapsule(shape->capsule.radius, shape->capsule.height);
            break;
        }

        igPopID();
        igNewLine();

        shape = shape->next;
    }
}

void ed_e_UpdateUI()
{
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize;
    window_flags |= ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar;

    if(ed_entity_state.cur_ent_def)
    {
        r_i_SetViewProjectionMatrix(NULL);
        r_i_SetModelMatrix(NULL);
        r_i_SetShader(NULL);
        struct e_ent_def_t *ent_def = ed_entity_state.cur_ent_def;
        igSetNextWindowPos((ImVec2){r_width, 40}, 0, (ImVec2){1, 0});
        igSetNextWindowSize((ImVec2){350, 0}, 0);
        if(igBegin("##ent_defs_data", NULL, 0))
        {
            igInputText("Name", ent_def->name, sizeof(ent_def->name), 0, 0, NULL);
            igText("Total child nodes: %d", ent_def->children_count);

            igSeparator();
            igText("Hierarchy");
            igSeparator();

            if(ed_e_EntDefHierarchyUI(ent_def))
            {
                e_DestroyEntity(ed_entity_state.cur_entity);
                ed_entity_state.cur_entity = e_SpawnEntity(ent_def, &vec3_t_c(0.0, 0.0, 0.0), &vec3_t_c(1.0, 1.0, 1.0), &mat3_t_c_id());
            }
        }
        igEnd();
    }

    if(ed_entity_state.ent_def_window_open)
    {
//        igSetNextWindowSizeConstraints((ImVec2){200, 200})
        igSetNextWindowPos((ImVec2){0, 40}, 0, (ImVec2){0, 0});
        if(igBegin("Ent defs", NULL, 0))
        {
            if(igSelectable_Bool("New", 0, 0, (ImVec2){50, 50}))
            {
                struct e_ent_def_t *ent_def = e_AllocEntDef(E_ENT_DEF_TYPE_ROOT);
                ed_e_SelectEntDef(ent_def);
                ed_entity_state.ent_def_window_open = 0;
            }
            for(uint32_t ent_def_index = 0; ent_def_index < e_ent_defs[E_ENT_DEF_TYPE_ROOT].cursor; ent_def_index++)
            {
                struct e_ent_def_t *ent_def = e_GetEntDef(E_ENT_DEF_TYPE_ROOT, ent_def_index);

                igSelectable_Bool(ent_def->name, 0, 0, (ImVec2){50, 50});
                if(igIsItemClicked(ImGuiMouseButton_Left))
                {
                    if(igIsMouseDoubleClicked(ImGuiMouseButton_Left))
                    {
                        ed_e_SelectEntDef(ent_def);
                        ed_entity_state.ent_def_window_open = 0;
                    }
                }
            }
        }
        igEnd();
    }

    igSetNextWindowBgAlpha(0.5);
    igPushStyleVar_Float(ImGuiStyleVar_WindowBorderSize, 0.0);
    igSetNextWindowSize((ImVec2){r_width, 0}, 0);
    igSetNextWindowPos((ImVec2){0.0, r_height}, 0, (ImVec2){0, 1});
    if(igBegin("Footer window", NULL, window_flags))
    {
        char label[32];
        sprintf(label, "Ent defs (%d)", e_ent_defs[E_ENT_DEF_TYPE_ROOT].used);
        if(igSelectable_Bool(label, ed_entity_state.ent_def_window_open, 0, (ImVec2){100, 0}))
        {
            ed_entity_state.ent_def_window_open = !ed_entity_state.ent_def_window_open;
        }
    }
    igEnd();
    igPopStyleVar(1);
}

void ed_e_Update()
{
    ed_e_UpdateUI();

    r_SetViewPitchYaw(ed_entity_state.camera_pitch, ed_entity_state.camera_yaw);
    vec3_t forward_vec = r_camera_matrix.rows[2].xyz;
    vec3_t_mul(&forward_vec, &forward_vec, ed_entity_state.camera_zoom);
    vec3_t_add(&forward_vec, &forward_vec, &ed_entity_state.camera_offset);
    r_SetViewPos(&forward_vec);

    ed_entity_state.light->position = forward_vec;
    ed_LevelEditorDrawGrid();
}

void ed_e_ResetEditor()
{

}

void ed_e_SelectEntDef(struct e_ent_def_t *ent_def)
{
    if(ent_def && ent_def != ed_entity_state.cur_ent_def)
    {
        if(ed_entity_state.cur_entity)
        {
            e_DestroyEntity(ed_entity_state.cur_entity);
        }

        ed_entity_state.cur_ent_def = ent_def;
        ed_entity_state.cur_entity = e_SpawnEntity(ent_def, &vec3_t_c(0.0, 0.0, 0.0), &vec3_t_c(1.0, 1.0, 1.0), &mat3_t_c_id());
    }
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
            vec3_t *camera_offset = &ed_entity_state.camera_offset;
            vec3_t_fmadd(camera_offset, camera_offset, &r_camera_matrix.rows[0].xyz, -dx * zoom);
            vec3_t_fmadd(camera_offset, camera_offset, &r_camera_matrix.rows[1].xyz, -dy * zoom);
        }
        else
        {
            ed_entity_state.camera_yaw -= dx;
            ed_entity_state.camera_pitch += dy;
            ed_entity_state.camera_pitch = fminf(fmaxf(ed_entity_state.camera_pitch, -0.5), 0.5);
        }
    }
}






//uint32_t ed_EntDefRecordCount(struct e_ent_def_t *ent_def)
//{
//    uint32_t count = 1;
//    struct e_ent_def_t *child = ent_def->children;
//
//    while(child)
//    {
//        count += ed_EntDefRecordCount(child);
//        child = child->next;
//    }
//
//    return count;
//}

void ed_e_SerializeEntDefRecursive(char *start_out_buffer, char **out_buffer, struct e_ent_def_t *ent_def)
{
    char *cur_out_buffer = *out_buffer;
    struct e_ent_def_record_t *record = (struct e_ent_def_record_t *)cur_out_buffer;
    cur_out_buffer += sizeof(struct e_ent_def_record_t);

    if(ent_def->collider.shape_count)
    {
        record->collider_start = cur_out_buffer - start_out_buffer;

        struct p_col_def_record_t *collider_record = (struct p_col_def_record_t *)cur_out_buffer;
//        collider_record->shape_count = ent_def->collider.shape_count;
        collider_record->type = ent_def->collider.type;
        collider_record->mass = ent_def->collider.mass;
        cur_out_buffer += sizeof(struct p_col_def_record_t) + sizeof(struct p_shape_def_fields_t) * ent_def->collider.shape_count;

        struct p_shape_def_t *shape = ent_def->collider.shape;

        while(shape)
        {
//            struct p_shape_data_t *shape_data = collider_record->shape + collider_record->shape_count;
//            collider_record->shape_count++;
//            shape_data->type = shape->type;
//            shape = shape->next;
        }
    }

//    struct e_ent_def_record_t *record = records + section->record_count;
//    section->record_count++;
//
//    record->orientation = ent_def->orientation;
//    record->position = ent_def->position;
//    record->scale = ent_def->scale;
//    record->shape_start = 0;
//    record->child_start = 0;
//
//    if(ent_def->model)
//    {
//        strncpy(record->model, ent_def->model->name, sizeof(record->model));
//    }
//
//    struct e_ent_def_t *child_def = ent_def->children;
//    while(child_def)
//    {
//        ed_SerializeEntDefRecursive(child_def, section, records);
//        child_def = child_def->next;
//        record->child_count++;
//    }
}

void ed_e_SerializeEntDef(void **buffer, size_t *buffer_size, struct e_ent_def_t *ent_def)
{
    char *start_out_buffer;
    size_t out_size = 0;
    size_t record_count = 1 + ent_def->children_count;
    out_size = sizeof(struct e_ent_def_section_t);
    out_size += sizeof(struct e_ent_def_record_t) * record_count;

    if(ent_def->collider.shape_count)
    {
        out_size += sizeof(struct p_col_def_record_t);
        out_size += sizeof(struct p_shape_def_fields_t) * ent_def->collider.shape_count;
    }

    start_out_buffer = mem_Calloc(1, out_size);
    *buffer = start_out_buffer;
    *buffer_size = out_size;

    char *cur_out_buffer = start_out_buffer;

    struct e_ent_def_section_t *ent_def_section = (struct e_ent_def_section_t *)cur_out_buffer;
    cur_out_buffer += sizeof(struct e_ent_def_section_t);
    ent_def_section->record_start = cur_out_buffer - start_out_buffer;
    ent_def_section->record_count = record_count;

    ed_e_SerializeEntDefRecursive(start_out_buffer, &cur_out_buffer, ent_def);

//    struct e_ent_def_record_t *ent_def_records = (struct e_ent_def_record_t *)cur_out_buffer;
//    cur_out_buffer += sizeof(struct e_ent_def_record_t) * record_count;

//    ed_SerializeEntDefRecursive(ent_def, ent_def_section, ent_def_records);
}

//void ed_SaveEntDef(char *file_name, struct e_ent_def_t *ent_def)
//{
//
//
//
//
//}
