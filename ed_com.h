#ifndef ED_COM_H
#define ED_COM_H

#include <stdint.h>
#include "dstuff/ds_buffer.h"
#include "dstuff/ds_vector.h"
#include "dstuff/ds_matrix.h"
#include "r_com.h"

struct ed_polygon_t
{
    struct ed_polygon_t *next;
    struct ed_polygon_t *prev;
    struct ds_buffer_t vertices;
    struct r_material_t *material;
    uint32_t index;
    vec3_t normal;
};

struct ed_bspn_t
{
    struct ed_bspn_t *front;
    struct ed_bspn_t *back;
    struct ed_polygon_t *splitter;
    uint32_t index;
};

struct ed_face_t
{
    struct r_material_t *material;
    struct ds_buffer_t indices;
    vec3_t normal;
    vec3_t tangent;
};

struct ed_brush_batch_t
{
    struct r_batch_t batch;
    uint32_t index;
};

struct ed_brush_t
{
    mat3_t orientation;
    vec3_t position;
    uint32_t index;
    struct ds_list_t faces;
    struct ds_buffer_t vertices;
    struct r_model_t *model;
    struct ed_bspn_t *bsp;
};

#endif // ED_COM_H
