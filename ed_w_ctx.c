#include "ed_w_ctx.h"
#include "ed_pick.h"
#include "ed_brush.h"
#include "ed_main.h"
#include "r_main.h"
#include "gui.h"
#include "game.h"
#include "input.h"

extern struct ed_context_t ed_contexts[];
struct ed_world_context_data_t ed_world_context_data;
struct ed_context_t *ed_world_context;
extern mat4_t r_camera_matrix;
extern struct r_shader_t *ed_center_grid_shader;
extern struct r_shader_t *ed_picking_shader;
extern struct r_model_t *ed_translation_widget_model;
struct ed_pickable_t *ed_translation_widget;
struct r_shader_t *ed_outline_shader;
extern struct r_i_verts_t *ed_grid;
extern struct ds_slist_t r_lights;
extern uint32_t r_width;
extern uint32_t r_height;
extern float r_fov;
extern float r_z_near;

vec4_t ed_selection_outline_colors[][2] =
{
    [ED_PICKABLE_TYPE_BRUSH][0] = vec4_t_c(1.0, 0.2, 0.0, 1.0),
    [ED_PICKABLE_TYPE_BRUSH][1] = vec4_t_c(1.0, 0.5, 0.0, 1.0),

    [ED_PICKABLE_TYPE_LIGHT][0] = vec4_t_c(1.0, 0.2, 0.0, 1.0),
    [ED_PICKABLE_TYPE_LIGHT][1] = vec4_t_c(1.0, 0.5, 0.0, 1.0),

    [ED_PICKABLE_TYPE_ENTITY][0] = vec4_t_c(1.0, 0.2, 0.0, 1.0),
    [ED_PICKABLE_TYPE_ENTITY][1] = vec4_t_c(1.0, 0.5, 0.0, 1.0),

    [ED_PICKABLE_TYPE_FACE][0] = vec4_t_c(0.3, 0.4, 1.0, 1.0),
    [ED_PICKABLE_TYPE_FACE][1] = vec4_t_c(0.3, 0.4, 1.0, 1.0),
};

struct ed_state_t ed_world_context_states[] =
{
    [ED_WORLD_CONTEXT_STATE_IDLE] = ed_w_ctx_Idle,
    [ED_WORLD_CONTEXT_STATE_LEFT_CLICK] = ed_w_ctx_LeftClick,
    [ED_WORLD_CONTEXT_STATE_BRUSH_BOX] = ed_w_ctx_BrushBox,
    [ED_WORLD_CONTEXT_STATE_WIDGET_SELECTED] = ed_w_ctx_WidgetSelected
//    [ED_WORLD_CONTEXT_STATE_CREATE_BRUSH] = ed_WorldContextCreateBrush,
//    [ED_WORLD_CONTEXT_STATE_PROCESS_SELECTION] = ed_w_ctx_ProcessSelection,
//    [ED_WORLD_CONTEXT_STATE_ENTER_OBJECT_EDIT_MODE] = ed_WorldContextEnterObjectEditMode,
//    [ED_WORLD_CONTEXT_STATE_ENTER_BRUSH_EDIT_MODE] = ed_WorldContextEnterBrushEditMode,
};

void ed_w_ctx_Init()
{
    ed_contexts[ED_CONTEXT_WORLD].update = ed_w_ctx_Update;
    ed_contexts[ED_CONTEXT_WORLD].states = ed_world_context_states;
    ed_contexts[ED_CONTEXT_WORLD].current_state = ED_WORLD_CONTEXT_STATE_IDLE;
    ed_contexts[ED_CONTEXT_WORLD].context_data = &ed_world_context_data;
    ed_world_context_data.selections[ED_WORLD_CONTEXT_LIST_OBJECTS] = ds_list_create(sizeof(uint32_t), 512);
    ed_world_context_data.selections[ED_WORLD_CONTEXT_LIST_BRUSH_PARTS] = ds_list_create(sizeof(uint32_t), 512);
    ed_world_context_data.pickables[ED_WORLD_CONTEXT_LIST_OBJECTS] = ds_slist_create(sizeof(struct ed_pickable_t), 512);
    ed_world_context_data.pickables[ED_WORLD_CONTEXT_LIST_BRUSH_PARTS] = ds_slist_create(sizeof(struct ed_pickable_t), 512);
    ed_world_context_data.pickables[ED_WORLD_CONTEXT_LIST_WIDGETS] = ds_slist_create(sizeof(struct ed_pickable_t), 512);
    ed_world_context_data.pickable_ranges = ds_slist_create(sizeof(struct ed_pickable_range_t), 512);

    ed_world_context_data.active_pickables = &ed_world_context_data.pickables[0];
    ed_world_context_data.active_selections = &ed_world_context_data.selections[0];

    ed_world_context_data.brushes = ds_slist_create(sizeof(struct ed_brush_t), 512);
    ed_world_context_data.global_brush_batches = ds_list_create(sizeof(struct ed_brush_batch_t), 512);

    ed_world_context_data.camera_pitch = -0.15;
    ed_world_context_data.camera_yaw = -0.3;
    ed_world_context_data.camera_pos = vec3_t_c(-6.0, 4.0, 4.0);

    ed_world_context_data.edit_mode = ED_WORLD_CONTEXT_EDIT_MODE_OBJECT;
    ed_world_context = ed_contexts + ED_CONTEXT_WORLD;


    ed_translation_widget = ed_w_ctx_CreatePickable(ED_PICKABLE_TYPE_WIDGET);
    ed_translation_widget->mode = GL_TRIANGLES;
    ed_translation_widget->range_count = 1;
    ed_translation_widget->ranges = ed_w_ctx_AllocPickableRange();
    ed_translation_widget->ranges->start = ((struct r_batch_t *)ed_translation_widget_model->batches.buffer)->start;
    ed_translation_widget->ranges->count = ed_translation_widget_model->indices.buffer_size;
    ed_translation_widget->primary_index = 0;
    mat4_t_identity(&ed_translation_widget->transform);
}

