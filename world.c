#include "world.h"
#include "dstuff/ds_dbvh.h"
#include "dstuff/ds_alloc.h"

struct dbvh_tree_t w_dbvh;
struct ds_chunk_h w_vert_chunk = DS_INVALID_CHUNK_HANDLE;
struct ds_chunk_h w_index_chunk = DS_INVALID_CHUNK_HANDLE;

extern mat4_t r_view_matrix;
extern mat4_t r_inv_view_matrix;
extern mat4_t r_view_projection_matrix;
extern struct stack_list_t r_lights;
extern struct r_cluster_t *r_clusters;
extern uint32_t r_light_buffer_cursor;
extern uint32_t r_light_index_buffer_cursor;
extern struct r_l_data_t *r_light_buffer;
extern uint16_t *r_light_index_buffer;

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

void w_VisibleLights()
{
    r_light_buffer_cursor = 0;
    r_light_index_buffer_cursor = 0;
    
    for(uint32_t light_index = 0; light_index < r_lights.cursor; light_index++)
    {
        struct r_light_t *light = r_GetLight(light_index);
        
        if(light)
        {
            struct r_l_data_t *data = r_light_buffer + r_light_buffer_cursor;
            r_light_buffer_cursor++;
            vec4_t light_pos = light->data.pos_rad;
            light_pos.w = 1.0;
            mat4_t_vec4_t_mul(&light_pos, &r_inv_view_matrix, &light_pos);
            light_pos.w = light->data.pos_rad.w;
            
            data->color_type = light->data.color_type;
            data->pos_rad = light_pos;
        }
    }
    
    for(uint32_t slice_index = 0; slice_index < R_CLUSTER_SLICES; slice_index++)
    {
        uint32_t slice_offset = slice_index * R_CLUSTER_ROWS * R_CLUSTER_ROW_WIDTH;
        
        for(uint32_t row_index = 0; row_index < R_CLUSTER_ROWS; row_index++)
        {
            uint32_t row_offset = row_index * R_CLUSTER_ROW_WIDTH;
            
            for(uint32_t cluster_index = 0; cluster_index < R_CLUSTER_ROW_WIDTH; cluster_index++)
            {
                struct r_cluster_t *cluster = r_clusters + cluster_index + row_offset + slice_offset;
                cluster->start = r_light_index_buffer_cursor;
                
                for(uint32_t light_index = 0; light_index < r_lights.cursor; light_index++)
                {
                    struct r_light_t *light = r_GetLight(light_index);
                    if(light)
                    {
                        r_light_index_buffer[r_light_index_buffer_cursor] = light_index;
                        r_light_index_buffer_cursor++;
                    }
                }
                
                cluster->count = r_light_index_buffer_cursor - cluster->start;
            }
        }
    }
}

void w_VisibleEntities()
{
    
}

void w_DrawWorld()
{
    
}
