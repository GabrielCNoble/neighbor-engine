#ifndef ED_MAIN_H
#define ED_MAIN_H

#include "../lib/dstuff/ds_vector.h"
#include "../lib/dstuff/ds_matrix.h"
#include "../lib/dstuff/ds_alloc.h"
#include "../lib/dstuff/ds_buffer.h"
#include "ed_defs.h"
#include "ed_brush.h"
#include "ed_pick.h"
#include "ed_level.h"

void ed_Init();

void ed_Shutdown();

void ed_Quit();

struct ed_editor_t *ed_RegisterEditor(struct ed_editor_t *editor);

void ed_SwitchToEditor(struct ed_editor_t *editor);

void ed_UpdateEditor();

void ed_SetNextState(void (*state_fn)(uint32_t just_changed));

/*
=============================================================
=============================================================
=============================================================
*/

void ed_InitProjectFolder(char *path, char *folder_name);

/*
=============================================================
=============================================================
=============================================================
*/

void ed_UpdateExplorer();

void ed_OpenExplorer(char *path);

void ed_OpenExplorerSave();

void ed_OpenExplorerLoad();

void ed_OpenExplorerSelectFolder();

void ed_CloseExplorer();

void ed_EnumerateExplorerDrives();

void ed_ChangeExplorerPath(char *path);

void ed_AddExplorerExtFilter(char *ext_filter);

void ed_MatchExplorerEntries(char *match);

void ed_ClearExplorerExtFilters();

uint32_t ed_ExplorerSaveFile(char *path, char *file);

uint32_t ed_ExplorerLoadFile(char *path, char *file);

uint32_t ed_ExplorerSelectFolder(char *path, char *file);

#endif // ED_H