void ed_w_ctx_Shutdown()
{

}

void ed_w_ctx_FlyCamera()
{
    float dx;
    float dy;

    in_GetMouseDelta(&dx, &dy);

    ed_world_context_data.camera_pitch += dy;
    ed_world_context_data.camera_yaw -= dx;

    if(ed_world_context_data.camera_pitch > 0.5)
    {
        ed_world_context_data.camera_pitch = 0.5;
    }
    else if(ed_world_context_data.camera_pitch < -0.5)
    {
        ed_world_context_data.camera_pitch = -0.5;
    }

    vec4_t translation = {};

    if(in_GetKeyState(SDL_SCANCODE_W) & IN_KEY_STATE_PRESSED)
    {
        translation.z -= 0.05;
    }
    if(in_GetKeyState(SDL_SCANCODE_S) & IN_KEY_STATE_PRESSED)
    {
        translation.z += 0.05;
    }

    if(in_GetKeyState(SDL_SCANCODE_A) & IN_KEY_STATE_PRESSED)
    {
        translation.x -= 0.05;
    }
    if(in_GetKeyState(SDL_SCANCODE_D) & IN_KEY_STATE_PRESSED)
    {
        translation.x += 0.05;
    }

    mat4_t_vec4_t_mul_fast(&translation, &r_camera_matrix, &translation);
    vec3_t_add(&ed_world_context_data.camera_pos, &ed_world_context_data.camera_pos, &vec3_t_c(translation.x, translation.y, translation.z));
}

struct ed_pickable_range_t *ed_w_ctx_AllocPickableRange()
{
    struct ed_pickable_range_t *range = NULL;
    uint32_t index = ds_slist_add_element(&ed_world_context_data.pickable_ranges, NULL);
    range = ds_slist_get_element(&ed_world_context_data.pickable_ranges, index);
    range->index = index;
    return range;
}

void ed_w_ctx_FreePickableRange(struct ed_pickable_range_t *range)
{
    if(range && range->index != 0xffffffff)
    {
        ds_slist_remove_element(&ed_world_context_data.pickable_ranges, range->index);
        range->index = 0xffffffff;
    }
}

struct ds_slist_t *ed_w_ctx_PickableListFromType(uint32_t type)
{
    switch(type)
    {
        case ED_PICKABLE_TYPE_BRUSH:
        case ED_PICKABLE_TYPE_LIGHT:
        case ED_PICKABLE_TYPE_ENTITY:
            return &ed_world_context_data.pickables[ED_WORLD_CONTEXT_LIST_OBJECTS];

        case ED_PICKABLE_TYPE_FACE:
            return &ed_world_context_data.pickables[ED_WORLD_CONTEXT_LIST_BRUSH_PARTS];

        case ED_PICKABLE_TYPE_WIDGET:
            return &ed_world_context_data.pickables[ED_WORLD_CONTEXT_LIST_WIDGETS];

    }

    return NULL;
}

struct ed_pickable_t *ed_w_ctx_CreatePickable(uint32_t type)
{
    struct ed_pickable_t *pickable;
    uint32_t index;

    struct ds_slist_t *list = ed_w_ctx_PickableListFromType(type);

    index = ds_slist_add_element(list, NULL);
    pickable = ds_slist_get_element(list, index);
    pickable->index = index;
    pickable->type = type;
    pickable->selection_index = 0xffffffff;

    return pickable;
}

void ed_w_ctx_DestroyPickable(struct ed_pickable_t *pickable)
{
    if(pickable && pickable->index != 0xffffffff)
    {
        switch(pickable->type)
        {
            case ED_PICKABLE_TYPE_BRUSH:
            {
                ed_DestroyBrush(ed_GetBrush(pickable->primary_index));
                struct ds_slist_t *brush_parts = &ed_world_context_data.pickables[ED_WORLD_CONTEXT_LIST_BRUSH_PARTS];
                struct ds_list_t *brush_selections = &ed_world_context_data.selections[ED_WORLD_CONTEXT_LIST_BRUSH_PARTS];
                for(uint32_t pickable_index = 0; pickable_index < brush_parts->cursor; pickable_index++)
                {
                    struct ed_pickable_t *brush_pickable = ds_slist_get_element(brush_parts, pickable_index);

                    if(brush_pickable && brush_pickable->index != 0xffffffff)
                    {
                        if(brush_pickable->selection_index != 0xffffffff)
                        {
                            ed_w_ctx_DropSelection(brush_pickable, brush_parts, brush_selections);
                        }

                        while(brush_pickable->ranges)
                        {
                            struct ed_pickable_range_t *next = brush_pickable->ranges->next;
                            ed_w_ctx_FreePickableRange(brush_pickable->ranges);
                            brush_pickable->ranges = next;
                        }

                        ds_slist_remove_element(brush_parts, brush_pickable->index);
                        brush_pickable->index = 0xffffffff;
                    }
                }
            }
            break;

            case ED_PICKABLE_TYPE_LIGHT:
                r_DestroyLight(r_GetLight(pickable->primary_index));
            break;

            case ED_PICKABLE_TYPE_ENTITY:
                g_DestroyEntity(g_GetEntity(pickable->primary_index));
            break;
        }

        while(pickable->ranges)
        {
            struct ed_pickable_range_t *next = pickable->ranges->next;
            ed_w_ctx_FreePickableRange(pickable->ranges);
            pickable->ranges = next;
        }

        ds_slist_remove_element(ed_w_ctx_PickableListFromType(pickable->type), pickable->index);
        pickable->index = 0xffffffff;
    }
}

