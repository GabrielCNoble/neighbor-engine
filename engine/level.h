#ifndef LEVEL_H
#define LEVEL_H

#include "l_defs.h"
#include "r_main.h"

struct w_face_t
{
    uint32_t first_index;
    uint32_t batch_index;
};

struct l_next_level_t
{
    struct r_model_t *world_model;
//    struct ds_dbvn_t world_dbvt;
    struct ds_slist_t entities;
};

void l_Init();

void l_Shutdown();

void l_InitGeometry(struct r_model_geometry_t *geometry);

void l_ClearGeometry();

void l_DeserializeLevel(void *level_buffer, size_t buffer_size);

#endif
