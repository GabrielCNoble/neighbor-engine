#ifndef ED_LEVEL_H
#define ED_LEVEL_H

#include "ed_pick.h"
#include "ed_level_defs.h"




void ed_l_Init(struct ed_editor_t *editor);

void ed_l_Shutdown();

void ed_l_Suspend();

void ed_l_Resume();

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

void ed_LevelEditorTranslateSelected(vec3_t *translation, uint32_t transform_mode);

void ed_LevelEditorRotateSelected(mat3_t *rotation, vec3_t *pivot, uint32_t transform_mode);

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

void ed_l_Update();

/*
=============================================================
=============================================================
=============================================================
*/

void ed_LevelEditorDrawManipulator();

void ed_LevelEditorDrawWidgets();

void ed_LevelEditorDrawGrid();

//void ed_LevelEditorDrawBrushes();

void ed_LevelEditorDrawLights();

void ed_LevelEditorDrawSelections();

//void ed_w_PingInfoWindow();

uint32_t ed_w_IntersectPlaneFromCamera(float mouse_x, float mouse_y, vec3_t *plane_point, vec3_t *plane_normal, vec3_t *result);

void ed_w_PointPixelCoords(int32_t *x, int32_t *y, vec3_t *point);

void ed_LevelEditorIdle(uint32_t just_changed);

void ed_LevelEditorFlyCamera(uint32_t just_changed);

void ed_LevelEditorLeftClick(uint32_t just_changed);

void ed_LevelEditorRightClick(uint32_t just_changed);

void ed_LevelEditorBrushBox(uint32_t just_changed);

void ed_LevelEditorPickObjectOrWidget(uint32_t just_changed);

void ed_LevelEditorPickObject(uint32_t just_changed);

void ed_LevelEditorPlaceLightAtCursor(uint32_t just_changed);

void ed_LevelEditorTransformSelections(uint32_t just_changed);

void ed_SerializeLevel(void **level_buffer, size_t *buffer_size, uint32_t serialize_brushes);

void ed_DeserializeLevel(void *level_buffer, size_t buffer_size);

void ed_l_SaveLevel(char *path, char *file);

void ed_l_LoadFile(char *path, char *file);

void ed_l_ClearBrushEntities();

void ed_l_RestoreBrushEntities();

void ed_l_SaveGameLevelSnapshot();

void ed_l_LoadGameLevelSnapshot();

void ed_l_PlayGame();

void ed_l_StopGame();

void ed_l_ResetEditor();

void ed_l_BuildWorldData();

void ed_l_ClearWorldData();


#endif // ED_W_CTX_H