struct ed_pickable_t *ed_w_ctx_GetPickable(uint32_t index, uint32_t type)
{
    struct ed_pickable_t *pickable = NULL;

    struct ds_slist_t *list = ed_w_ctx_PickableListFromType(type);

    pickable = ds_slist_get_element(list, index);

    if(pickable && pickable->index == 0xffffffff)
    {
        pickable = NULL;
    }

    return pickable;
}

struct ed_pickable_t *ed_w_ctx_CreateBrushPickable(vec3_t *position, mat3_t *orientation, vec3_t *size)
{
    struct ed_pickable_t *pickable = NULL;

    struct ed_brush_t *brush = ed_CreateBrush(position, orientation, size);
    struct r_batch_t *first_batch = (struct r_batch_t *)brush->model->batches.buffer;

    pickable = ed_w_ctx_CreatePickable(ED_PICKABLE_TYPE_BRUSH);
    pickable->mode = GL_TRIANGLES;
    pickable->primary_index = brush->index;
    pickable->range_count = 1;
    pickable->ranges = ed_w_ctx_AllocPickableRange();
//    pickable->start = first_batch->start;
//    pickable->count = brush->model->indices.buffer_size;

    for(uint32_t face_index = 0; face_index < brush->faces.cursor; face_index++)
    {
        struct ed_face_t *face = ds_list_get_element(&brush->faces, face_index);
        struct ed_pickable_t *face_pickable = ed_w_ctx_CreatePickable(ED_PICKABLE_TYPE_FACE);
        face_pickable->primary_index = brush->index;
        face_pickable->secondary_index = face_index;
        face_pickable->mode = GL_TRIANGLES;
        face_pickable->range_count = 0;
    }

    return pickable;
}

struct ed_pickable_t *ed_w_ctx_CreateLightPickable(vec3_t *pos, vec3_t *color, float radius, float energy)
{
    return NULL;
}

struct ed_pickable_t *ed_w_ctx_CreateEntityPickable(mat4_t *transform, struct r_model_t *model)
{
    return NULL;
}

void ed_w_ctx_UpdateUI()
{
    int32_t mouse_x;
    int32_t mouse_y;

    in_GetMousePos(&mouse_x, &mouse_y);

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize;
    window_flags |= ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoInputs;

    igSetNextWindowBgAlpha(0.5);
    igPushStyleVar_Float(ImGuiStyleVar_WindowBorderSize, 0.0);
    igSetNextWindowPos((ImVec2){0.0, 20.0}, ImGuiCond_Always, (ImVec2){0.0, 0.0});
    if(igBegin("Info window", NULL, window_flags))
    {
        char *edit_mode_text;

        switch(ed_world_context_data.edit_mode)
        {
            case ED_WORLD_CONTEXT_EDIT_MODE_OBJECT:
                edit_mode_text = "Object";
            break;

            case ED_WORLD_CONTEXT_EDIT_MODE_BRUSH:
                edit_mode_text = "Brush";
            break;
        }
        igPushStyleVar_Float(ImGuiStyleVar_Alpha, 0.5);
        if(igBeginTable("Stats table", 4, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_Hideable, (ImVec2){0.0, 0.0}, 0.0))
        {
            igTableNextRow(0, 0.0);
            igTableNextColumn();
            igText("Edit mode: %s  ", edit_mode_text);
            igTableNextColumn();
            igText("Objects: %d  ", ed_world_context_data.pickables[ED_WORLD_CONTEXT_LIST_OBJECTS].used);
            igTableNextColumn();
            igText("Selected: %d  ", ed_world_context_data.active_selections->cursor);
            igTableNextColumn();

            if(ed_world_context_data.edit_mode == ED_WORLD_CONTEXT_EDIT_MODE_BRUSH)
            {
                igTableSetColumnEnabled(-1, 1);

                if(ed_world_context->current_state == ED_WORLD_CONTEXT_STATE_BRUSH_BOX)
                {
                    igText("Creating brush");
                }
                else
                {
                    igText("Manipulating brush");
                }
            }
            else
            {
                igTableSetColumnEnabled(-1, 0);
            }

            igEndTable();
        }

        igPopStyleVar(1);
    }
    igEnd();
    igPopStyleVar(1);


    if(ed_world_context_data.open_delete_selections_popup)
    {
        igOpenPopup_Str("Delete selections", 0);
        ed_world_context_data.open_delete_selections_popup = 0;
    }

    igSetNextWindowPos((ImVec2){mouse_x, mouse_y}, ImGuiCond_Once, (ImVec2){0.0, 0.0});

    if(igBeginPopup("Delete selections", 0))
    {
        if(igMenuItem_Bool("Delete selections?", NULL, 0, 1))
        {
            ed_w_ctx_DeleteSelections();
        }
        igEndPopup();
    }
}

