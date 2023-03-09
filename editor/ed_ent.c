#include "ed_ent.h"
#include "../engine/ent.h"
#include "../engine/p_defs.h"
#include "../lib/dstuff/ds_vector.h"
#include "../lib/dstuff/ds_matrix.h"
#include "../lib/dstuff/ds_mem.h"
#include "../lib/dstuff/ds_path.h"
#include "../engine/r_draw.h"
#include "../engine/input.h"
#include "../engine/gui.h"
#include "../engine/phys.h"
#include "level.h"
#include "ed_main.h"
#include <stddef.h>
#include <math.h>

struct ed_entity_state_t ed_entity_state;
extern struct ed_level_state_t ed_level_state;
extern mat4_t r_camera_matrix;

extern uint32_t r_width;
extern uint32_t r_height;
extern struct ds_slist_t e_ent_defs[];
extern struct ds_slist_t r_models;

extern char *p_col_shape_names[];
extern char *p_constraint_names[];

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
//    ed_entity_state.prev_debug_flags = r_renderer_state.
}

uint32_t ed_e_SaveEntDef(char *path, char *file)
{
    void *buffer;
    size_t buffer_size;
    char file_name[PATH_MAX];

    ed_e_SerializeEntDef(&buffer, &buffer_size, ed_entity_state.cur_ent_def);

    ds_path_append_end(path, file, file_name, PATH_MAX);
    ds_path_set_ext(file_name, "ent", file_name, PATH_MAX);

    FILE *fp = fopen(file_name, "wb");
    fwrite(buffer, buffer_size, 1, fp);
    fclose(fp);

    if(!ed_entity_state.cur_ent_def->name[0])
    {
        ds_path_drop_ext(file, ed_entity_state.cur_ent_def->name, sizeof(ed_entity_state.cur_ent_def->name));
    }

    return 1;
}

uint32_t ed_e_LoadEntDef(char *path, char *file)
{
    char file_name[PATH_MAX];

    if(strstr(file, ".ent"))
    {
        ds_path_append_end(path, file, file_name, PATH_MAX);
        e_LoadEntDef(file_name);
    }

    return 0;
}

void ed_e_OpenExplorerSave(struct ed_explorer_state_t *explorer_state)
{
    if(ed_level_state.project.base_folder[0])
    {
        ds_path_append_end(ed_level_state.project.base_folder, "entities", explorer_state->current_path, PATH_MAX);

        if(ed_entity_state.cur_ent_def && ed_entity_state.cur_ent_def->name[0])
        {
            strcpy(explorer_state->current_file, ed_entity_state.cur_ent_def->name);
            ds_path_set_ext(explorer_state->current_file, "ent", explorer_state->current_file, PATH_MAX);
        }
        else
        {
            explorer_state->current_file[0] = '\0';
        }
    }
}

void ed_e_OpenExplorerLoad(struct ed_explorer_state_t *explorer_state)
{
    if(ed_level_state.project.base_folder[0])
    {
        ds_path_append_end(ed_level_state.project.base_folder, "entities", explorer_state->current_path, PATH_MAX);
    }
}

void ed_e_AddCollisionShape(struct e_ent_def_t *ent_def)
{
    struct p_shape_def_t *new_shape = p_AllocShapeDef();
    new_shape->type = P_COL_SHAPE_TYPE_BOX;
    new_shape->box.size = vec3_t_c(1.0, 1.0, 1.0);
    new_shape->position = vec3_t_c(0.0, 0.0, 0.0);
    new_shape->orientation = mat3_t_c_id();

    new_shape->next = ent_def->collider.passive.shape;
    if(!ent_def->collider.passive.shape_count)
    {
        ent_def->collider.passive.mass = 1.0;
        ent_def->collider.type = P_COLLIDER_TYPE_DYNAMIC;
    }
    ent_def->collider.passive.shape = new_shape;
    ent_def->collider.passive.shape_count++;
}

