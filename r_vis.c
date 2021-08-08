#include "r_vis.h"
#include "game.h"

extern struct ds_list_t r_visible_lights;
extern mat4_t r_camera_matrix;
extern mat4_t r_view_matrix;
extern mat4_t r_view_projection_matrix;
extern mat4_t r_projection_matrix;
extern struct ds_slist_t r_lights;
extern struct ds_slist_t r_vis_items;
extern struct r_cluster_t *r_clusters;

extern struct r_l_data_t *r_light_buffer;
extern uint32_t r_light_buffer_cursor;

extern uint32_t *r_light_index_buffer;
extern uint32_t r_light_index_buffer_cursor;

extern uint32_t *r_shadow_index_buffer;
extern uint32_t  r_shadow_index_buffer_cursor;
//extern mat4_t r_point_shadow_projection_matrices[6];
//extern mat4_t r_point_shadow_view_matrices[6];
extern mat4_t r_point_shadow_view_projection_matrices[6];
extern vec3_t r_point_shadow_frustum_planes[6];
extern uint16_t r_point_shadow_frustum_masks[6];

extern float r_z_near;
extern float r_z_far;
extern float r_denom;
extern float r_fov;

extern struct ds_slist_t g_entities;

int32_t r_CompareVisibleLights(void *a, void *b)
{
    struct r_light_t *light_a = *(struct r_light_t **)a;
    struct r_light_t *light_b = *(struct r_light_t **)b;

    if(light_a->data.color_res.w > light_b->data.color_res.w) return 1;
    if(light_a->data.color_res.w < light_b->data.color_res.w) return -1;
    return 0;
}

