#ifndef ED_PROJ_H
#define ED_PROJ_H

#include "ed_defs.h"

void ed_ProjEditorInit(struct ed_editor_t *editor);

void ed_ProjEditorShutdown();

void ed_ProjEditorSuspend();

void ed_ProjEditorResume();

void ed_ProjEditorUpdate();

void ed_ProjEditorIdle();

void ed_ProjNewProject();

#endif // ED_START_H
