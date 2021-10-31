#ifndef ED_MAIN_H
#define ED_MAIN_H

#include "../lib/dstuff/ds_vector.h"
#include "../lib/dstuff/ds_matrix.h"
#include "../lib/dstuff/ds_alloc.h"
#include "../lib/dstuff/ds_buffer.h"
#include "ed_defs.h"
#include "ed_brush.h"
#include "ed_pick.h"

void ed_Init();

void ed_Shutdown();

void ed_UpdateEditor();

void ed_SetNextContextState(struct ed_context_t *context, void (*state_fn)(struct ed_context_t *context, uint32_t just_changed));

/*
=============================================================
=============================================================
=============================================================
*/

void ed_UpdateExplorer();

void ed_OpenExplorer(char *path, uint32_t mode);

void ed_CloseExplorer();

void ed_EnumerateExplorerDrives();

void ed_ChangeExplorerPath(char *path);

void ed_AddExplorerExtFilter(char *ext_filter);

void ed_MatchExplorerEntries(char *match);

void ed_ClearExplorerExtFilters();

void ed_SetExplorerLoadCallback(void (*load_callback)(char *path, char *file));

void ed_SetExplorerSaveCallback(void (*save_callback)(char *path, char *file));

/*
=============================================================
=============================================================
=============================================================
*/

//void ed_FlyCamera();
//
//void ed_DrawGrid();
//
//void ed_DrawBrushes();
//
//void ed_DrawLights();
//
//void ed_DrawSelections();

//void ed_SetContextState(struct ed_context_t *context, uint32_t state);

//void ed_WorldContextUpdate();
//
//void ed_WorldContextIdleState(struct ed_context_t *context, uint32_t just_changed);
//
//void ed_WorldContextLeftClickState(struct ed_context_t *context, uint32_t just_changed);
//
//void ed_WorldContextStateBrushBox(struct ed_context_t *context, uint32_t just_changed);
//
//void ed_WorldContextCreateBrush(struct ed_context_t *context, uint32_t just_changed);
//
//void ed_WorldContextProcessSelection(struct ed_context_t *context, uint32_t just_changed);

//uint32_t ed_PickObject(int32_t mouse_x, int32_t mouse_y, struct ed_selection_t *selection);

#endif // ED_H





