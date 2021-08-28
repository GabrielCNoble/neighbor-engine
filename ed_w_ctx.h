#ifndef ED_W_CTX_H
#define ED_W_CTX_H

#include "ed_com.h"

enum ED_WORLD_CONTEXT_LISTS
{
    ED_WORLD_CONTEXT_LIST_OBJECTS = 0,
    ED_WORLD_CONTEXT_LIST_BRUSH_PARTS,
    ED_WORLD_CONTEXT_LIST_WIDGETS
};

void ed_w_ctx_Init();

void ed_w_ctx_Shutdown();

void ed_w_ctx_FlyCamera();

struct ds_slist_t *ed_w_ctx_PickableListFromType(uint32_t type);

struct ed_pickable_t *ed_w_ctx_CreatePickable(uint32_t type);

void ed_w_ctx_DestroyPickable(struct ed_pickable_t *pickable);

struct ed_pickable_t *ed_w_ctx_GetPickable(uint32_t index, uint32_t type);

struct ed_pickable_t *ed_w_ctx_CreateBrushPickable(vec3_t *position, mat3_t *orientation, vec3_t *size);

struct ed_pickable_t *ed_w_ctx_CreateLightPickable(vec3_t *pos, vec3_t *color, float radius, float energy);

struct ed_pickable_t *ed_w_ctx_CreateEntityPickable(mat4_t *transform, struct r_model_t *model);

void ed_w_ctx_UpdateUI();

void ed_w_ctx_UpdatePickables();

void ed_w_ctx_ClearSelections();

void ed_w_ctx_DeleteSelections();

void ed_w_ctx_AddSelection(struct ed_pickable_t *selection, uint32_t multiple_key_down);

void ed_w_ctx_TranslateSelected(vec3_t *translation);

void ed_w_ctx_RotateSelected(mat3_t *rotation);

void ed_w_ctx_DrawGrid();

void ed_w_ctx_DrawBrushes();

void ed_w_ctx_DrawLights();

void ed_w_ctx_DrawSelections(struct ds_list_t *selections, struct ds_slist_t *pickables);

void ed_w_ctx_PingInfoWindow();

void ed_w_ctx_Update();

void ed_w_ctx_Idle(struct ed_context_t *context, uint32_t just_changed);

void ed_w_ctx_LeftClick(struct ed_context_t *context, uint32_t just_changed);

void ed_w_ctx_RightClick(struct ed_context_t *context, uint32_t just_changed);

void ed_w_ctx_BrushBox(struct ed_context_t *context, uint32_t just_changed);

void ed_w_ctx_WidgetSelected(struct ed_context_t *context, uint32_t just_changed);

void ed_w_ctx_ObjectSelected(struct ed_context_t *context, uint32_t just_changed);

void ed_w_ctx_EnterObjectEditMode(struct ed_context_t *context, uint32_t just_changed);

void ed_w_ctx_EnterBrushEditMode(struct ed_context_t *context, uint32_t just_changed);


#endif // ED_W_CTX_H
