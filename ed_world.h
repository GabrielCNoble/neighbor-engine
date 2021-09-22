#ifndef ED_WORLD_H
#define ED_WORLD_H

#include "ed_com.h"
#include "ed_pick.h"

//enum ED_PICKABLE_TYPE
//{
//    ED_PICKABLE_TYPE_BRUSH = 1,
//    ED_PICKABLE_TYPE_ENTITY,
//    ED_PICKABLE_TYPE_LIGHT,
//    ED_PICKABLE_TYPE_FACE,
//    ED_PICKABLE_TYPE_WIDGET,
//};
//
//struct ed_pickable_range_t
//{
//    struct ed_pickable_range_t *prev;
//    struct ed_pickable_range_t *next;
//    uint32_t index;
//    uint32_t start;
//    uint32_t count;
//};
//
//struct ed_pickable_t
//{
//    uint32_t type;
//    uint32_t index;
//    struct ds_slist_t *list;
//
//    uint32_t selection_index;
//
//    mat4_t transform;
//    uint32_t mode;
//    uint32_t range_count;
//    struct ed_pickable_range_t *ranges;
//
//    uint32_t primary_index;
//    uint32_t secondary_index;
//};
//
//struct ed_pick_result_t
//{
//    uint32_t type;
//    uint32_t index;
//};
//
//
//
//struct ed_widget_t
//{
//    mat4_t transform;
//    uint32_t index;
//    uint32_t stencil_layer;
//    struct ds_slist_t pickables;
//};

//enum ED_W_CTX_PICKABLE_LISTS
//{
//    ED_W_CTX_PICKABLE_LIST_OBJECT = 0,
//    ED_W_CTX_PICKABLE_LIST_BRUSH_PARTS,
//    ED_W_CTX_PICKABLE_LIST_LAST
////    ED_WORLD_CONTEXT_LIST_WIDGETS
//};

enum ED_WORLD_CONTEXT_STATES
{
    ED_WORLD_CONTEXT_STATE_IDLE = 0,
    ED_WORLD_CONTEXT_STATE_LEFT_CLICK,
    ED_WORLD_CONTEXT_STATE_TRANSFORM_SELECTIONS,
    ED_WORLD_CONTEXT_STATE_BRUSH_BOX,
    ED_WORLD_CONTEXT_STATE_ENTER_OBJECT_EDIT_MODE,
    ED_WORLD_CONTEXT_STATE_ENTER_BRUSH_EDIT_MODE,
    ED_WORLD_CONTEXT_STATE_FLY_CAMERA,
    ED_WORLD_CONTEXT_STATE_LAST
};

enum ED_W_CTX_EDIT_MODES
{
    ED_W_CTX_EDIT_MODE_OBJECT = 0,
    ED_W_CTX_EDIT_MODE_BRUSH,
    ED_W_CTX_EDIT_MODE_LAST
};

enum ED_W_CTX_MANIPULATOR_MODES
{
    ED_W_CTX_MANIPULATOR_MODE_TRANSLATION = 0,
    ED_W_CTX_MANIPULATOR_MODE_ROTATION,
    ED_W_CTX_MANIPULATOR_MODE_SCALE,
};

struct ed_w_ctx_object_list_t
{
    struct ds_slist_t pickables;
    struct ds_list_t selections;
};

struct ed_world_context_data_t
{
    vec3_t box_start;
    vec3_t box_end;
//    uint32_t edit_mode;
//    uint32_t active_list;

    struct
    {
        uint32_t edit_mode;
        struct ed_w_ctx_object_list_t *active_list;
        struct ed_w_ctx_object_list_t lists[ED_W_CTX_EDIT_MODE_LAST];
        struct ed_pickable_t *last_selected;

    } pickables;

//    struct ds_slist_t pickables[3];
//    struct ds_list_t selections[2];
    struct ds_slist_t pickable_ranges;
    struct ds_slist_t widgets;

    struct
    {
        uint32_t mode;
        uint32_t visible;
        vec3_t start_offset;
        struct ed_widget_t *widgets[3];
        mat4_t transform;

    } manipulator;

//    struct ds_slist_t *active_pickables;
//    struct ds_list_t *active_selections;

