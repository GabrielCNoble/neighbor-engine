#include "world.h"
#include "dstuff/ds_dbvt.h"
#include "dstuff/ds_alloc.h"
#include "r_draw.h"

struct r_model_t *w_world_model;
struct ds_dbvn_t w_world_dbvt;

void w_Init()
{

}

void w_Shutdown()
{

}

void w_InitGeometry(struct r_model_geometry_t *geometry)
{
    w_world_model = r_CreateModel(geometry, NULL);
}

void w_ClearGeometry()
{
    r_DestroyModel(w_world_model);
    w_world_model = NULL;
}

//void w_DrawWorld()
//{
//
//}