void ed_e_RemoveCollisionShape(struct e_ent_def_t *ent_def, struct p_shape_def_t *shape_def)
{
    struct p_shape_def_t *prev_shape = ent_def->collider.passive.shape;

    if(prev_shape != shape_def)
    {
        while(prev_shape->next != shape_def)
        {
            prev_shape = prev_shape->next;
        }

        prev_shape->next = shape_def->next;
    }
    else
    {
        ent_def->collider.passive.shape = shape_def->next;
    }

    ent_def->collider.passive.shape_count--;
}

struct e_constraint_t *ed_e_GetConstraint(struct e_ent_def_t *child_def, struct e_ent_def_t *parent_def)
{
    struct e_constraint_t *constraint = parent_def->constraints;

    while(constraint)
    {
        if(constraint->child_def == child_def)
        {
            break;
        }

        constraint = constraint->next;
    }

    return constraint;
}

void ed_e_AddConstraint(struct e_ent_def_t *child_def, struct e_ent_def_t *parent_def)
{
    if(!ed_e_GetConstraint(child_def, parent_def))
    {
        struct e_constraint_t *constraint = e_AllocConstraint();

        constraint->child_def = child_def;
        constraint->next = parent_def->constraints;
        if(parent_def->constraints)
        {
            parent_def->constraints->prev = constraint;
        }
        parent_def->constraints = constraint;

        constraint->def.type = P_CONSTRAINT_TYPE_HINGE;
        constraint->def.axis = mat3_t_c_id();
        constraint->def.hinge.min_angle = -0.5;
        constraint->def.hinge.max_angle = 0.5;
        constraint->def.pivot_a = vec3_t_c(0.0, 0.0, 0.0);
        constraint->def.pivot_b = vec3_t_c(0.0, 0.0, 0.0);

//        parent_def->constraint_count++;
    }
}

void ed_e_RemoveConstraint(struct e_constraint_t *constraint, struct e_ent_def_t *parent_def)
{
    if(constraint && parent_def)
    {
        if(constraint->prev)
        {
            constraint->prev->next = constraint->next;
        }
        else
        {
            parent_def->constraints = constraint->next;
        }

        if(constraint->next)
        {
            constraint->next->prev = constraint->prev;
        }

        e_DeallocConstraint(constraint);

//        parent_def->constraint_count--;
    }
}

void ed_e_AddEntDefChild(struct e_ent_def_t *parent_def)
{
    struct e_ent_def_t *new_child = e_AllocEntDef(E_ENT_DEF_TYPE_CHILD);
    new_child->position = vec3_t_c(0.0, 0.0, 0.0);
    new_child->scale = vec3_t_c(1.0, 1.0, 1.0);
    new_child->orientation = mat3_t_c_id();
    new_child->node_count = 1;
    new_child->collider.type = P_COLLIDER_TYPE_LAST;
    new_child->collider.passive.shape_count = 0;

    new_child->next = parent_def->children;
    if(parent_def->children)
    {
        parent_def->children->prev = new_child;
    }
    parent_def->children = new_child;

//    parent_def->children_count++;
}

void ed_e_RemoveEntDefChild(struct e_ent_def_t *child_def, struct e_ent_def_t *parent_def)
{
    if(child_def->prev)
    {
        child_def->prev->next = child_def->next;
    }
    else
    {
        parent_def->children = child_def->next;
    }

    if(child_def->next)
    {
        child_def->next->prev = child_def->prev;
    }

    struct e_ent_def_t *child_child_def = child_def->children;

    while(child_child_def)
    {
        struct e_ent_def_t *next_def = child_child_def->next;
        ed_e_RemoveEntDefChild(child_child_def, child_def);
        child_child_def = next_def;
    }

    struct e_constraint_t *constraint = ed_e_GetConstraint(child_def, parent_def);

    if(constraint)
    {
        ed_e_RemoveConstraint(constraint, parent_def);
    }

    e_DeallocEntDef(child_def);

//    parent_def->children_count--;
}

