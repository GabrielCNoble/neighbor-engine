#ifndef G_BARRIER_H
#define G_BARRIER_H

#include "g_defs.h"

void g_BarrierInit();

struct g_barrier_t *g_CreateBarrier(uint32_t type, uint32_t sub_type, vec3_t *position, mat3_t *orientation);

struct g_barrier_t *g_GetBarrier(uint32_t index);

void g_DestroyBarrier(struct g_barrier_t *barrier);

void g_UnlockBarrier(struct g_barrier_t *barrier, struct g_key_t *key);




#endif // G_BARRIER_H