void ed_w_ctx_UpdatePickables()
{
    for(uint32_t pickable_index = 0; pickable_index < ed_world_context_data.active_pickables->cursor; pickable_index++)
    {
        struct ed_pickable_t *pickable = ds_slist_get_element(ed_world_context_data.active_pickables, pickable_index);

        if(pickable && pickable->index != 0xffffffff)
        {
            mat4_t_identity(&pickable->transform);

            switch(pickable->type)
            {
                case ED_PICKABLE_TYPE_BRUSH:
                {
                    struct ed_brush_t *brush = ed_GetBrush(pickable->primary_index);
                    struct r_batch_t *first_batch = brush->model->batches.buffer;
                    pickable->ranges->start = first_batch->start;
                    pickable->ranges->count = brush->model->indices.buffer_size;
                    mat4_t_comp(&pickable->transform, &brush->orientation, &brush->position);
                }
                break;

                case ED_PICKABLE_TYPE_FACE:
                {
                    struct ed_brush_t *brush = ed_GetBrush(pickable->primary_index);
                    struct ed_face_t *face = ds_list_get_element(&brush->faces, pickable->secondary_index);
                    struct r_batch_t *first_batch = brush->model->batches.buffer;

                    if(pickable->range_count > face->clipped_polygon_count)
                    {
                        while(pickable->range_count > face->clipped_polygon_count)
                        {
                            struct ed_pickable_range_t *next_range = pickable->ranges->next;
                            next_range->prev = NULL;

                            ed_w_ctx_FreePickableRange(pickable->ranges);
                            pickable->range_count--;
                            pickable->ranges = next_range;
                        }
                    }
                    else if(pickable->range_count < face->clipped_polygon_count)
                    {
                        while(pickable->range_count < face->clipped_polygon_count)
                        {
                            struct ed_pickable_range_t *new_range = ed_w_ctx_AllocPickableRange();
                            new_range->next = pickable->ranges;
                            if(pickable->ranges)
                            {
                                pickable->ranges->prev = new_range;
                            }
                            pickable->ranges = new_range;
                            pickable->range_count++;
                        }
                    }

                    struct ed_polygon_t *polygon = face->clipped_polygons;
                    struct ed_pickable_range_t *range = pickable->ranges;

                    while(polygon)
                    {
                        range->start = polygon->mesh_start + first_batch->start;
                        range->count = polygon->mesh_count;

                        polygon = polygon->next;
                        range = range->next;
                    }

                    mat4_t_comp(&pickable->transform, &brush->orientation, &brush->position);
                }
                break;

                case ED_PICKABLE_TYPE_LIGHT:

                break;

                case ED_PICKABLE_TYPE_ENTITY:

                break;
            }
        }
    }
}

void ed_w_ctx_ClearSelections()
{
    for(uint32_t selection_index = 0; selection_index < ed_world_context_data.active_selections->cursor; selection_index++)
    {
        uint32_t pickable_index = *(uint32_t *)ds_list_get_element(ed_world_context_data.active_selections, selection_index);
        struct ed_pickable_t *pickable = ds_slist_get_element(ed_world_context_data.active_pickables, pickable_index);
        pickable->selection_index = 0xffffffff;
    }

    ed_world_context_data.active_selections->cursor = 0;
}

void ed_w_ctx_DeleteSelections()
{
    for(uint32_t selection_index = 0; selection_index < ed_world_context_data.active_selections->cursor; selection_index++)
    {
        uint32_t pickable_index = *(uint32_t *)ds_list_get_element(ed_world_context_data.active_selections, selection_index);
        struct ed_pickable_t *pickable = ds_slist_get_element(ed_world_context_data.active_pickables, pickable_index);
        ed_w_ctx_DestroyPickable(pickable);
    }

    ed_w_ctx_ClearSelections();
}

void ed_w_ctx_AddSelection(struct ed_pickable_t *selection, uint32_t multiple_key_down)
{
//    uint32_t selection_index = 0xffffffff;

//    for(uint32_t index = 0; index < ed_world_context_data.active_selections->cursor; index++)
//    {
//        uint32_t pickable_index = *(uint32_t *)ds_list_get_element(ed_world_context_data.active_selections, index);
//
//        if(pickable_index == selection->index)
//        {
//            selection_index = index;
//            break;
//        }
//    }

    if(selection->selection_index != 0xffffffff)
    {
        uint32_t last_index = ed_world_context_data.active_selections->cursor - 1;
        uint32_t selection_index = selection->selection_index;

        /* This selection already exists in the list. In this case, it can either be the
        main selection (last in the list), in which case it'll be dropped from the list, or
        it can be some other selection, in which case it'll be re-added at the back of the
        list, becoming the main selection. Either way, we need to remove it here. */
        ed_w_ctx_DropSelection(selection, ed_world_context_data.active_pickables, ed_world_context_data.active_selections);

        if(selection_index >= last_index)
        {
            /* pickable is the last in the list */
            if(multiple_key_down || !ed_world_context_data.active_selections->cursor)
            {
                /* the behavior is, if this is the last pickable, there are more pickables
                in the list and the multiple selection key is down, this pickable gets dropped.
                If, instead, this is the last pickable, there are more pickables in the list
                but the key is not being held, then this pickable becomes the only pickable in
                the list. If, then, it gets selected again, without the multiple selection key
                down, it gets dropped. */
                return;
            }
        }
    }

    if(!multiple_key_down)
    {
        ed_w_ctx_ClearSelections();
    }

    /* This is either a new selection, or an already existing selection becoming the main selection. */
    selection->selection_index = ds_list_add_element(ed_world_context_data.active_selections, &selection->index);
//    printf("%p %d\n", selection, selection->selection_index);
}