uint32_t ed_e_EntDefHierarchyUI(struct e_ent_def_t *ent_def, struct e_ent_def_t *parent_def)
{
    uint32_t refresh_entity = 0;

    igPushID_Ptr(ent_def);
    uint32_t node_visible = igCollapsingHeader_BoolPtr("Entity", NULL, ImGuiTreeNodeFlags_AllowItemOverlap);

    if(parent_def)
    {
        igSameLine(0, -1.0);
        if(igButton("Remove", (ImVec2){0, 0}))
        {
            ed_e_RemoveEntDefChild(ent_def, parent_def);
            refresh_entity = 1;
        }
    }

    if(node_visible)
    {
        igSeparator();
        igText("Transform");
        igIndent(0.0);
        refresh_entity |= igInputFloat3("Position", ent_def->position.comps, "%0.6f", 0);
        refresh_entity |= igInputFloat3("Scale", ent_def->scale.comps, "%0.6f", 0);
        igUnindent(0.0);
        igNewLine();

        igSeparator();
        igText("Model");
        char *model_name = "None";
        igIndent(0.0);
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
        igUnindent(0.0);
        igNewLine();

        igSeparator();
        igText("Collider");
        ent_def->collider_count = 0;

        if(ent_def->collider.passive.shape_count)
        {
            ent_def->collider_count = 1;
            igSameLine(0, -1);

            struct e_constraint_t *constraint = NULL;
//
            if(parent_def)
            {
                constraint = ed_e_GetConstraint(ent_def, parent_def);
            }

            if(igButton("Remove##collider", (ImVec2){0, 0}))
            {
                refresh_entity = 1;
                struct p_shape_def_t *shape = ent_def->collider.passive.shape;
                ent_def->collider.passive.shape_count = 0;
                ent_def->collider.passive.shape = NULL;
                ent_def->collider.type = P_COLLIDER_TYPE_LAST;

                while(shape)
                {
                    struct p_shape_def_t *next_shape = shape->next;
                    p_FreeShapeDef(shape);
                    shape = next_shape;
                }

                ed_e_RemoveConstraint(constraint, parent_def);

                struct e_constraint_t *constraint = ent_def->constraints;
                while(constraint)
                {
                    struct e_constraint_t *next_constraint = constraint->next;
                    ed_e_RemoveConstraint(constraint, ent_def);
                    constraint = next_constraint;
                }
            }

            igNewLine();
            igIndent(0.0);

            refresh_entity |= igSliderFloat("Mass", &ent_def->collider.passive.mass, 0.0, 100.0, "%0.6f", 0);

            ent_def->constraint_count = 0;

            if(parent_def)
            {
                struct e_constraint_t *constraint = ed_e_GetConstraint(ent_def, parent_def);

                if(constraint)
                {
                    if(igButton("Remove constraint", (ImVec2){0, 0}))
                    {
                        refresh_entity = 1;
                        ed_e_RemoveConstraint(constraint, parent_def);
                    }

                    if(igBeginCombo("Constraint type", p_constraint_names[constraint->def.type], 0))
                    {
                        for(uint32_t type = P_CONSTRAINT_TYPE_HINGE; type < P_CONSTRAINT_TYPE_LAST; type++)
                        {
                            if(igSelectable_Bool(p_constraint_names[type], 0, 0, (ImVec2){0, 0}))
                            {
                                constraint->def.type = type;
                                refresh_entity = 1;
                            }
                        }
                        igEndCombo();
                    }

                    refresh_entity |= igInputFloat3("Pivot A", constraint->def.pivot_a.comps, "%0.6f", 0);
                    refresh_entity |= igInputFloat3("Pivot B", constraint->def.pivot_b.comps, "%0.6f", 0);

                    switch(constraint->def.type)
                    {
                        case P_CONSTRAINT_TYPE_HINGE:
                        {
                            refresh_entity |= igSliderAngle("Min angle", &constraint->def.hinge.min_angle, -180.0, 180.0, "%0.2f", 0);
                            refresh_entity |= igSliderAngle("Max limit", &constraint->def.hinge.max_angle, -180.0, 180.0, "%0.2f", 0);
                            refresh_entity |= igCheckbox("Use limits", &constraint->def.hinge.use_limits);
                            char preview[32];
                            vec3_t hinge_axis = constraint->def.axis.rows[0];
                            sprintf(preview, "[%f %f %f]", hinge_axis.x, hinge_axis.y, hinge_axis.z);

                            if(igBeginCombo("Axis", preview, 0))
                            {
                                if(igMenuItem_Bool("[1.0, 0.0, 0.0]", 0, 0, 1))
                                {
                                    constraint->def.axis.rows[0] = vec3_t_c(1.0, 0.0, 0.0);
                                    constraint->def.axis.rows[1] = vec3_t_c(0.0, 1.0, 0.0);
                                    constraint->def.axis.rows[2] = vec3_t_c(0.0, 0.0, 1.0);
                                    refresh_entity = 1;
                                }
                                if(igMenuItem_Bool("[0.0, 1.0, 0.0]", 0, 0, 1))
                                {
                                    constraint->def.axis.rows[1] = vec3_t_c(1.0, 0.0, 0.0);
                                    constraint->def.axis.rows[0] = vec3_t_c(0.0, 1.0, 0.0);
                                    constraint->def.axis.rows[2] = vec3_t_c(0.0, 0.0, 1.0);
                                    refresh_entity = 1;
                                }
                                if(igMenuItem_Bool("[0.0, 0.0, 1.0]", 0, 0, 1))
                                {
                                    constraint->def.axis.rows[2] = vec3_t_c(1.0, 0.0, 0.0);
                                    constraint->def.axis.rows[1] = vec3_t_c(0.0, 1.0, 0.0);
                                    constraint->def.axis.rows[0] = vec3_t_c(0.0, 0.0, 1.0);
                                    refresh_entity = 1;
                                }
                                igEndCombo();
                            }
                        }
                        break;

                        case P_CONSTRAINT_TYPE_SLIDER:
                        {
                            refresh_entity |= igInputFloat("Min dist", &constraint->def.slider.min_dist, 0.0, 0.0, "%0.6f", 0);
                            refresh_entity |= igInputFloat("Max dist", &constraint->def.slider.max_dist, 0.0, 0.0, "%0.6f", 0);
                            refresh_entity |= igCheckbox("Use limits", &constraint->def.slider.use_limits);
                        }
                        break;
                    }

                    ent_def->constraint_count = 1;
                }
                else
                {
                    if(igButton("Add constraint", (ImVec2){0, 0}))
                    {
                        if(parent_def->collider.passive.shape_count)
                        {
                            refresh_entity = 1;
                            ed_e_AddConstraint(ent_def, parent_def);
                            ent_def->constraint_count = 1;
                        }
                    }
                }
            }
        }
        else
        {
            igIndent(0.0);
        }

        igNewLine();

        if(igButton("Add shape", (ImVec2){0, 0}))
        {
            ed_e_AddCollisionShape(ent_def);
            refresh_entity = 1;
        }

        refresh_entity |= ed_e_CollisionShapeUI(ent_def);
        igUnindent(0.0);
        igNewLine();

        ent_def->node_count = 1;
        ent_def->shape_count = ent_def->collider.passive.shape_count;


        igSeparator();
        igText("Children");
        igIndent(0.0);
        if(igButton("Add child", (ImVec2){0, 0}))
        {
            ed_e_AddEntDefChild(ent_def);
            refresh_entity = 1;
        }
        struct e_ent_def_t *child_def = ent_def->children;
        while(child_def)
        {
            refresh_entity |= ed_e_EntDefHierarchyUI(child_def, ent_def);
            ent_def->node_count += child_def->node_count;
            ent_def->shape_count += child_def->shape_count;
            ent_def->constraint_count += child_def->constraint_count;
            ent_def->collider_count += child_def->collider_count;
            child_def = child_def->next;
        }

        igUnindent(0.0);
    }

    igPopID();

    return refresh_entity;
}

