#ifndef ED_PICK_H
#define ED_PICK_H

#include "ed_com.h"
#include "r_com.h"

void ed_BeginPicking();

uint32_t ed_EndPicking(int32_t mouse_x, int32_t mouse_y, struct ed_pick_result_t *result);

void ed_DrawPickable(struct ed_pickable_t *pickable, mat4_t *parent_transform);

struct ed_pickable_t *ed_SelectPickable(int32_t mouse_x, int32_t mouse_y, struct ds_slist_t *pickables, mat4_t *parent_transform);

#endif