void ed_w_ctx_DropSelection(struct ed_pickable_t *selection, struct ds_slist_t *pickable_list, struct ds_list_t *selection_list)
{
    if(selection && selection->selection_index != 0xffffffff)
    {
        uint32_t last_index = selection_list->cursor - 1;
        ds_list_remove_element(selection_list, selection->selection_index);

        if(selection_list->cursor && selection->selection_index != last_index)
        {
            uint32_t moved_pickable_index = *(uint32_t *)ds_list_get_element(selection_list, selection->selection_index);
            struct ed_pickable_t *moved_pickable = ds_slist_get_element(pickable_list, moved_pickable_index);
            moved_pickable->selection_index = selection->selection_index;
        }

        selection->selection_index = 0xffffffff;
    }
}

void ed_w_ctx_TranslateSelected(vec3_t *translation)
{
    for(uint32_t pickable_index = 0; pickable_index < ed_world_context_data.active_selections->cursor; pickable_index++)
    {

    }
}

void ed_w_ctx_RotateSelected(mat3_t *rotation)
{

}

void ed_w_ctx_DrawWidgets()
{
    r_i_SetModelMatrix(NULL);
    r_i_SetViewProjectionMatrix(NULL);
    r_i_SetShader(ed_outline_shader);
    r_i_SetUniform(r_GetNamedUniform(ed_outline_shader, "ed_color"), 1, &vec4_t_c(1.0, 0.0, 0.0, 1.0));

    struct r_i_draw_list_t *draw_list = r_i_AllocDrawList(1);
    draw_list->commands[0].start = ed_translation_widget->ranges->start;
    draw_list->commands[0].count = ed_translation_widget->ranges->count;
    draw_list->indexed = 1;
    r_i_DrawImmediate(R_I_DRAW_CMD_TRIANGLE_LIST, draw_list);
}

