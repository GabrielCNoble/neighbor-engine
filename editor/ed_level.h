#ifndef ED_LEVEL_H
#define ED_LEVEL_H

#include "ed_pick.h"
#include "ed_level_defs.h"




void ed_LevelEditorInit(struct ed_editor_t *editor);

void ed_LevelEditorShutdown();

void ed_LevelEditorSuspend();

void ed_LevelEditorResume();

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

void ed_LevelEditorUpdate();

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

void ed_w_Idle(uint32_t just_changed);

void ed_w_FlyCamera(uint32_t just_changed);

void ed_w_LeftClick(uint32_t just_changed);

void ed_w_RightClick(uint32_t just_changed);

void ed_w_BrushBox(uint32_t just_changed);

void ed_w_PickObjectOrWidget(uint32_t just_changed);

void ed_w_PickObject(uint32_t just_changed);

void ed_w_PlaceLightAtCursor(uint32_t just_changed);

void ed_w_TransformSelections(uint32_t just_changed);

void ed_SerializeLevel(void **level_buffer, size_t *buffer_size, uint32_t serialize_brushes);

void ed_DeserializeLevel(void *level_buffer, size_t buffer_size);

void ed_LevelEditorSaveLevel(char *path, char *file);

void ed_LevelEditorLoadLevel(char *path, char *file);

void ed_SaveGameLevelSnapshot();

void ed_LoadGameLevelSnapshot();

void ed_LevelEditorReset();

void ed_BuildWorldData();


#endif // ED_W_CTX_H
