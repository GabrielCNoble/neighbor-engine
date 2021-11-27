#ifndef ED_ENT_H
#define ED_ENT_H

#include "ed_ent_defs.h"

void ed_e_Init(struct ed_editor_t *editor);

void ed_e_Shutdown();

void ed_e_Suspend();

void ed_e_Resume();

uint32_t ed_e_EntDefHierarchyUI(struct e_ent_def_t *ent_def);

uint32_t ed_e_CollisionShapeUI(struct p_col_def_t *col_def);

void ed_e_UpdateUI();

void ed_e_Update();

void ed_e_ResetEditor();

void ed_e_SelectEntDef(struct e_ent_def_t *ent_def);

void ed_EntityEditorIdle(uint32_t just_changed);

void ed_EntityEditorFlyCamera(uint32_t just_changed);




void ed_SerializeEntDef(void **buffer, size_t *buffer_size, struct e_ent_def_t *ent_def);

void ed_SaveEntDef(char *file_name, struct e_ent_def_t *ent_def);


#endif // ED_ENT_H