void ed_w_ctx_DrawGrid()
{
    r_i_SetModelMatrix(NULL);
    r_i_SetViewProjectionMatrix(NULL);
    r_i_SetBlending(GL_TRUE, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    r_i_SetStencil(GL_FALSE, GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE);
    r_i_SetRasterizer(GL_FALSE, GL_BACK, GL_FILL);
    r_i_SetShader(ed_center_grid_shader);
    r_i_DrawVerts(R_I_DRAW_CMD_TRIANGLE_LIST, ed_grid, 1.0);
    r_i_SetShader(NULL);
    r_i_SetModelMatrix(NULL);
    r_i_SetViewProjectionMatrix(NULL);
    r_i_SetBlending(GL_FALSE, GL_ONE, GL_ZERO);
    r_i_DrawLine(&vec3_t_c(-10000.0, 0.0, 0.0), &vec3_t_c(10000.0, 0.0, 0.0), &vec4_t_c(1.0, 0.0, 0.0, 1.0), 3.0);
    r_i_DrawLine(&vec3_t_c(0.0, 0.0, -10000.0), &vec3_t_c(0.0, 0.0, 10000.0), &vec4_t_c(0.0, 0.0, 1.0, 1.0), 3.0);
}

void ed_w_ctx_DrawBrushes()
{
    for(uint32_t brush_index = 0; brush_index < ed_world_context_data.brushes.cursor; brush_index++)
    {
        struct ed_brush_t *brush = ed_GetBrush(brush_index);

        if(brush)
        {
            mat4_t transform;
            mat4_t_identity(&transform);
            mat4_t_comp(&transform, &brush->orientation, &brush->position);
            r_DrawEntity(&transform, brush->model);
        }
    }
}

void ed_w_ctx_DrawLights()
{
    r_i_SetModelMatrix(NULL);
    r_i_SetViewProjectionMatrix(NULL);
    r_i_SetShader(NULL);

    for(uint32_t light_index = 0; light_index < r_lights.cursor; light_index++)
    {
        struct r_light_t *light = r_GetLight(light_index);

        if(light)
        {
            vec3_t position = vec3_t_c(light->data.pos_rad.x, light->data.pos_rad.y, light->data.pos_rad.z);
            vec4_t color = vec4_t_c(light->data.color_res.x, light->data.color_res.y, light->data.color_res.z, 1.0);
            r_i_DrawPoint(&position, &color, 8.0);
        }
    }
}

void ed_w_ctx_DrawSelections(struct ds_list_t *selections, struct ds_slist_t *pickables)
{
    if(ed_world_context_data.active_selections->cursor)
    {
        r_i_SetViewProjectionMatrix(NULL);
        r_i_SetShader(ed_outline_shader);
        r_i_SetBuffers(NULL, NULL);
        r_i_SetRasterizer(GL_TRUE, GL_FRONT, GL_LINE);

        uint32_t selection_count = ed_world_context_data.active_selections->cursor - 1;
        uint32_t selection_index = 0;
        uint8_t stencil_value = 1;

        struct r_named_uniform_t *color_uniform = r_GetNamedUniform(ed_outline_shader, "ed_color");

        for(uint32_t index = 0; index < 2; index++)
        {
            for(; selection_index < selection_count; selection_index++)
            {
                uint32_t pickable_index = *(uint32_t *)ds_list_get_element(ed_world_context_data.active_selections, selection_index);
                struct ed_pickable_t *pickable = ds_slist_get_element(ed_world_context_data.active_pickables, pickable_index);

                r_i_SetModelMatrix(&pickable->transform);
                struct r_i_draw_list_t *draw_list = NULL;

//                r_i_SetUniform(color_uniform, 1, &ed_selection_outline_colors[pickable->type][index != 0].comps);

                switch(pickable->type)
                {
                    case ED_PICKABLE_TYPE_FACE:
                        draw_list = r_i_AllocDrawList(pickable->range_count);
                        struct ed_pickable_range_t *range = pickable->ranges;

                        for(uint32_t range_index = 0; range_index < pickable->range_count; range_index++)
                        {
                            draw_list->commands[range_index].start = range->start;
                            draw_list->commands[range_index].count = range->count;
                            range = range->next;
                        }
                        draw_list->size = 6.0;
                        draw_list->indexed = 1;

                        r_i_SetDrawMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE, 0xff);
                        r_i_SetStencil(GL_TRUE, GL_KEEP, GL_KEEP, GL_REPLACE, GL_ALWAYS, 0xff, 0xff);
                        r_i_SetDepth(GL_TRUE, GL_ALWAYS);
                        r_i_SetRasterizer(GL_TRUE, GL_BACK, GL_FILL);
                        r_i_DrawImmediate(R_I_DRAW_CMD_TRIANGLE_LIST, draw_list);



                        r_i_SetUniform(color_uniform, 1, &vec4_t_c(0.2, 0.7, 0.4, 1.0));
                        draw_list = r_i_AllocDrawList(pickable->range_count);
                        range = pickable->ranges;

                        for(uint32_t range_index = 0; range_index < pickable->range_count; range_index++)
                        {
                            draw_list->commands[range_index].start = range->start;
                            draw_list->commands[range_index].count = range->count;
                            range = range->next;
                        }
                        draw_list->size = 6.0;
                        draw_list->indexed = 1;

                        r_i_SetDrawMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
                        r_i_SetStencil(GL_TRUE, GL_KEEP, GL_KEEP, GL_KEEP, GL_EQUAL, 0xff, 0x00);
                        r_i_SetRasterizer(GL_TRUE, GL_BACK, GL_LINE);
                        r_i_DrawImmediate(R_I_DRAW_CMD_TRIANGLE_LIST, draw_list);



                        r_i_SetUniform(color_uniform, 1, &vec4_t_c(0.3, 0.4, 1.0, 0.4));
                        draw_list = r_i_AllocDrawList(pickable->range_count);
                        range = pickable->ranges;

                        for(uint32_t range_index = 0; range_index < pickable->range_count; range_index++)
                        {
                            draw_list->commands[range_index].start = range->start;
                            draw_list->commands[range_index].count = range->count;
                            range = range->next;
                        }
                        draw_list->size = 4.0;
                        draw_list->indexed = 1;

                        r_i_SetBlending(GL_TRUE, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                        r_i_SetDrawMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE, 0xff);
                        r_i_SetStencil(GL_TRUE, GL_KEEP, GL_KEEP, GL_REPLACE, GL_ALWAYS, 0xff, 0x00);
                        r_i_SetDepth(GL_TRUE, GL_ALWAYS);
                        r_i_SetRasterizer(GL_TRUE, GL_BACK, GL_FILL);
                        r_i_DrawImmediate(R_I_DRAW_CMD_TRIANGLE_LIST, draw_list);
                    break;

                    default:
                        if(index)
                        {
                            r_i_SetUniform(color_uniform, 1, &vec4_t_c(1.0, 0.4, 0.0, 1.0));
                        }
                        else
                        {
                            r_i_SetUniform(color_uniform, 1, &vec4_t_c(1.0, 0.2, 0.0, 1.0));
                        }


                        draw_list = r_i_AllocDrawList(1);
                        draw_list->commands[0].start = pickable->ranges->start;
                        draw_list->commands[0].count = pickable->ranges->count;
                        draw_list->size = 4.0;
                        draw_list->indexed = 1;

                        r_i_SetDrawMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE, 0xff);
                        r_i_SetDepth(GL_TRUE, GL_ALWAYS);
                        r_i_SetStencil(GL_TRUE, GL_KEEP, GL_KEEP, GL_REPLACE, GL_ALWAYS, 0xff, 0xff);
                        r_i_SetRasterizer(GL_TRUE, GL_FRONT, GL_FILL);
                        r_i_DrawImmediate(R_I_DRAW_CMD_TRIANGLE_LIST, draw_list);

                        draw_list = r_i_AllocDrawList(1);
                        draw_list->commands[0].start = pickable->ranges->start;
                        draw_list->commands[0].count = pickable->ranges->count;
                        draw_list->size = 4.0;
                        draw_list->indexed = 1;

                        r_i_SetDrawMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE, 0xff);
                        r_i_SetDepth(GL_TRUE, GL_ALWAYS);
                        r_i_SetStencil(GL_TRUE, GL_KEEP, GL_KEEP, GL_KEEP, GL_EQUAL, 0xff, 0x00);
                        r_i_SetRasterizer(GL_TRUE, GL_FRONT, GL_LINE);
                        r_i_DrawImmediate(R_I_DRAW_CMD_TRIANGLE_LIST, draw_list);

                        draw_list = r_i_AllocDrawList(1);
                        draw_list->commands[0].start = pickable->ranges->start;
                        draw_list->commands[0].count = pickable->ranges->count;
                        draw_list->size = 4.0;
                        draw_list->indexed = 1;

                        r_i_SetDrawMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE, 0xff);
                        r_i_SetDepth(GL_TRUE, GL_ALWAYS);
                        r_i_SetStencil(GL_TRUE, GL_KEEP, GL_KEEP, GL_REPLACE, GL_ALWAYS, 0xff, 0x00);
                        r_i_SetRasterizer(GL_TRUE, GL_FRONT, GL_FILL);
                        r_i_DrawImmediate(R_I_DRAW_CMD_TRIANGLE_LIST, draw_list);
                    break;
                }
            }

            selection_count++;
        }

        r_i_SetRasterizer(GL_TRUE, GL_BACK, GL_FILL);
        r_i_SetDepth(GL_TRUE, GL_LESS);
        r_i_SetBlending(GL_FALSE, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        r_i_SetDrawMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE, 0xff);
        r_i_SetStencil(GL_FALSE, GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE);
    }
}

