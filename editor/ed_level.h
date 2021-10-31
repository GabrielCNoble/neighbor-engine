#ifndef ED_LEVEL_H
#define ED_LEVEL_H

#include "ed_pick.h"
#include "ed_level_defs.h"




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

void ed_w_CopySelections(struct ds_list_t *selections);

void ed_w_DeleteSelections();

void ed_w_TranslateSelected(vec3_t *translation, uint32_t transform_mode);

void ed_w_RotateSelected(mat3_t *rotation, vec3_t *pivot, uint32_t transform_mode);

void ed_w_MarkPickableModified(struct ed_pickable_t *pickable);

void ed_w_MarkBrushModified(struct ed_brush_t *brush);

/*
=============================================================
=============================================================
=============================================================
*/

void ed_w_UpdateUI();

void ed_w_UpdateManipulator();

void ed_w_UpdatePickableObjects();

void ed_w_Update();

/*
=============================================================
=============================================================
=============================================================
*/

void ed_w_DrawManipulator();

void ed_w_DrawWidgets();

void ed_w_DrawGrid();

void ed_w_DrawBrushes();

void ed_w_DrawLights();

void ed_w_DrawSelections();

void ed_w_PingInfoWindow();

uint32_t ed_w_IntersectPlaneFromCamera(float mouse_x, float mouse_y, vec3_t *plane_point, vec3_t *plane_normal, vec3_t *result);

void ed_w_PointPixelCoords(int32_t *x, int32_t *y, vec3_t *point);

void ed_w_Idle(struct ed_context_t *context, uint32_t just_changed);

void ed_w_FlyCamera(struct ed_context_t *context, uint32_t just_changed);

void ed_w_LeftClick(struct ed_context_t *context, uint32_t just_changed);

void ed_w_RightClick(struct ed_context_t *context, uint32_t just_changed);

void ed_w_BrushBox(struct ed_context_t *context, uint32_t just_changed);

void ed_w_PickObjectOrWidget(struct ed_context_t *context, uint32_t just_changed);

void ed_w_PickObject(struct ed_context_t *context, uint32_t just_changed);

void ed_w_PlaceLightAtCursor(struct ed_context_t *context, uint32_t just_changed);

void ed_w_TransformSelections(struct ed_context_t *context, uint32_t just_changed);

void ed_SerializeLevel(void **level_buffer, size_t *buffer_size);

void ed_DeserializeLevel(void *level_buffer, size_t buffer_size);

void ed_SaveLevel(char *path, char *file);

void ed_LoadLevel(char *path, char *file);

void ed_ResetLevelEditor();


#endif // ED_W_CTX_H
