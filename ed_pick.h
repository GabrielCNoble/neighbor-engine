#ifndef ED_PICK_H
#define ED_PICK_H

#include "ed_com.h"
#include "r_com.h"



enum ED_PICKABLE_TYPE
{
    ED_PICKABLE_TYPE_BRUSH = 1,
    ED_PICKABLE_TYPE_ENTITY,
    ED_PICKABLE_TYPE_LIGHT,
    ED_PICKABLE_TYPE_FACE,
    ED_PICKABLE_TYPE_EDGE,
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

enum ED_PICKABLE_TRANSFORM_FLAGS
{
    ED_PICKABLE_TRANSFORM_FLAG_TRANSLATION = 1,
    ED_PICKABLE_TRANSFORM_FLAG_ROTATION = 1 << 1,
    ED_PICKABLE_TRANSFORM_FLAG_SCALING = 1 << 2,
};

struct ed_pickable_t
{
    uint32_t type;
    uint32_t index;
    struct ds_slist_t *list;
    uint32_t selection_index;

    uint32_t transform_flags;

    vec3_t translation;
    vec3_t scale;
    mat3_t rotation;

    mat4_t transform;
    mat4_t draw_offset;

    uint32_t mode;
    uint32_t range_count;
    struct ed_pickable_range_t *ranges;

    uint32_t primary_index;
    uint32_t secondary_index;
};

//struct ed_pick_result_t
//{
//    uint32_t type;
//    uint32_t index;
//};

struct ed_widget_t
{
    uint32_t index;
    uint32_t stencil_layer;
    struct ds_slist_t pickables;

    void (*compute_model_view_projection_matrix)(mat4_t *view_projection_matrix, mat4_t *widget_transform);
    void (*setup_pickable_draw_state)(uint32_t pickable_index, struct ed_pickable_t *pickable);
};


void ed_BeginPicking(mat4_t *view_projection_matrix);

uint32_t ed_EndPicking(int32_t mouse_x, int32_t mouse_y, struct ed_pickable_t *result);

void ed_DrawPickable(struct ed_pickable_t *pickable, mat4_t *parent_transform);

struct ed_pickable_t *ed_SelectPickable(int32_t mouse_x, int32_t mouse_y, struct ds_slist_t *pickables, mat4_t *parent_transform, mat4_t *view_projection_matrix);

struct ed_pickable_t *ed_SelectWidget(int32_t mouse_x, int32_t mouse_y, struct ed_widget_t *widget, mat4_t *widget_transform);



struct ed_widget_t *ed_CreateWidget();

void ed_DestroyWidget(struct ed_widget_t *widget);

void ed_DrawWidget(struct ed_widget_t *widget, mat4_t *widget_transform);

void ed_WidgetDefaultComputeModelViewProjectionMatrix(mat4_t *view_projection_matrix, mat4_t *widget_transform);

void ed_WidgetDefaultSetupPickableDrawState(uint32_t pickable_index, struct ed_pickable_t *pickable);

struct ed_pickable_range_t *ed_AllocPickableRange();

void ed_FreePickableRange(struct ed_pickable_range_t *range);

struct ds_slist_t *ed_PickableListFromType(uint32_t type);

struct ed_pickable_t *ed_CreatePickableOnList(uint32_t type, struct ds_slist_t *pickables);

struct ed_pickable_t *ed_CreatePickable(uint32_t type);

void ed_DestroyPickableOnList(struct ed_pickable_t *pickable, struct ds_slist_t *pickables);

void ed_DestroyPickable(struct ed_pickable_t *pickable);

struct ed_pickable_t *ed_GetPickableOnList(uint32_t index, struct ds_slist_t *pickables);

struct ed_pickable_t *ed_GetPickable(uint32_t index, uint32_t type);

struct ed_pickable_t *ed_CreateBrushPickable(vec3_t *position, mat3_t *orientation, vec3_t *size);

struct ed_pickable_t *ed_CreateLightPickable(vec3_t *pos, vec3_t *color, float radius, float energy);

struct ed_pickable_t *ed_CreateEntityPickable(mat4_t *transform, struct r_model_t *model);


#endif