void ed_w_ctx_PingInfoWindow()
{
    ed_world_context_data.info_window_alpha = 1.0;
}

void ed_w_ctx_Update()
{
    r_SetViewPos(&ed_world_context_data.camera_pos);
    r_SetViewPitchYaw(ed_world_context_data.camera_pitch, ed_world_context_data.camera_yaw);

    ed_w_ctx_UpdateUI();
    ed_w_ctx_UpdatePickables();
    ed_w_ctx_DrawGrid();
    ed_w_ctx_DrawSelections(NULL, NULL);
    ed_w_ctx_DrawBrushes();
    ed_w_ctx_DrawLights();
    ed_w_ctx_DrawWidgets();
}

void ed_w_ctx_Idle(struct ed_context_t *context, uint32_t just_changed)
{
    struct ed_world_context_data_t *context_data = context->context_data;

    if(in_GetMouseButtonState(SDL_BUTTON_RIGHT) & IN_KEY_STATE_PRESSED)
    {
        ed_w_ctx_FlyCamera();
    }
    else if(in_GetMouseButtonState(SDL_BUTTON_LEFT) & IN_KEY_STATE_JUST_PRESSED)
    {
        ed_SetNextContextState(context, ED_WORLD_CONTEXT_STATE_LEFT_CLICK);
    }

    if(in_GetKeyState(SDL_SCANCODE_TAB) & IN_KEY_STATE_JUST_PRESSED)
    {
        if(context_data->edit_mode == ED_WORLD_CONTEXT_EDIT_MODE_OBJECT)
        {
            ed_w_ctx_EnterBrushEditMode(context, just_changed);
        }
        else
        {
            ed_w_ctx_EnterObjectEditMode(context, just_changed);
        }
    }

    if(ed_world_context_data.active_selections->cursor && (in_GetKeyState(SDL_SCANCODE_DELETE) & IN_KEY_STATE_JUST_PRESSED))
    {
        ed_world_context_data.open_delete_selections_popup = 1;
    }
}

void ed_w_ctx_RightClick(struct ed_context_t *context, uint32_t just_changed)
{

}

void ed_w_ctx_LeftClick(struct ed_context_t *context, uint32_t just_changed)
{
    struct ed_world_context_data_t *context_data = (struct ed_world_context_data_t *)context->context_data;
    uint32_t left_button_state = in_GetMouseButtonState(SDL_BUTTON_LEFT);
    float dx = 0;
    float dy = 0;
    int32_t mouse_x;
    int32_t mouse_y;

    in_GetMouseDelta(&dx, &dy);
    in_GetMousePos(&mouse_x, &mouse_y);

    if(just_changed)
    {
        context_data->last_selected = ed_SelectPickable(mouse_x, mouse_y, &ed_world_context_data.pickables[ED_WORLD_CONTEXT_LIST_WIDGETS]);

        if(!context_data->last_selected)
        {
            context_data->last_selected = ed_SelectPickable(mouse_x, mouse_y, ed_world_context_data.active_pickables);
        }
    }

    if(left_button_state & IN_KEY_STATE_PRESSED)
    {
        if(context_data->last_selected)
        {
            if(context_data->last_selected->type == ED_PICKABLE_TYPE_WIDGET)
            {
                ed_SetNextContextState(context, ED_WORLD_CONTEXT_STATE_WIDGET_SELECTED);
            }
        }
        else if((in_GetKeyState(SDL_SCANCODE_LCTRL) & IN_KEY_STATE_PRESSED) && context_data->edit_mode == ED_WORLD_CONTEXT_EDIT_MODE_BRUSH)
        {
            ed_SetNextContextState(context, ED_WORLD_CONTEXT_STATE_BRUSH_BOX);
        }
    }
    else
    {
        if(context_data->last_selected)
        {
            ed_w_ctx_ObjectSelected(context, just_changed);
        }
        else
        {
            ed_SetNextContextState(context, ED_WORLD_CONTEXT_STATE_IDLE);
        }
    }
}

