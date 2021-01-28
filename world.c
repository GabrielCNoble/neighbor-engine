#include "world.h"
#include "dstuff/ds_dbvh.h"
#include "dstuff/ds_alloc.h"
#include "r_draw.h"

struct dbvh_tree_t w_dbvh;
struct ds_chunk_h w_vert_chunk = DS_INVALID_CHUNK_HANDLE;
struct ds_chunk_h w_index_chunk = DS_INVALID_CHUNK_HANDLE;
struct list_t w_visible_lights;

extern mat4_t r_view_matrix;
extern mat4_t r_inv_view_matrix;
extern mat4_t r_view_projection_matrix;
extern mat4_t r_projection_matrix;
extern struct stack_list_t r_lights;
extern struct r_cluster_t *r_clusters;
extern uint32_t r_light_buffer_cursor;
extern uint32_t r_light_index_buffer_cursor;
extern struct r_l_data_t *r_light_buffer;
extern uint32_t *r_light_index_buffer;
extern float r_z_near;
extern float r_z_far;
extern float r_fov;

void w_Init()
{
    w_dbvh = create_dbvh_tree(sizeof(struct w_face_t));
    w_visible_lights = create_list(sizeof(uint32_t), 512);
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
    w_visible_lights.cursor = 0;
    
    vec3_t axes[2] = {vec3_t_c(1.0, 0.0, 0.0), vec3_t_c(0.0, 1.0, 0.0)};
    vec2_t extents[2];
    mat4_t transform;
    mat4_t_identity(&transform);
    
    r_i_SetTransform(&transform);
    r_i_SetPrimitiveType(GL_LINES);
    
    float tan2theta = 2.0 * tan(r_fov);
//    float denom = log(1.0 + tan2theta / (float)R_CLUSTER_ROWS);
    float denom = log(r_z_far / r_z_near);
    
    for(uint32_t light_index = 0; light_index < r_lights.cursor; light_index++)
    {
        struct r_light_t *light = r_GetLight(light_index);
        
        if(light)
        {
            struct r_l_data_t *data = r_light_buffer + r_light_buffer_cursor;
            vec4_t pos_rad = vec4_t_c(light->data.pos_rad.x, light->data.pos_rad.y, light->data.pos_rad.z, 1.0);
            mat4_t_vec4_t_mul(&pos_rad, &r_inv_view_matrix, &pos_rad);
            
            vec3_t light_pos = vec3_t_c(pos_rad.x, pos_rad.y, pos_rad.z);
            float sqrd_radius = light->data.pos_rad.w * light->data.pos_rad.w;
            float sol = 0.0;
            
            if(light_pos.z - light->data.pos_rad.w > -r_z_near)
            {
                /* light completely behind the near plane */
                continue;
            }
            else
            {
                float near_dist = light_pos.z + light->data.pos_rad.w;
                
                if(near_dist > 0.0)
                {
                    /* camera is inside sphere, so it covers the whole screen */
                    extents[0].x = -1.0;
                    extents[0].y = 1.0;
                    extents[1].x = -1.0;
                    extents[1].y = 1.0;
                }
                else
                {   
                    if(near_dist >= -r_z_near) 
                    {
                        /* light touches near plane, so compute the intersection with it */
                        sol = sqrt(fabs(sqrd_radius - (r_z_near - light_pos.z)));
                    }
                    
                    for(uint32_t axis_index = 0; axis_index < 2; axis_index++)
                    {
                        vec2_t light_vec;
                        float light_a = vec3_t_dot(&axes[axis_index], &light_pos);
                        light_vec.x = light_a;
                        light_vec.y = light_pos.z;
                        float center_dist = vec2_t_length(&light_vec);
                        float tangent_dist = sqrt(fabs(center_dist * center_dist - sqrd_radius));
                        vec2_t_mul(&light_vec, &light_vec, 1.0 / center_dist);
                        
                        float cos_theta = tangent_dist / center_dist;
                        float sin_theta = light->data.pos_rad.w / center_dist;
                        float scale = r_projection_matrix.rows[axis_index].comps[axis_index];
                        vec2_t *extent = extents + axis_index;
                        vec2_t b;
                        vec2_t t;
                        
                        t.x = (cos_theta * light_vec.x + sin_theta * light_vec.y) * tangent_dist;
                        t.y = (-sin_theta * light_vec.x + cos_theta * light_vec.y) * tangent_dist;
                        b.x = (cos_theta * light_vec.x - sin_theta * light_vec.y) * tangent_dist;
                        b.y = (sin_theta * light_vec.x + cos_theta * light_vec.y) * tangent_dist;
                        
                        if(b.y >= -r_z_near)
                        {
                            /* tangent point behind near plane */
                            b.x = light_a + sol;
                            b.y = -r_z_near;
                        }
                        
                        if(t.y >= -r_z_near)
                        {
                            /* tangent point behind near plane */
                            t.x = light_a - sol;
                            t.y = -r_z_near;
                        }
                        
                        t.x = (t.x * scale) / -t.y;
                        b.x = (b.x * scale) / -b.y;
                        
                        extent->x = fmin(fmax(t.x, -1.0), 1.0);
                        extent->y = fmin(fmax(b.x, -1.0), 1.0);
                    }
                }
            }
            
            if((extents[0].x - extents[0].y) * (extents[1].x - extents[1].y) == 0.0)
            {
                continue;
            }
            
            light->min_x = (uint32_t)(R_CLUSTER_ROW_WIDTH * (extents[0].x * 0.5 + 0.5));
            light->max_x = (uint32_t)(R_CLUSTER_ROW_WIDTH * (extents[0].y * 0.5 + 0.5));
            if(light->max_x > R_CLUSTER_MAX_X) light->max_x = R_CLUSTER_MAX_X;
            
            light->min_y = (uint32_t)(R_CLUSTER_ROWS * (extents[1].x * 0.5 + 0.5));
            light->max_y = (uint32_t)(R_CLUSTER_ROWS * (extents[1].y * 0.5 + 0.5));
            if(light->max_y > R_CLUSTER_MAX_Y) light->max_y = R_CLUSTER_MAX_Y;
            
            float num = log(fmax(-light_pos.z - light->data.pos_rad.w, r_z_near) / r_z_near);
            light->min_z = (uint32_t)floorf(R_CLUSTER_ROWS * (num / denom));
            num = log(fmax(-light_pos.z + light->data.pos_rad.w, r_z_near) / r_z_near);
            light->max_z = (uint32_t)floorf(R_CLUSTER_ROWS * (num / denom));
            
            if(light->max_z > R_CLUSTER_MAX_Z)
            {
                light->max_z = R_CLUSTER_MAX_Z;
            }
            
            light->gpu_index = r_light_buffer_cursor;
            add_list_element(&w_visible_lights, &light_index);
            r_light_buffer_cursor++;
            
//            vec3_t corners[4];
//            corners[0] = vec3_t_c(extents[0].x, extents[1].y, -r_z_near);
//            corners[1] = vec3_t_c(extents[0].x, extents[1].x, -r_z_near);
//            corners[2] = vec3_t_c(extents[0].y, extents[1].x, -r_z_near);
//            corners[3] = vec3_t_c(extents[0].y, extents[1].y, -r_z_near);
//            
//            r_i_DrawLine(&corners[0], &corners[1], &vec3_t_c(0.0, 1.0, 0.0), 1.0);
//            r_i_DrawLine(&corners[1], &corners[2], &vec3_t_c(0.0, 1.0, 0.0), 1.0);
//            r_i_DrawLine(&corners[2], &corners[3], &vec3_t_c(0.0, 1.0, 0.0), 1.0);
//            r_i_DrawLine(&corners[3], &corners[0], &vec3_t_c(0.0, 1.0, 0.0), 1.0);
            
            vec3_t_mul(&data->color, &light->data.color, light->energy);
            data->type = light->data.type;
            data->pos_rad.x = light_pos.x;
            data->pos_rad.y = light_pos.y;
            data->pos_rad.z = light_pos.z;
            data->pos_rad.w = light->data.pos_rad.w;
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
                uint32_t cluster_offset = cluster_index + row_offset + slice_offset;
                struct r_cluster_t *cluster = r_clusters + cluster_offset;
                cluster->start = cluster_offset * R_MAX_CLUSTER_LIGHTS;
                cluster->count = 0;
            }
        }
    }
    
    for(uint32_t visible_index = 0; visible_index < w_visible_lights.cursor; visible_index++)
    {
        uint32_t light_index = *(uint32_t *)get_list_element(&w_visible_lights, visible_index);
        struct r_light_t *light = r_GetLight(light_index);
        
        for(uint32_t slice_index = light->min_z; slice_index <= light->max_z; slice_index++)
        {
            uint32_t slice_offset = slice_index * R_CLUSTER_ROWS * R_CLUSTER_ROW_WIDTH;
            
            for(uint32_t row_index = light->min_y; row_index <= light->max_y; row_index++)
            {
                uint32_t row_offset = row_index * R_CLUSTER_ROW_WIDTH;
                
                for(uint32_t cluster_index = light->min_x; cluster_index <= light->max_x; cluster_index++)
                {
                    uint32_t cluster_offset = cluster_index + row_offset + slice_offset;
                    struct r_cluster_t *cluster = r_clusters + cluster_offset;
                    r_light_index_buffer[cluster->start + cluster->count] = light->gpu_index;
                    cluster->count++;
                }
            }
        }
    }
    
    
        
