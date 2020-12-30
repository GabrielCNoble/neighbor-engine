#include "world.h"
#include "dstuff/ds_dbvh.h"
#include "dstuff/ds_alloc.h"

struct dbvh_tree_t w_dbvh;
struct ds_chunk_h w_vert_chunk = DS_INVALID_CHUNK_HANDLE;
struct ds_chunk_h w_index_chunk = DS_INVALID_CHUNK_HANDLE;

void w_Init()
{
    w_dbvh = create_dbvh_tree(sizeof(struct w_face_t));
}

void w_Shutdown()
{
    
}

void w_FillGeometry(struct r_vert_t *verts, uint32_t vert_count, uint32_t *indices, uint32_t indice_count)
{
    if(w_vert_chunk.index != DS_INVALID_CHUNK_INDEX)
    {
        r_FreeVertices(w_vert_chunk);
        r_FreeIndices(w_index_chunk);
    }
}

void w_DrawWorld()
{
    
}
