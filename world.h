#ifndef WORLD_H
#define WORLD_H

#include <stdint.h>
#include "r_main.h"



struct w_face_t
{
    uint32_t first_index;
    uint32_t batch_index;
};

void w_Init();

void w_Shutdown();

void w_InitGeometry(struct r_model_geometry_t *geometry);

void w_ClearGeometry();

//void w_DrawWorld();

#endif // WORLD_H
