#ifndef ED_PICK_H
#define ED_PICK_H

#include "ed_com.h"
#include "r_com.h"


struct ed_pickable_t *ed_CreatePickable();

void ed_DestroyPickable(struct ed_pickable_t *pickable);

struct ed_pickable_t *ed_GetPickable(uint32_t index);

struct ed_pickable_t *ed_CreateBrushPickable(vec3_t *position, mat3_t *orientation, vec3_t *size);

struct ed_pickable_t *ed_CreateLightPickable(vec3_t *pos, vec3_t *color, float radius, float energy);

struct ed_pickable_t *ed_CreateEntityPickable(mat4_t *transform, struct r_model_t *model);

void ed_UpdatePickables();

struct ed_pickable_t *ed_SelectPickable(int32_t mouse_x, int32_t mouse_y);

void ed_ClearSelections();

void ed_AddSelection(struct ed_pickable_t *selection, uint32_t multiple_key_down);

void ed_TranslateSelected(vec3_t *translation);

void ed_RotateSelected(mat3_t *rotation);

#endif
