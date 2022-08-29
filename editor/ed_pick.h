#ifndef ED_PICK_H
#define ED_PICK_H

#include "ed_pick_defs.h"
#include "ed_brush_defs.h"
#include "../engine/r_defs.h"
#include "../engine/g_defs.h"

void ed_PickingInit();

void ed_PickingShutdown();

void ed_BeginPicking();

uint32_t ed_EndPicking(int32_t mouse_x, int32_t mouse_y, struct ed_pickable_t *result);

void ed_PickableModelViewProjectionMatrix(struct ed_pickable_t *pickable, mat4_t *parent_transform, mat4_t *model_view_projection_matrix);

//void ed_PickableRangeModelViewProjectionMatrix()

//void ed_DrawPickable(struct ed_pickable_t *pickable, mat4_t *parent_transform);

struct ed_pickable_t *ed_SelectPickable(int32_t mouse_x, int32_t mouse_y, struct ds_slist_t *pickables, mat4_t *parent_transform, uint32_t ignore_types);

struct ed_pickable_t *ed_SelectWidget(int32_t mouse_x, int32_t mouse_y, struct ed_widget_t *widget, mat4_t *widget_transform);

uint32_t ed_ClickOnManipulator(int32_t mouse_x, int32_t mouse_y, struct ed_manipulator_t *manipulator);


struct ed_widget_t *ed_CreateWidget();

void ed_DestroyWidget(struct ed_widget_t *widget);

void ed_DrawWidget(struct ed_widget_t *widget, mat4_t *widget_transform);

void ed_WidgetDefaultSetupPickableDrawState(uint32_t pickable_index, struct ed_pickable_t *pickable);

struct ed_pickable_range_t *ed_AllocPickableRange();

void ed_FreePickableRange(struct ed_pickable_range_t *range);

struct ds_slist_t *ed_PickableListFromType(uint32_t type);

struct ed_pickable_t *ed_CreatePickableOnList(uint32_t type, struct ds_slist_t *pickables);

struct ed_pickable_t *ed_CreatePickable(uint32_t type);

void ed_DestroyPickableOnList(struct ed_pickable_t *pickable, struct ds_slist_t *pickables);

void ed_DestroyPickable(struct ed_pickable_t *pickable);

struct ed_pickable_t *ed_GetPickableOnList(uint32_t index, struct ds_slist_t *pickables);

struct ed_pickable_t *ed_GetPickable(uint32_t index);

struct ed_pickable_t *ed_CopyPickable(struct ed_pickable_t *src_pickable);

//struct ed_pickable_t *ed_CreateBrushPickable(vec3_t *position, mat3_t *orientation, vec3_t *size, struct ed_brush_t *src_brush);

struct ed_pickable_t *ed_CreateLightPickable(vec3_t *pos, vec3_t *color, float radius, float energy, uint32_t type, struct r_light_t *src_light);

struct ed_pickable_t *ed_CreateEntityPickable(struct e_ent_def_t *ent_def, vec3_t *position, vec3_t *scale, mat3_t *orientation, struct e_entity_t *src_entity);

void ed_UpdateEntityPickableRanges(struct ed_pickable_t *pickable, struct e_entity_t *entity);

struct ed_pickable_t *ed_CreateEnemyPickable(uint32_t type, vec3_t *position, mat3_t *orientation, struct g_enemy_t *src_enemy);


#endif
