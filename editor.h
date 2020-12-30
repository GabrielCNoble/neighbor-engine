#ifndef EDITOR_H
#define EDITOR_H

#include "dstuff/ds_vector.h"
#include "dstuff/ds_matrix.h"
#include "dstuff/ds_alloc.h"

struct ed_brush_t
{
    mat3_t orientation;
    vec3_t position;
    vec3_t size;
    struct ds_chunk_h vert_chunk;
    struct ds_chunk_h index_chunk;
};


void ed_Init();

void ed_Shutdown();

void ed_UpdateEditor();

void ed_DrawBrushes();

void ed_CreateBrush(vec3_t *position, mat3_t *orientation, vec3_t *size);

#endif // ED_H
