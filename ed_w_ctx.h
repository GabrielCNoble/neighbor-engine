#ifndef ED_W_CTX_H
#define ED_W_CTX_H

#include "ed_com.h"

enum ED_PICKABLE_TYPE
{
    ED_PICKABLE_TYPE_BRUSH = 1,
    ED_PICKABLE_TYPE_ENTITY,
    ED_PICKABLE_TYPE_LIGHT,
    ED_PICKABLE_TYPE_FACE,
    ED_PICKABLE_TYPE_WIDGET,
};

struct ed_pickable_range_t
{
    struct ed_pickable_range_t *prev;
    struct ed_pickable_range_t *next;
    uint32_t index;
    uint32_t start;
    uint32_t count;
};

struct ed_pickable_t
{
    uint32_t type;
    uint32_t index;
    uint32_t selection_index;

    mat4_t transform;
    uint32_t mode;
    uint32_t range_count;
    struct ed_pickable_range_t *ranges;

    uint32_t primary_index;
    uint32_t secondary_index;
};

struct ed_pick_result_t
{
    uint32_t type;
    uint32_t index;
};

struct ed_widget_t
{
    mat4_t transform;
    uint32_t index;
    struct ds_slist_t pickables;
};

enum ED_WORLD_CONTEXT_LISTS
{
    ED_WORLD_CONTEXT_LIST_OBJECTS = 0,
    ED_WORLD_CONTEXT_LIST_BRUSH_PARTS,
    ED_WORLD_CONTEXT_LIST_WIDGETS
};

enum ED_WORLD_CONTEXT_STATES
{
    ED_WORLD_CONTEXT_STATE_IDLE = 0,
    ED_WORLD_CONTEXT_STATE_LEFT_CLICK,
    ED_WORLD_CONTEXT_STATE_WIDGET_SELECTED,
    ED_WORLD_CONTEXT_STATE_BRUSH_BOX,
    ED_WORLD_CONTEXT_STATE_ENTER_OBJECT_EDIT_MODE,
    ED_WORLD_CONTEXT_STATE_ENTER_BRUSH_EDIT_MODE,
    ED_WORLD_CONTEXT_STATE_FLY_CAMERA,
    ED_WORLD_CONTEXT_STATE_LAST
};

enum ED_WORLD_CONTEXT_EDIT_MODES
{
    ED_WORLD_CONTEXT_EDIT_MODE_OBJECT = 0,
    ED_WORLD_CONTEXT_EDIT_MODE_BRUSH,
};

enum ED_W_CTX_MANIPULATORS
{
    ED_W_CTX_MANIPULATOR_TRANSLATION = 0,
    ED_W_CTX_MANIPULATOR_ROTATION,
    ED_W_CTX_MANIPULATOR_SCALE,
    ED_W_CTX_MANIPULATOR_EXTRUDE,
};

struct ed_world_context_data_t
{
    vec3_t box_start;
    vec3_t box_end;
    struct ed_pickable_t *last_selected;
    uint32_t edit_mode;
    uint32_t active_list;

    struct ds_slist_t pickables[3];
    struct ds_list_t selections[2];
    struct ds_slist_t pickable_ranges;
    struct ds_slist_t widgets;

    struct ed_widget_t *manipulators[3];

    struct ds_slist_t *active_pickables;
    struct ds_list_t *active_selections;

    struct ds_slist_t brushes;
    uint32_t global_brush_vert_count;
    uint32_t global_brush_index_count;
    struct ds_list_t global_brush_batches;

    float info_window_alpha;
    uint32_t open_delete_selections_popup;
    uint32_t show_manipulator;

    float camera_pitch;
    float camera_yaw;
    vec3_t camera_pos;
};

void ed_w_ctx_Init();

void ed_w_ctx_Shutdown();

struct ed_widget_t *ed_w_ctx_CreateWidget(mat4_t *transform);

void ed_w_ctx_DestroyWidget(struct ed_widget_t *widget);

struct ed_pickable_range_t *ed_w_ctx_AllocPickableRange();

void ed_w_ctx_FreePickableRange(struct ed_pickable_range_t *range);

struct ds_slist_t *ed_w_ctx_PickableListFromType(uint32_t type);

struct ed_pickable_t *ed_w_ctx_CreatePickableOnList(uint32_t type, struct ds_slist_t *pickables);

struct ed_pickable_t *ed_w_ctx_CreatePickable(uint32_t type);

void ed_w_ctx_DestroyPickableOnList(struct ed_pickable_t *pickable, struct ds_slist_t *pickables);

void ed_w_ctx_DestroyPickable(struct ed_pickable_t *pickable);

struct ed_pickable_t *ed_w_ctx_GetPickableOnList(uint32_t index, struct ds_slist_t *pickables);

struct ed_pickable_t *ed_w_ctx_GetPickable(uint32_t index, uint32_t type);

struct ed_pickable_t *ed_w_ctx_CreateBrushPickable(vec3_t *position, mat3_t *orientation, vec3_t *size);

struct ed_pickable_t *ed_w_ctx_CreateLightPickable(vec3_t *pos, vec3_t *color, float radius, float energy);

struct ed_pickable_t *ed_w_ctx_CreateEntityPickable(mat4_t *transform, struct r_model_t *model);

void ed_w_ctx_UpdateUI();

void ed_w_ctx_UpdatePickables();

void ed_w_ctx_ClearSelections();

void ed_w_ctx_DeleteSelections();

void ed_w_ctx_AddSelection(struct ed_pickable_t *selection, uint32_t multiple_key_down);

void ed_w_ctx_DropSelection(struct ed_pickable_t *selection, struct ds_slist_t *pickable_list, struct ds_list_t *selection_list);

void ed_w_ctx_TranslateSelected(vec3_t *translation);

void ed_w_ctx_RotateSelected(mat3_t *rotation);

void ed_w_ctx_DrawWidgets();

void ed_w_ctx_DrawGrid();

void ed_w_ctx_DrawBrushes();

void ed_w_ctx_DrawLights();

void ed_w_ctx_DrawSelections(struct ds_list_t *selections, struct ds_slist_t *pickables);

void ed_w_ctx_PingInfoWindow();

void ed_w_ctx_Update();

void ed_w_ctx_Idle(struct ed_context_t *context, uint32_t just_changed);

void ed_w_ctx_FlyCamera(struct ed_context_t *context, uint32_t just_changed);

void ed_w_ctx_LeftClick(struct ed_context_t *context, uint32_t just_changed);

void ed_w_ctx_RightClick(struct ed_context_t *context, uint32_t just_changed);

void ed_w_ctx_BrushBox(struct ed_context_t *context, uint32_t just_changed);

void ed_w_ctx_WidgetSelected(struct ed_context_t *context, uint32_t just_changed);

void ed_w_ctx_ObjectSelected(struct ed_context_t *context, uint32_t just_changed);

void ed_w_ctx_EnterObjectEditMode(struct ed_context_t *context, uint32_t just_changed);

void ed_w_ctx_EnterBrushEditMode(struct ed_context_t *context, uint32_t just_changed);


#endif // ED_W_CTX_H