    struct ds_slist_t brushes;
    uint32_t global_brush_vert_count;
    uint32_t global_brush_index_count;
    struct ds_list_t global_brush_batches;

    float info_window_alpha;
    uint32_t open_delete_selections_popup;
//    uint32_t show_manipulator;

    float camera_pitch;
    float camera_yaw;
    vec3_t camera_pos;
};

void ed_w_Init();

void ed_w_Shutdown();

void ed_w_ManipulatorWidgetSetupPickableDrawState(uint32_t pickable_index, struct ed_pickable_t *pickable);

//struct ed_widget_t *ed_w_ctx_CreateWidget(mat4_t *transform);
//
//void ed_w_ctx_DestroyWidget(struct ed_widget_t *widget);
//
//void ed_w_ctx_WidgetViewProjectionMatrix(struct ed_widget_t *widget, mat4_t *view_projection_matrix);
//
//struct ed_pickable_range_t *ed_w_ctx_AllocPickableRange();
//
//void ed_w_ctx_FreePickableRange(struct ed_pickable_range_t *range);
//
//struct ds_slist_t *ed_w_ctx_PickableListFromType(uint32_t type);
//
//struct ed_pickable_t *ed_w_ctx_CreatePickableOnList(uint32_t type, struct ds_slist_t *pickables);
//
//struct ed_pickable_t *ed_w_ctx_CreatePickable(uint32_t type);
//
//void ed_w_ctx_DestroyPickableOnList(struct ed_pickable_t *pickable, struct ds_slist_t *pickables);
//
//void ed_w_ctx_DestroyPickable(struct ed_pickable_t *pickable);
//
//struct ed_pickable_t *ed_w_ctx_GetPickableOnList(uint32_t index, struct ds_slist_t *pickables);
//
//struct ed_pickable_t *ed_w_ctx_GetPickable(uint32_t index, uint32_t type);
//
//struct ed_pickable_t *ed_w_ctx_CreateBrushPickable(vec3_t *position, mat3_t *orientation, vec3_t *size);
//
//struct ed_pickable_t *ed_w_ctx_CreateLightPickable(vec3_t *pos, vec3_t *color, float radius, float energy);
//
//struct ed_pickable_t *ed_w_ctx_CreateEntityPickable(mat4_t *transform, struct r_model_t *model);

void ed_w_UpdateUI();

void ed_w_UpdatePickables();

void ed_w_DeleteSelections();

void ed_w_AddSelection(struct ed_pickable_t *selection, uint32_t multiple_key_down, struct ds_list_t *selections);

void ed_w_DropSelection(struct ed_pickable_t *selection, struct ds_list_t *selections);

void ed_w_ClearSelections(struct ds_list_t *selections);

void ed_w_TranslateSelected(vec3_t *translation, uint32_t transform_mode);

void ed_w_RotateSelected(mat3_t *rotation, uint32_t transform_mode);

void ed_w_DrawManipulator();

void ed_w_DrawWidgets();

void ed_w_DrawGrid();

void ed_w_DrawBrushes();

void ed_wx_DrawLights();

void ed_w_DrawSelections(struct ds_list_t *selections, struct ds_slist_t *pickables);

void ed_w_PingInfoWindow();

void ed_w_Update();

uint32_t ed_w_IntersectPlaneFromCamera(float mouse_x, float mouse_y, vec3_t *plane_point, vec3_t *plane_normal, vec3_t *result);

void ed_w_Idle(struct ed_context_t *context, uint32_t just_changed);

void ed_w_FlyCamera(struct ed_context_t *context, uint32_t just_changed);

void ed_w_LeftClick(struct ed_context_t *context, uint32_t just_changed);

void ed_w_RightClick(struct ed_context_t *context, uint32_t just_changed);

void ed_w_BrushBox(struct ed_context_t *context, uint32_t just_changed);

void ed_w_WidgetSelected(struct ed_context_t *context, uint32_t just_changed);

void ed_w_TransformSelections(struct ed_context_t *context, uint32_t just_changed);

void ed_w_ObjectSelected(struct ed_context_t *context, uint32_t just_changed);

void ed_w_EnterObjectEditMode(struct ed_context_t *context, uint32_t just_changed);

void ed_w_EnterBrushEditMode(struct ed_context_t *context, uint32_t just_changed);


#endif // ED_W_CTX_H
