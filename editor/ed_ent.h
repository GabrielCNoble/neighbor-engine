#ifndef ED_ENT_H
#define ED_ENT_H

#include "ed_ent_defs.h"

void ed_e_Init(struct ed_editor_t *editor);

void ed_e_Shutdown();

void ed_e_Suspend();

void ed_e_Resume();

uint32_t ed_e_SaveEntDef(char *path, char *file);

uint32_t ed_e_LoadEntDef(char *path, char *file);

struct e_constraint_t *ed_e_GetConstraint(struct e_ent_def_t *child_def, struct e_ent_def_t *parent_def);

void ed_e_AddConstraint(struct e_ent_def_t *child_def, struct e_ent_def_t *parent_def);

void ed_e_RemoveConstraint(struct e_constraint_t *constraint, struct e_ent_def_t *parent_def);

void ed_e_AddEntDefChild(struct e_ent_def_t *parent_def);

void ed_e_RemoveEntDefChild(struct e_ent_def_t *child_def, struct e_ent_def_t *parent_def);

uint32_t ed_e_EntDefHierarchyUI(struct e_ent_def_t *ent_def, struct e_ent_def_t *parent_def);

uint32_t ed_e_CollisionShapeUI(struct p_col_def_t *col_def);

void ed_e_UpdateUI();

void ed_e_Update();

void ed_e_ResetEditor();

void ed_e_SelectEntDef(struct e_ent_def_t *ent_def);

void ed_EntityEditorIdle(uint32_t just_changed);

void ed_EntityEditorFlyCamera(uint32_t just_changed);




void ed_e_SerializeEntDef(void **buffer, size_t *buffer_size, struct e_ent_def_t *ent_def);

//void ed_e_SaveEntDef(char *file_name, struct e_ent_def_t *ent_def);


#endif // ED_ENT_H