void ed_w_ctx_BrushBox(struct ed_context_t *context, uint32_t just_changed)
{
    struct ed_world_context_data_t *context_data = (struct ed_world_context_data_t *)context->context_data;

    if(in_GetMouseButtonState(SDL_BUTTON_LEFT) & IN_KEY_STATE_PRESSED)
    {
        if(in_GetKeyState(SDL_SCANCODE_ESCAPE) & IN_KEY_STATE_PRESSED)
        {
            ed_SetNextContextState(context, ED_WORLD_CONTEXT_STATE_IDLE);
        }
        else
        {
            vec3_t mouse_pos;
            vec3_t camera_pos;
            vec4_t mouse_vec = {};

            float aspect = (float)r_width / (float)r_height;
            float top = tan(r_fov) * r_z_near;
            float right = top * aspect;

            in_GetNormalizedMousePos(&mouse_vec.x, &mouse_vec.y);
            mouse_vec.x *= right;
            mouse_vec.y *= top;
            mouse_vec.z = -r_z_near;
            vec4_t_normalize(&mouse_vec, &mouse_vec);
            mat4_t_vec4_t_mul_fast(&mouse_vec, &r_camera_matrix, &mouse_vec);

            camera_pos.x = r_camera_matrix.rows[3].x;
            camera_pos.y = r_camera_matrix.rows[3].y;
            camera_pos.z = r_camera_matrix.rows[3].z;

            mouse_pos.x = camera_pos.x + mouse_vec.x;
            mouse_pos.y = camera_pos.y + mouse_vec.y;
            mouse_pos.z = camera_pos.z + mouse_vec.z;

            float dist_a = camera_pos.y;
            float dist_b = mouse_pos.y;
            float denom = (dist_a - dist_b);

            r_i_SetModelMatrix(NULL);
            r_i_SetViewProjectionMatrix(NULL);
            r_i_SetShader(NULL);

            if(denom)
            {
                float frac = dist_a / denom;
                vec3_t intersection = {};
                vec3_t_fmadd(&intersection, &camera_pos, &vec3_t_c(mouse_vec.x, mouse_vec.y, mouse_vec.z), frac);

                if(just_changed)
                {
                    context_data->box_start = intersection;
                }

                context_data->box_end = intersection;
                vec3_t start = context_data->box_start;
                vec3_t end = context_data->box_end;

                r_i_DrawLine(&start, &vec3_t_c(start.x, start.y, end.z), &vec4_t_c(0.0, 1.0, 0.0, 1.0), 2.0);
                r_i_DrawLine(&vec3_t_c(start.x, start.y, end.z), &end, &vec4_t_c(0.0, 1.0, 0.0, 1.0), 2.0);
                r_i_DrawLine(&end, &vec3_t_c(end.x, start.y, start.z), &vec4_t_c(0.0, 1.0, 0.0, 1.0), 2.0);
                r_i_DrawLine(&vec3_t_c(end.x, start.y, start.z), &start, &vec4_t_c(0.0, 1.0, 0.0, 1.0), 2.0);
            }
        }
    }
    else
    {
        if(just_changed)
        {
            ed_SetNextContextState(context, ED_WORLD_CONTEXT_STATE_IDLE);
        }
        else
        {
            vec3_t position;
            vec3_t size;
            mat3_t orientation;

            mat3_t_identity(&orientation);
            vec3_t_sub(&size, &context_data->box_end, &context_data->box_start);
            vec3_t_add(&position, &context_data->box_start, &context_data->box_end);
            vec3_t_mul(&position, &position, 0.5);

            size.y = 1.0;

            ed_w_ctx_CreateBrushPickable(&position, &orientation, &size);
            ed_SetNextContextState(context, ED_WORLD_CONTEXT_STATE_IDLE);
//            ed_w_ctx_PingInfoWindow();
        }
    }
}

void ed_w_ctx_WidgetSelected(struct ed_context_t *context, uint32_t just_changed)
{

}

void ed_w_ctx_ObjectSelected(struct ed_context_t *context, uint32_t just_changed)
{
    struct ed_world_context_data_t *context_data = (struct ed_world_context_data_t *)context->context_data;
    uint32_t mouse_state = in_GetMouseButtonState(SDL_BUTTON_LEFT);
    uint32_t shift_state = in_GetKeyState(SDL_SCANCODE_LSHIFT);

    int32_t mouse_x;
    int32_t mouse_y;
    in_GetMousePos(&mouse_x, &mouse_y);
    struct ed_pickable_t *selection = ed_SelectPickable(mouse_x, mouse_y, ed_world_context_data.active_pickables);

    if(selection == context_data->last_selected)
    {
        ed_w_ctx_AddSelection(selection, shift_state & IN_KEY_STATE_PRESSED);
    }

//    for(uint32_t selection_index = 0; selection_index < ed_world_context_data.active_selections->cursor; selection_index++)
//    {
//        uint32_t pickable_index = *(uint32_t *)ds_list_get_element(ed_world_context_data.active_selections, selection_index);
//        struct ed_pickable_t *pickable = ds_slist_get_element(ed_world_context_data.active_pickables, pickable_index);
//
//        printf("%p %d %d\n", pickable, pickable->selection_index, selection_index);
//    }
//
//    printf("\n");

    context_data->last_selected = NULL;
}

void ed_w_ctx_EnterObjectEditMode(struct ed_context_t *context, uint32_t just_changed)
{
    struct ed_world_context_data_t *context_data = context->context_data;
    context_data->edit_mode = ED_WORLD_CONTEXT_EDIT_MODE_OBJECT;
    context_data->active_pickables = &context_data->pickables[ED_WORLD_CONTEXT_LIST_OBJECTS];
    context_data->active_selections = &context_data->selections[ED_WORLD_CONTEXT_LIST_OBJECTS];
}

void ed_w_ctx_EnterBrushEditMode(struct ed_context_t *context, uint32_t just_changed)
{
    struct ed_world_context_data_t *context_data = context->context_data;
    context_data->edit_mode = ED_WORLD_CONTEXT_EDIT_MODE_BRUSH;
    context_data->active_pickables = &context_data->pickables[ED_WORLD_CONTEXT_LIST_BRUSH_PARTS];
    context_data->active_selections = &context_data->selections[ED_WORLD_CONTEXT_LIST_BRUSH_PARTS];
}
