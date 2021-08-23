#ifndef ED_BSP_H
#define ED_BSP_H

#include <stdint.h>
#include <stddef.h>
#include "ed_com.h"
#include "dstuff/ds_vector.h"
#include "dstuff/ds_list.h"
#include "dstuff/ds_buffer.h"

enum ED_SPLITTER_SIDES
{
    ED_SPLITTER_SIDE_FRONT = 0,
    ED_SPLITTER_SIDE_BACK = 1,
    ED_SPLITTER_SIDE_STRADDLE = 2,
    ED_SPLITTER_SIDE_ON_FRONT = 3,
    ED_SPLITTER_SIDE_ON_BACK = 4,
};

void ed_UnlinkPolygon(struct ed_polygon_t *polygon, struct ed_polygon_t **first_polygon);

uint32_t ed_PolygonOnSplitter(struct ed_polygon_t *polygon, vec3_t *point, vec3_t *normal);

struct ed_polygon_t *ed_BestSplitter(struct ed_polygon_t *polygons);

struct ed_bspn_t *ed_BrushBspFromPolygons(struct ed_polygon_t *polygons);

#endif // ED_BSP_H