uint32_t ed_e_CollisionShapeUI(struct e_ent_def_t *ent_def)
{
    uint32_t refresh_shape = 0;
    struct p_col_def_t *col_def = &ent_def->collider;
    struct p_shape_def_t *shape = col_def->passive.shape;

    while(shape)
    {
        igPushID_Ptr(shape);

        if(igButton("Remove", (ImVec2){0, 0}))
        {
            ed_e_RemoveCollisionShape(ent_def, shape);
            refresh_shape = 1;
        }

        if(igBeginCombo("Shape type", p_col_shape_names[shape->type], 0))
        {
            for(uint32_t shape_type = P_COL_SHAPE_TYPE_CAPSULE; shape_type <= P_COL_SHAPE_TYPE_BOX; shape_type++)
            {
                if(igSelectable_Bool(p_col_shape_names[shape_type], 0, 0, (ImVec2){0, 0}))
                {
                    shape->type = shape_type;
                    refresh_shape = 1;
                }
            }
            igEndCombo();
        }

        refresh_shape |= igInputFloat3("Position offset", shape->position.comps, "%.6f", 0);

        switch(shape->type)
        {
            case P_COL_SHAPE_TYPE_BOX:
                refresh_shape |= igInputFloat3("Half-size", shape->box.size.comps, "%0.6f", 0);
            break;

            case P_COL_SHAPE_TYPE_CYLINDER:

            break;

            case P_COL_SHAPE_TYPE_CAPSULE:
                refresh_shape |= igInputFloat("Height", &shape->capsule.height, 0.0, 0.0, "%0.6f", 0);
                refresh_shape |= igInputFloat("Radius", &shape->capsule.radius, 0.0, 0.0, "%0.6f", 0);
            break;
        }

        igPopID();
        igNewLine();

        shape = shape->next;
    }

    return refresh_shape;
}

