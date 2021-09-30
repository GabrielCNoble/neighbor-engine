#ifndef ED_WORLD_H
#define ED_WORLD_H

#include "ed_com.h"
#include "ed_pick.h"

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

    struct
    {
        vec3_t box_start;
        vec3_t box_end;
        vec3_t plane_point;
        vec2_t box_size;
        mat3_t plane_orientation;
        uint32_t drawing;

        struct ds_slist_t bsp_nodes;
        struct ds_slist_t bsp_polygons;

        struct ds_slist_t brushes;
        struct ds_slist_t brush_faces;
        struct ds_slist_t brush_face_polygons;
        struct ds_slist_t brush_edges;

        uint32_t brush_vert_count;
        uint32_t brush_index_count;
        struct ds_list_t brush_batches;
    } brush;

    struct
    {
        uint32_t edit_mode;
        struct ed_w_ctx_object_list_t *active_list;
        struct ed_w_ctx_object_list_t lists[ED_W_CTX_EDIT_MODE_LAST];
        struct ed_pickable_t *last_selected;

    } pickables;

    struct ds_slist_t pickable_ranges;
    struct ds_slist_t widgets;

    struct
    {
        uint32_t mode;
        uint32_t visible;
        float prev_angle;
        vec3_t start_pos;
        vec3_t prev_offset;
        mat4_t transform;
        float linear_snap;
        float angular_snap;
        struct ed_widget_t *widgets[3];

    } manipulator;

    struct ed_widget_t *ball_widget;
    mat4_t ball_transform;

    float info_window_alpha;
    uint32_t open_delete_selections_popup;

    float camera_pitch;
    float camera_yaw;
    vec3_t camera_pos;
};

void ed_w_Init();

void ed_w_Shutdown();

void ed_w_ManipulatorWidgetSetupPickableDrawState(uint32_t pickable_index, struct ed_pickable_t *pickable);

/*
=============================================================
=============================================================
=============================================================
*/

void ed_w_AddSelection(struct ed_pickable_t *selection, uint32_t multiple_key_down, struct ds_list_t *selections);

void ed_w_DropSelection(struct ed_pickable_t *selection, struct ds_list_t *selections);

void ed_w_ClearSelections(struct ds_list_t *selections);

void ed_w_DeleteSelections();

void ed_w_TranslateSelected(vec3_t *translation, uint32_t transform_mode);

void ed_w_RotateSelected(mat3_t *rotation, vec3_t *pivot, uint32_t transform_mode);

/*
=============================================================
=============================================================
=============================================================
*/

void ed_w_UpdateUI();

void ed_w_UpdateManipulator();

void ed_w_UpdatePickables();

/*
=============================================================
=============================================================
=============================================================
*/

void ed_w_DrawManipulator();

void ed_w_DrawWidgets();

void ed_w_DrawGrid();

void ed_w_DrawBrushes();

void ed_wx_DrawLights();

void ed_w_DrawSelections(struct ds_list_t *selections, struct ds_slist_t *pickables);

void ed_w_PingInfoWindow();

void ed_w_Update();

uint32_t ed_w_IntersectPlaneFromCamera(float mouse_x, float mouse_y, vec3_t *plane_point, vec3_t *plane_normal, vec3_t *result);

void ed_w_PointPixelCoords(int32_t *x, int32_t *y, vec3_t *point);

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
