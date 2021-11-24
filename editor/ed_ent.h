#ifndef ED_ENT_H
#define ED_ENT_H

#include "ed_ent_defs.h"

void ed_EntityEditorInit(struct ed_editor_t *editor);

void ed_EntityEditorShutdown();

void ed_EntityEditorSuspend();

void ed_EntityEditorResume();

void ed_EntityEditorUpdate();

void ed_EntityEditorReset();

void ed_EntityEditorIdle(uint32_t just_changed);

void ed_EntityEditorFlyCamera(uint32_t just_changed);




void ed_SerializeEntDef(void **buffer, size_t *buffer_size, struct e_ent_def_t *ent_def);

void ed_SaveEntDef(char *file_name, struct e_ent_def_t *ent_def);


#endif // ED_ENT_H