void ed_e_UpdateUI()
{
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize;
    window_flags |= ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar;

    if(ed_entity_state.cur_ent_def)
    {
//        r_i_SetViewProjectionMatrix(NULL);
//        r_i_SetModelMatrix(NULL);
//        r_i_SetShader(NULL);
        struct e_ent_def_t *ent_def = ed_entity_state.cur_ent_def;
        igSetNextWindowPos((ImVec2){r_width, 40}, ImGuiCond_Once, (ImVec2){1, 0});
        igSetNextWindowSize((ImVec2){450, r_height - 100}, ImGuiCond_Once);
        if(igBegin("##ent_defs_data", NULL, 0))
        {
            igInputText("Name", ent_def->name, sizeof(ent_def->name), 0, 0, NULL);
            igText("Nodes: %d", ent_def->node_count);
            igText("Constraints: %d", ent_def->constraint_count);
            igText("Colliders: %d\n", ent_def->collider_count);
            igText("Collision shapes: %d\n", ent_def->shape_count);

            igSeparator();
            igText("Hierarchy");
            igSeparator();

            if(ed_e_EntDefHierarchyUI(ent_def, NULL))
            {
                e_DestroyEntity(ed_entity_state.cur_entity);
                ed_entity_state.cur_entity = e_SpawnEntity(ent_def, &vec3_t_c(0.0, 0.0, 0.0), &vec3_t_c(1.0, 1.0, 1.0), &mat3_t_c_id());
            }
        }
        igEnd();
    }

    if(ed_entity_state.ent_def_window_open)
    {
        igSetNextWindowPos((ImVec2){0, 40}, 0, (ImVec2){0, 0});
        if(igBegin("Ent defs", NULL, 0))
        {
            if(igSelectable_Bool("New", 0, 0, (ImVec2){50, 50}))
            {
                struct e_ent_def_t *ent_def = e_AllocEntDef(E_ENT_DEF_TYPE_ROOT);
                ent_def->scale = vec3_t_c(1.0, 1.0, 1.0);
                ent_def->position = vec3_t_c(0.0, 0.0, 0.0);
                ent_def->orientation = mat3_t_c_id();
                ent_def->node_count = 1;
                ent_def->collider.type = P_COLLIDER_TYPE_LAST;

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
    ed_l_DrawGrid();

    p_DebugDrawPhysics();
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

    record->position = ent_def->position;
    record->orientation = ent_def->orientation;
    record->scale = ent_def->scale;
    record->record_size = (uint64_t)cur_out_buffer;

    cur_out_buffer += sizeof(struct e_ent_def_record_t);

    if(ent_def->constraints)
    {
        record->constraint_start = cur_out_buffer - start_out_buffer;
        struct e_constraint_t *constraint = ent_def->constraints;

        while(constraint)
        {
            cur_out_buffer += sizeof(struct e_constraint_record_t);
            constraint = constraint->next;
        }
    }

    if(ent_def->collider.type != P_COLLIDER_TYPE_LAST)
    {
        record->collider_start = cur_out_buffer - start_out_buffer;

        struct p_col_def_record_t *collider_record = (struct p_col_def_record_t *)cur_out_buffer;
        cur_out_buffer += sizeof(struct p_col_def_record_t);

        collider_record->type = ent_def->collider.type;

        if(ent_def->collider.type == P_COLLIDER_TYPE_CHARACTER)
        {
            collider_record->character.crouch_height = ent_def->collider.character.crouch_height;
            collider_record->character.step_height = ent_def->collider.character.step_height;
            collider_record->character.radius = ent_def->collider.character.radius;
            collider_record->character.height = ent_def->collider.character.height;
        }
        else
        {
            collider_record->passive.mass = ent_def->collider.passive.mass;
            collider_record->shape_start = cur_out_buffer - start_out_buffer;
            cur_out_buffer += sizeof(struct p_shape_def_fields_t) * ent_def->collider.passive.shape_count;

            struct p_shape_def_t *shape = ent_def->collider.passive.shape;
            while(shape)
            {
                collider_record->passive.shape[collider_record->passive.shape_count] = shape->fields;
                collider_record->passive.shape_count++;
                shape = shape->next;
            }
        }
    }

    if(ent_def->model)
    {
        strcpy(record->model, ent_def->model->name);
    }

    struct e_ent_def_t *child_def = ent_def->children;

    if(child_def)
    {
        record->child_start = cur_out_buffer - start_out_buffer;
        struct e_constraint_record_t *constraint_records = NULL;

        if(record->constraint_start)
        {
            constraint_records = (struct e_constraint_record_t *)(start_out_buffer + record->constraint_start);

            while(child_def)
            {
                struct e_constraint_t *constraint = ent_def->constraints;

                while(constraint)
                {
                    if(constraint->child_def == child_def)
                    {
                        struct e_constraint_record_t *constraint_record = constraint_records + record->constraint_count;
                        record->constraint_count++;

                        constraint_record->def = constraint->def;
                        constraint_record->child_index = record->child_count;

                        break;
                    }
                    constraint = constraint->next;
                }

                ed_e_SerializeEntDefRecursive(start_out_buffer, &cur_out_buffer, child_def);

                record->child_count++;
                child_def = child_def->next;
            }
        }
        else
        {
            while(child_def)
            {
                ed_e_SerializeEntDefRecursive(start_out_buffer, &cur_out_buffer, child_def);
                record->child_count++;
                child_def = child_def->next;
            }
        }
    }

    record->record_size = (uint64_t)cur_out_buffer - record->record_size;
    *out_buffer = cur_out_buffer;
}

void ed_e_SerializeEntDef(void **buffer, size_t *buffer_size, struct e_ent_def_t *ent_def)
{
    char *start_out_buffer;
    size_t out_size = 0;

    out_size = sizeof(struct e_ent_def_section_t);
    out_size += sizeof(struct e_ent_def_record_t) * ent_def->node_count;
    out_size += sizeof(struct p_col_def_record_t) * ent_def->collider_count;
    out_size += sizeof(struct p_shape_def_fields_t) * ent_def->shape_count;
    out_size += sizeof(struct e_constraint_record_t) * ent_def->constraint_count;

    start_out_buffer = mem_Calloc(1, out_size);
    *buffer = start_out_buffer;
    *buffer_size = out_size;

    char *cur_out_buffer = start_out_buffer;

    struct e_ent_def_section_t *ent_def_section = (struct e_ent_def_section_t *)cur_out_buffer;
    cur_out_buffer += sizeof(struct e_ent_def_section_t);
    ent_def_section->data_start = cur_out_buffer - start_out_buffer;
    ent_def_section->node_count = ent_def->node_count;
    ent_def_section->shape_count = ent_def->shape_count;
    ent_def_section->constraint_count = ent_def->constraint_count;

    ed_e_SerializeEntDefRecursive(start_out_buffer, &cur_out_buffer, ent_def);
}

//void ed_SaveEntDef(char *file_name, struct e_ent_def_t *ent_def)
//{
//
//
//
//
//}