void r_VisibleLights()
{
    r_light_buffer_cursor = 0;
    r_light_index_buffer_cursor = 0;
    r_shadow_index_buffer_cursor = 0;

    r_visible_lights.cursor = 0;

    vec3_t axes[2] = {vec3_t_c(1.0, 0.0, 0.0), vec3_t_c(0.0, 1.0, 0.0)};
    vec2_t extents[2];
    mat4_t transform;
    mat4_t_identity(&transform);

    for(uint32_t light_index = 0; light_index < r_lights.cursor; light_index++)
    {
        struct r_light_t *light = r_GetLight(light_index);

        if(light)
        {
            struct r_l_data_t *data = r_light_buffer + r_light_buffer_cursor;
            vec4_t pos_rad = vec4_t_c(light->data.pos_rad.x, light->data.pos_rad.y, light->data.pos_rad.z, 1.0);
            mat4_t_vec4_t_mul(&pos_rad, &r_view_matrix, &pos_rad);

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
                if(light_pos.z + light->data.pos_rad.w >= -r_z_near)
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
                    uint32_t camera_inside = (vec2_t_dot(&light_vec, &light_vec) - sqrd_radius) <= 0.0;
                    float center_dist = vec2_t_length(&light_vec);
                    float scale = r_projection_matrix.rows[axis_index].comps[axis_index];
                    vec2_t *extent = extents + axis_index;
                    vec2_t b;
                    vec2_t t;

                    if(camera_inside)
                    {
                        /* camera is inside the light, so tangent points will be the
                        intersections with the near plane */
                        t.x = light_a + sol;
                        t.y = -r_z_near;

                        b.x = light_a - sol;
                        b.y = -r_z_near;
                    }
                    else
                    {
                        float tangent_dist = sqrt(fabs(center_dist * center_dist - sqrd_radius));
                        float cos_theta = tangent_dist / center_dist;
                        float sin_theta = light->data.pos_rad.w / center_dist;

                        t.x = (cos_theta * light_vec.x - sin_theta * light_vec.y);
                        t.y = (sin_theta * light_vec.x + cos_theta * light_vec.y);
                        b.x = (cos_theta * light_vec.x + sin_theta * light_vec.y);
                        b.y = (-sin_theta * light_vec.x + cos_theta * light_vec.y);

                        /* let 'light_vec' be the vector from the camera to the center of the
                        sphere. The paper says that the tangent points are this vector, normalized,
                        rotated by an angle theta and -theta, then multiplied by the tangent distance.
                        To normalized 'light_vec' it's required to multiply it by 1.0 / length, and to
                        have the tangent point, it's required to multiply the rotated, normalized vector
                        by 'tangent_dist'. So, both things get done with the same multiplication by
                        multiplying the rotated unnormalized vector by 'tangent_dist' / 'center_dist',
                        or in other words, 'cos_theta'. Neat trick :) */
                        vec2_t_mul(&t, &t, cos_theta);
                        vec2_t_mul(&b, &b, cos_theta);

                        if(b.y >= -r_z_near)
                        {
                            /* tangent point behind near plane */
                            b.x = light_a - sol;
                            b.y = -r_z_near;
                        }

                        if(t.y >= -r_z_near)
                        {
                            /* tangent point behind near plane */
                            t.x = light_a + sol;
                            t.y = -r_z_near;
                        }
                    }

                    t.x = (t.x * scale) / -t.y;
                    b.x = (b.x * scale) / -b.y;
                    extent->x = fmin(fmax(b.x, -1.0), 1.0);
                    extent->y = fmin(fmax(t.x, -1.0), 1.0);
                }
            }

            if((extents[0].x - extents[0].y) * (extents[1].x - extents[1].y) == 0.0)
            {
                /* light offscreen */
                continue;
            }

            light->min_x = (uint32_t)(R_CLUSTER_ROW_WIDTH * (extents[0].x * 0.5 + 0.5));
            light->max_x = (uint32_t)(R_CLUSTER_ROW_WIDTH * (extents[0].y * 0.5 + 0.5));
            if(light->max_x > R_CLUSTER_MAX_X) light->max_x = R_CLUSTER_MAX_X;

            light->min_y = (uint32_t)(R_CLUSTER_ROWS * (extents[1].x * 0.5 + 0.5));
            light->max_y = (uint32_t)(R_CLUSTER_ROWS * (extents[1].y * 0.5 + 0.5));
            if(light->max_y > R_CLUSTER_MAX_Y) light->max_y = R_CLUSTER_MAX_Y;

            float num = log(fmax(-light_pos.z - light->data.pos_rad.w, r_z_near) / r_z_near);
            light->min_z = (uint32_t)floorf(R_CLUSTER_SLICES * (num / r_denom));
            num = log(fmax(-light_pos.z + light->data.pos_rad.w, 0) / r_z_near);
            light->max_z = (uint32_t)floorf(R_CLUSTER_SLICES * (num / r_denom));

            if(light->min_z > R_CLUSTER_MAX_Z)
            {
                light->min_z = R_CLUSTER_MAX_Z;
            }

            if(light->max_z > R_CLUSTER_MAX_Z)
            {
                light->max_z = R_CLUSTER_MAX_Z;
            }

            light->gpu_index = r_light_buffer_cursor;
            ds_list_add_element(&r_visible_lights, &light);
            r_light_buffer_cursor++;
            light->data.color_res.w = 16;

            data->color_res.x = light->data.color_res.x * light->energy;
            data->color_res.y = light->data.color_res.y * light->energy;
            data->color_res.z = light->data.color_res.z * light->energy;
            data->color_res.w = light->data.color_res.w;

            data->pos_rad.x = light_pos.x;
            data->pos_rad.y = light_pos.y;
            data->pos_rad.z = light_pos.z;
            data->pos_rad.w = light->data.pos_rad.w;


        }
    }

    ds_list_qsort(&r_visible_lights, r_CompareVisibleLights);

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

    for(uint32_t visible_index = 0; visible_index < r_visible_lights.cursor; visible_index++)
    {
        struct r_light_t *light = *(struct r_light_t **)ds_list_get_element(&r_visible_lights, visible_index);

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


    for(uint32_t visible_light_index = 0; visible_light_index < r_visible_lights.cursor; visible_light_index++)
    {
        struct r_light_t *light = *(struct r_light_t **)ds_list_get_element(&r_visible_lights, visible_light_index);

        uint32_t first_index = light->gpu_index * 6;

        for(uint32_t face_index = 0; face_index < 6; face_index++)
        {
            uint32_t map_index = first_index + face_index;
            uint32_t map_res = light->data.color_res.w;
            uint32_t x_coord = (map_index % (R_SHADOW_MAP_ATLAS_WIDTH / R_SHADOW_MAP_MAX_RESOLUTION)) * map_res;
            uint32_t y_coord = (map_index / (R_SHADOW_MAP_ATLAS_WIDTH / R_SHADOW_MAP_MAX_RESOLUTION)) * map_res;

            uint32_t shadow_map = 0;
            shadow_map |= (x_coord << R_SHADOW_MAP_X_COORD_SHIFT);
            shadow_map |= (y_coord << R_SHADOW_MAP_Y_COORD_SHIFT);
            shadow_map |= (map_res << R_SHADOW_MAP_RES_SHIFT);
            r_shadow_index_buffer[map_index] = shadow_map;
        }
    }
}