//    for(uint32_t slice_index = 0; slice_index < R_CLUSTER_SLICES; slice_index++)
//    {
//        uint32_t slice_offset = slice_index * R_CLUSTER_ROWS * R_CLUSTER_ROW_WIDTH;
//        
//        for(uint32_t row_index = 0; row_index < R_CLUSTER_ROWS; row_index++)
//        {
//            uint32_t row_offset = row_index * R_CLUSTER_ROW_WIDTH;
//            
//            for(uint32_t cluster_index = 0; cluster_index < R_CLUSTER_ROW_WIDTH; cluster_index++)
//            {
//                struct r_cluster_t *cluster = r_clusters + cluster_index + row_offset + slice_offset;
//                cluster->start = r_light_index_buffer_cursor;
//                
//                for(uint32_t visible_index = 0; visible_index < w_visible_lights.cursor; visible_index++)
//                {
//                    uint32_t light_index = *(uint32_t *)get_list_element(&w_visible_lights, visible_index);
//                    struct r_light_t *light = r_GetLight(light_index);
//                    
//                    if(light->min_x <= cluster_index && light->max_x >= cluster_index)
//                    {
//                        if(light->min_y <= row_index && light->max_y >= row_index)
//                        {
//                            if(light->min_z <= slice_index && light->max_z >= slice_index)
//                            {
//                                r_light_index_buffer[r_light_index_buffer_cursor] = light->gpu_index;
//                                r_light_index_buffer_cursor++;
//                            }
//                        }
//                    }
//                }
//                
//                cluster->count = r_light_index_buffer_cursor - cluster->start;
//            }
//        }
//    }
}

void w_VisibleEntities()
{
    
}

void w_DrawWorld()
{
    
}
