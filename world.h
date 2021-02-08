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

void w_FillGeometry(struct r_vert_t *verts, uint32_t vert_count, uint32_t *indices, uint32_t indice_count);

//void w_VisibleLights();

//void w_VisibleEntities();

void w_DrawWorld();

#endif // WORLD_H