void r_VisibleEntities()
{
    for(uint32_t entity_index = 0; entity_index < g_entities.cursor; entity_index++)
    {
        struct g_entity_t *entity = g_GetEntity(entity_index);

        if(entity)
        {
            r_DrawEntity(&entity->transform, entity->model);
        }
    }
}

void r_VisibleEntitiesOnLights()
{
    for(uint32_t index = 0; index < r_visible_lights.cursor; index++)
    {
        struct r_light_t *light = *(struct r_light_t **)ds_list_get_element(&r_visible_lights, index);
        uint32_t *shadow_map = r_shadow_index_buffer + light->gpu_index * 6;

        mat4_t light_view_projection_matrices[6];

        for(uint32_t face_index = 0; face_index < 6; face_index++)
        {
            mat4_t_identity(&light_view_projection_matrices[face_index]);
            light_view_projection_matrices[face_index].rows[3].x = -light->data.pos_rad.x;
            light_view_projection_matrices[face_index].rows[3].y = -light->data.pos_rad.y;
            light_view_projection_matrices[face_index].rows[3].z = -light->data.pos_rad.z;
            mat4_t_mul(&light_view_projection_matrices[face_index],
                       &light_view_projection_matrices[face_index],
                       &r_point_shadow_view_projection_matrices[face_index]);
        }

        float light_radius = light->data.pos_rad.w;
        for(uint32_t entity_index = 0; entity_index < g_entities.cursor; entity_index++)
        {
            struct g_entity_t *entity = g_GetEntity(entity_index);

            if(entity && entity->model)
            {
                vec3_t light_entity_vec;
                vec3_t normalized_light_entity_vec;
                vec3_t model_extents;
                uint16_t dists = 0;

                model_extents.x = entity->extents.x * 0.5;
                model_extents.y = entity->extents.y * 0.5;
                model_extents.z = entity->extents.z * 0.5;

                vec3_t_sub(&light_entity_vec, &entity->transform.rows[3].xyz, &light->data.pos_rad.xyz);
                float light_entity_dist = vec3_t_length(&light_entity_vec);
                vec3_t_normalize(&normalized_light_entity_vec, &light_entity_vec);
                vec3_t_fabs(&normalized_light_entity_vec, &normalized_light_entity_vec);

                float box_radius = vec3_t_dot(&normalized_light_entity_vec, &model_extents);

                if(box_radius + light_radius > light_entity_dist)
                {
                    for(uint32_t plane_index = 0; plane_index < 6; plane_index++)
                    {
                        dists <<= 2;

                        vec3_t abs_plane_normal = r_point_shadow_frustum_planes[plane_index];
                        vec3_t_fabs(&abs_plane_normal, &abs_plane_normal);
                        box_radius = vec3_t_dot(&abs_plane_normal, &model_extents);
                        float dist_to_plane = vec3_t_dot(&r_point_shadow_frustum_planes[plane_index], &light_entity_vec);

                        if(fabs(dist_to_plane) < box_radius)
                        {
                            dists |= R_SHADOW_MAP_PLANE_FRONT | R_SHADOW_MAP_PLANE_BACK;
                        }
                        else if(dist_to_plane > 0.0)
                        {
                            dists |= R_SHADOW_MAP_PLANE_FRONT;
                        }
                        else
                        {
                            dists |= R_SHADOW_MAP_PLANE_BACK;
                        }
                    }

                    for(uint32_t face_index = 0; face_index < 6; face_index++)
                    {
                        uint16_t mask = r_point_shadow_frustum_masks[face_index];

                        if(mask && (mask & dists) == mask)
                        {
//                            printf("face %d\n", face_index);
                            struct r_batch_t *batch = (struct r_batch_t *)entity->model->batches.buffer;
                            uint32_t count = entity->model->indices.buffer_size;
                            mat4_t model_view_projection_matrix;
                            mat4_t_mul(&model_view_projection_matrix, &entity->transform, &light_view_projection_matrices[face_index]);
                            r_DrawShadow(&model_view_projection_matrix, shadow_map[face_index], batch->start, count);
                        }
                    }
//                    printf("\n\n");
                }
            }



        }
    }
}

void r_VisibleWorld()
{

}

void r_VisibleVisItems()
{
//    for(uint32_t item_index = 0; item_index < r_vis_items.cursor; item_index++)
//    {
//        struct r_vis_item_t *item = get_stack_list_element(&r_vis_items, item_index);
//
//        if(!item->transform)
//        {
//            continue;
//        }
//
//        r_DrawModel(item->transform, item->model);
//    }
}






