#include "r_vis.h"
#include "ent.h"
#include "g_main.h"

extern struct ds_list_t r_visible_lights;
extern mat4_t r_camera_matrix;
extern mat4_t r_view_matrix;
extern mat4_t r_view_projection_matrix;
extern mat4_t r_projection_matrix;
extern struct ds_slist_t r_lights[];
extern struct ds_slist_t r_vis_items;
extern struct r_cluster_t *r_clusters;

extern struct r_model_t *l_world_model;

extern struct r_point_data_t *r_point_light_buffer;
extern uint32_t r_point_light_buffer_cursor;
extern struct r_spot_data_t *r_spot_light_buffer;
extern uint32_t r_spot_light_buffer_cursor;
extern struct r_model_t *r_spot_light_model;
extern struct r_model_t *r_point_light_model;

extern uint32_t *r_light_index_buffer;
extern uint32_t r_light_index_buffer_cursor;

//extern uint32_t *r_shadow_index_buffer;
//extern uint32_t  r_shadow_index_buffer_cursor;
extern struct r_shadow_map_t *r_shadow_map_buffer;
extern uint32_t r_shadow_map_buffer_cursor;
extern float r_spot_light_tan_lut[];
extern float r_spot_light_cos_lut[];
extern uint32_t r_light_shadow_map_count[];
extern mat4_t r_point_shadow_view_projection_matrices[6];
extern vec3_t r_point_light_frustum_planes[6];
extern uint16_t r_point_light_frustum_masks[6];
extern struct r_renderer_state_t r_renderer_state;

extern float r_z_near;
extern float r_z_far;
extern uint32_t r_width;
extern uint32_t r_height;
extern float r_denom;
extern float r_fov;

//extern struct ds_slist_t g_entities;

extern struct ds_list_t e_components[];

int32_t r_CompareVisibleLights(void *a, void *b)
{
//    struct r_light_t *light_a = *(struct r_light_t **)a;
//    struct r_light_t *light_b = *(struct r_light_t **)b;
//
//    if(light_a->data.color_res.w > light_b->data.color_res.w) return 1;
//    if(light_a->data.color_res.w < light_b->data.color_res.w) return -1;
    return 0;
}

void r_VisibleLights()
{
    r_point_light_buffer_cursor = 0;
    r_spot_light_buffer_cursor = 0;
    r_light_index_buffer_cursor = 0;
    r_shadow_map_buffer_cursor = 0;
    r_visible_lights.cursor = 0;

    vec3_t axes[2] = {vec3_t_c(1.0, 0.0, 0.0), vec3_t_c(0.0, 1.0, 0.0)};
    vec2_t extents[2];
    vec4_t spot_verts[8];

    r_i_SetShader(NULL);
    r_i_SetModelMatrix(NULL);

    for(uint32_t light_index = 0; light_index < r_lights[R_LIGHT_TYPE_POINT].cursor; light_index++)
    {
        struct r_point_light_t *light = (struct r_point_light_t *)r_GetLight(R_LIGHT_INDEX(R_LIGHT_TYPE_POINT, light_index));

        if(light)
        {
            struct r_point_data_t *data = r_point_light_buffer + r_point_light_buffer_cursor;
            vec4_t pos_rad = vec4_t_c(light->position.x, light->position.y, light->position.z, 1.0);
            mat4_t_vec4_t_mul(&pos_rad, &r_view_matrix, &pos_rad);

            vec3_t light_pos = pos_rad.xyz;
            float sqrd_radius = light->range * light->range;
            float sol = 0.0;

            if(light_pos.z - light->range > -r_z_near)
            {
                /* light completely behind the near plane */
                continue;
            }
            else
            {
                extents[0] = vec2_t_c(-FLT_MAX, FLT_MAX);
                extents[1] = vec2_t_c(-FLT_MAX, FLT_MAX);

                if(light_pos.z + light->range >= -r_z_near)
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
                        float sin_theta = light->range / center_dist;

                        t.x = (cos_theta * light_vec.x - sin_theta * light_vec.y);
                        t.y = (sin_theta * light_vec.x + cos_theta * light_vec.y);
                        b.x = (cos_theta * light_vec.x + sin_theta * light_vec.y);
                        b.y = (-sin_theta * light_vec.x + cos_theta * light_vec.y);

                        /* let 'light_vec' be the vector from the camera to the center of the
                        sphere. The paper says that the tangent points are this vector, normalized,
                        rotated by an angle theta and -theta, then multiplied by the tangent distance.
                        To normalize 'light_vec' it's required to multiply it by 1.0 / length, and to
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

            if((extents[0].y - extents[0].x) * (extents[1].y - extents[1].x) == 0.0)
            {
                /* light offscreen */
                continue;
            }

//            mat4_t projection_matrix = mat4_t_c_id();
//            r_i_SetViewProjectionMatrix(&projection_matrix);
//            r_i_DrawLine(&vec3_t_c(extents[0].y, extents[1].x, -0.5), &vec3_t_c(extents[0].y, extents[1].y, -0.5), &vec4_t_c(0.0, 1.0, 0.0, 1.0), 1.0);
//            r_i_DrawLine(&vec3_t_c(extents[0].y, extents[1].y, -0.5), &vec3_t_c(extents[0].x, extents[1].y, -0.5), &vec4_t_c(0.0, 1.0, 0.0, 1.0), 1.0);
//            r_i_DrawLine(&vec3_t_c(extents[0].x, extents[1].y, -0.5), &vec3_t_c(extents[0].x, extents[1].x, -0.5), &vec4_t_c(0.0, 1.0, 0.0, 1.0), 1.0);
//            r_i_DrawLine(&vec3_t_c(extents[0].x, extents[1].x, -0.5), &vec3_t_c(extents[0].y, extents[1].x, -0.5), &vec4_t_c(0.0, 1.0, 0.0, 1.0), 1.0);

            float light_dist = vec3_t_length(&light_pos);
            float ratio = light->range / light_dist;
            uint32_t shadow_resolution = R_SHADOW_BUCKET2_RES;

//            if(ratio >= 0.5)
//            {
//                shadow_resolution = R_SHADOW_BUCKET2_RES;
//            }
//            else if(ratio >= 0.25)
//            {
//                shadow_resolution = R_SHADOW_BUCKET2_RES;
//            }
//            else if(ratio >= 0.125)
//            {
//                shadow_resolution = R_SHADOW_BUCKET2_RES;
//            }
//            else if(ratio >= 0.0625)
//            {
//                shadow_resolution = R_SHADOW_BUCKET1_RES;
//            }
//            else
//            {
//                shadow_resolution = R_SHADOW_BUCKET0_RES;
//            }


            light->min.x = (uint32_t)(R_CLUSTER_ROW_WIDTH * (extents[0].x * 0.5 + 0.5));
            light->max.x = (uint32_t)(R_CLUSTER_ROW_WIDTH * (extents[0].y * 0.5 + 0.5));
            if(light->max.x > R_CLUSTER_MAX_X) light->max.x = R_CLUSTER_MAX_X;

            light->min.y = (uint32_t)(R_CLUSTER_ROWS * (extents[1].x * 0.5 + 0.5));
            light->max.y = (uint32_t)(R_CLUSTER_ROWS * (extents[1].y * 0.5 + 0.5));
            if(light->max.y > R_CLUSTER_MAX_Y) light->max.y = R_CLUSTER_MAX_Y;

            float num = log(fmax(-light_pos.z - light->range, r_z_near) / r_z_near);
            light->min.z = (uint32_t)floorf(R_CLUSTER_SLICES * (num / r_denom));
            num = log(fmax(-light_pos.z + light->range, 0) / r_z_near);
            light->max.z = (uint32_t)floorf(R_CLUSTER_SLICES * (num / r_denom));

            if(light->min.z > R_CLUSTER_MAX_Z)
            {
                light->min.z = R_CLUSTER_MAX_Z;
            }

            if(light->max.z > R_CLUSTER_MAX_Z)
            {
                light->max.z = R_CLUSTER_MAX_Z;
            }

            light->light_buffer_index = r_point_light_buffer_cursor;
            light->shadow_map_buffer_index = r_shadow_map_buffer_cursor;
            light->shadow_map_res = shadow_resolution;
            ds_list_add_element(&r_visible_lights, &light);
            r_point_light_buffer_cursor++;

            union { uint32_t i; float f; }i_to_f;

            i_to_f.i = (shadow_resolution << 16) | (r_shadow_map_buffer_cursor & 0xffff);
            r_shadow_map_buffer_cursor += 6;

            data->color_shd.x = light->color.x * light->energy;
            data->color_shd.y = light->color.y * light->energy;
            data->color_shd.z = light->color.z * light->energy;
            data->color_shd.w = i_to_f.f;

            data->pos_rad.xyz = light_pos;
            data->pos_rad.w = light->range;
        }
    }

    mat4_t projection_matrix;
    mat4_t_identity(&projection_matrix);
//    r_i_SetViewProjectionMatrix(&projection_matrix);
//    r_i_SetModelMatrix(NULL);
//    r_i_SetShader(NULL);

    for(uint32_t light_index = 0; light_index < r_lights[R_LIGHT_TYPE_SPOT].cursor; light_index++)
    {
        struct r_spot_light_t *light = (struct r_spot_light_t *)r_GetLight(R_LIGHT_INDEX(R_LIGHT_TYPE_SPOT, light_index));

        if(light)
        {
            vec3_t light_vec;
            extents[0] = vec2_t_c(FLT_MAX, -FLT_MAX);
            extents[1] = vec2_t_c(FLT_MAX, -FLT_MAX);

            /* tentative "optimization", to avoid doing a bunch of trig in a hot loop. This is
            a relatively big table, though, and depending on the angle value distribution used
            by the spotlights, reads might be fairly scattered, which is not great for spatial
            coherence. Still gotta properly profile this. */
            float cos = r_spot_light_cos_lut[light->angle - R_SPOT_LIGHT_MIN_ANGLE];
            float tan = r_spot_light_tan_lut[light->angle - R_SPOT_LIGHT_MIN_ANGLE];
            float base_radius = tan * light->range;

            vec3_t_sub(&light_vec, &light->position, &r_camera_matrix.rows[3].xyz);
            float dist = vec3_t_length(&light_vec);
            vec3_t_div(&light_vec, &light_vec, dist);

            light->min.z = 0;
            light->max.z = R_CLUSTER_MAX_Z;

            if(vec3_t_dot(&light_vec, &light->orientation.rows[2]) >= cos && dist <= light->range)
            {
                /* camera inside cone */
                extents[0] = vec2_t_c(-1.0, 1.0);
                extents[1] = vec2_t_c(-1.0, 1.0);
            }
            else
            {
                vec4_t base = {.w = 1.0};
                vec4_t apex = {.w = 1.0};
                vec4_t right = {.w = 0.0};
                vec4_t neg_right = {.w = 0.0};
                vec4_t top = {.w = 0.0};
                vec4_t neg_top = {.w = 0.0};

                apex.xyz = light->position;
                vec3_t_fmadd(&base.xyz, &light->position, &light->orientation.rows[2], -light->range);
                vec3_t_mul(&right.xyz, &light->orientation.rows[0], base_radius);
                vec3_t_mul(&top.xyz, &light->orientation.rows[1], base_radius);

                mat4_t_vec4_t_mul(&apex, &r_view_matrix, &apex);
                mat4_t_vec4_t_mul(&base, &r_view_matrix, &base);
                mat4_t_vec4_t_mul(&right, &r_view_matrix, &right);
                mat4_t_vec4_t_mul(&top, &r_view_matrix, &top);

                vec3_t_neg(&neg_right.xyz, &right.xyz);
                vec3_t_neg(&neg_top.xyz, &top.xyz);

                spot_verts[0] = apex;
                spot_verts[1] = apex;
                spot_verts[2] = apex;
                spot_verts[3] = apex;

                vec3_t_add(&spot_verts[4].xyz, &base.xyz, &right.xyz);
                vec3_t_add(&spot_verts[4].xyz, &spot_verts[4].xyz, &top.xyz);

                vec3_t_add(&spot_verts[5].xyz, &base.xyz, &neg_right.xyz);
                vec3_t_add(&spot_verts[5].xyz, &spot_verts[5].xyz, &top.xyz);

                vec3_t_add(&spot_verts[6].xyz, &base.xyz, &right.xyz);
                vec3_t_add(&spot_verts[6].xyz, &spot_verts[6].xyz, &neg_top.xyz);

                vec3_t_add(&spot_verts[7].xyz, &base.xyz, &neg_right.xyz);
                vec3_t_add(&spot_verts[7].xyz, &spot_verts[7].xyz, &neg_top.xyz);

                for(uint32_t vert_index = 0; vert_index < 4; vert_index++)
                {
                    vec4_t *vert0 = spot_verts + vert_index;
                    vec4_t *vert1 = spot_verts + vert_index + 4;
                    vert1->w = 1.0;

                    float dist0 = vert0->z + r_z_near;
                    float dist1 = vert1->z + r_z_near;

                    if(dist0 > 0.0)
                    {
                        float t = dist1 / (dist1 - dist0);
                        vec4_t_lerp(vert0, vert1, vert0, t);
                    }
                    else if(dist1 > 0.0)
                    {
                        float t = dist0 / (dist0 - dist1);
                        vec4_t_lerp(vert1, vert0, vert1, t);
                    }

                    mat4_t_vec4_t_mul(vert0, &r_projection_matrix, vert0);
                    vert0->x /= vert0->w;
                    vert0->y /= vert0->w;

                    if(extents[0].x > vert0->x) extents[0].x = vert0->x;
                    if(extents[0].y < vert0->x) extents[0].y = vert0->x;

                    if(extents[1].x > vert0->y) extents[1].x = vert0->y;
                    if(extents[1].y < vert0->y) extents[1].y = vert0->y;
                }


                for(uint32_t vert_index = 0; vert_index < 4; vert_index++)
                {
                    vec4_t *vert0 = spot_verts + vert_index + 4;
                    vec4_t *vert1 = spot_verts + ((vert_index + 5) % 4);

                    float dist0 = vert0->z + r_z_near;
                    float dist1 = vert1->z + r_z_near;

                    if(dist0 > 0.0)
                    {
                        float t = dist1 / (dist1 - dist0);
                        vec4_t_lerp(vert0, vert1, vert0, t);
                    }
                    else if(dist1 > 0.0)
                    {
                        float t = dist0 / (dist0 - dist1);
                        vec4_t_lerp(vert1, vert0, vert1, t);
                    }

                    mat4_t_vec4_t_mul(vert0, &r_projection_matrix, vert0);
                    vert0->x /= vert0->w;
                    vert0->y /= vert0->w;

                    if(extents[0].x > vert0->x) extents[0].x = vert0->x;
                    if(extents[0].y < vert0->x) extents[0].y = vert0->x;

                    if(extents[1].x > vert0->y) extents[1].x = vert0->y;
                    if(extents[1].y < vert0->y) extents[1].y = vert0->y;
                }

                extents[0].x = fmin(fmax(extents[0].x, -1.0), 1.0);
                extents[0].y = fmin(fmax(extents[0].y, -1.0), 1.0);
                extents[1].x = fmin(fmax(extents[1].x, -1.0), 1.0);
                extents[1].y = fmin(fmax(extents[1].y, -1.0), 1.0);

                if((extents[0].y - extents[0].x) * (extents[1].y - extents[1].x) == 0.0)
                {
                    /* light offscreen */
                    continue;
                }
            }

//            r_i_SetViewProjectionMatrix(&projection_matrix);
//            r_i_DrawLine(&vec3_t_c(extents[0].y, extents[1].x, -0.5), &vec3_t_c(extents[0].y, extents[1].y, -0.5), &vec4_t_c(0.0, 1.0, 0.0, 1.0), 1.0);
//            r_i_DrawLine(&vec3_t_c(extents[0].y, extents[1].y, -0.5), &vec3_t_c(extents[0].x, extents[1].y, -0.5), &vec4_t_c(0.0, 1.0, 0.0, 1.0), 1.0);
//            r_i_DrawLine(&vec3_t_c(extents[0].x, extents[1].y, -0.5), &vec3_t_c(extents[0].x, extents[1].x, -0.5), &vec4_t_c(0.0, 1.0, 0.0, 1.0), 1.0);
//            r_i_DrawLine(&vec3_t_c(extents[0].x, extents[1].x, -0.5), &vec3_t_c(extents[0].y, extents[1].x, -0.5), &vec4_t_c(0.0, 1.0, 0.0, 1.0), 1.0);

            float spot_fovy = ((float)light->angle / 180.0) * 3.14159265;
            mat4_t_persp(&light->projection_matrix, spot_fovy, 1.0, 0.01, light->range);

            struct r_spot_data_t *data = r_spot_light_buffer + r_spot_light_buffer_cursor;
            light->light_buffer_index = r_spot_light_buffer_cursor;
            light->shadow_map_buffer_index = r_shadow_map_buffer_cursor;
            ds_list_add_element(&r_visible_lights, &light);
            r_spot_light_buffer_cursor++;

            union { uint32_t i; float f; }i_to_f;
            uint32_t shadow_resolution = R_SHADOW_BUCKET2_RES;
            i_to_f.i = (shadow_resolution << 16) | (r_shadow_map_buffer_cursor & 0xffff);
            r_shadow_map_buffer_cursor++;

            light->shadow_map_res = shadow_resolution;

            vec4_t pos_rad = vec4_t_c(light->position.x, light->position.y, light->position.z, 1.0);
            mat4_t_vec4_t_mul(&pos_rad, &r_view_matrix, &pos_rad);

            light->min.x = (uint32_t)(R_CLUSTER_ROW_WIDTH * (extents[0].x * 0.5 + 0.5));
            light->max.x = (uint32_t)(R_CLUSTER_ROW_WIDTH * (extents[0].y * 0.5 + 0.5));
            if(light->max.x > R_CLUSTER_MAX_X) light->max.x = R_CLUSTER_MAX_X;

            light->min.y = (uint32_t)(R_CLUSTER_ROWS * (extents[1].x * 0.5 + 0.5));
            light->max.y = (uint32_t)(R_CLUSTER_ROWS * (extents[1].y * 0.5 + 0.5));
            if(light->max.y > R_CLUSTER_MAX_Y) light->max.y = R_CLUSTER_MAX_Y;

            data->pos_rad = pos_rad;

            data->pos_rad.w = light->range;
            data->col_shd.x = light->color.x * light->energy;
            data->col_shd.y = light->color.y * light->energy;
            data->col_shd.z = light->color.z * light->energy;
            data->col_shd.w = i_to_f.f;

            data->rot0_angle.xyz = light->orientation.rows[0];
            data->rot0_angle.w = 0.0;
            mat4_t_vec4_t_mul(&data->rot0_angle, &r_view_matrix, &data->rot0_angle);
            data->rot0_angle.w = cos;

            data->rot1_soft.xyz = light->orientation.rows[1];
            data->rot1_soft.w = 0.0;
            mat4_t_vec4_t_mul(&data->rot1_soft, &r_view_matrix, &data->rot1_soft);
            data->rot1_soft.w = light->softness;

            data->rot2.xyz = light->orientation.rows[2];
            data->rot2.w = 0.0;
            mat4_t_vec4_t_mul(&data->rot2, &r_view_matrix, &data->rot2);

            data->proj.x = light->projection_matrix.comps[0][0];
            data->proj.y = r_spot_light_tan_lut[light->angle - R_SPOT_LIGHT_MIN_ANGLE];
            data->proj.z = light->projection_matrix.comps[2][2];
            data->proj.w = light->projection_matrix.comps[3][2];
        }
    }

    if(r_renderer_state.draw_lights)
    {
        r_i_SetViewProjectionMatrix(NULL);
        r_i_SetModelMatrix(NULL);
        r_i_SetShader(NULL);

        for(uint32_t light_index = 0; light_index < r_visible_lights.cursor; light_index++)
        {
            struct r_light_t *light = *(struct r_light_t **)ds_list_get_element(&r_visible_lights, light_index);
            struct r_i_draw_list_t *draw_list = r_i_AllocDrawList(1);
            mat4_t light_transform;
            uint32_t cmd_type;

            switch(light->type)
            {
                case R_LIGHT_TYPE_SPOT:
                {
                    struct r_spot_light_t *spot_light = (struct r_spot_light_t *)light;
                    float base_radius = r_spot_light_tan_lut[spot_light->angle - R_SPOT_LIGHT_MIN_ANGLE] * spot_light->range;

                    mat4_t model_transform;
                    mat4_t_identity(&model_transform);
                    model_transform.rows[0].x = base_radius;
                    model_transform.rows[1].y = base_radius;
                    model_transform.rows[2].z = spot_light->range;

                    mat4_t_comp(&light_transform, &spot_light->orientation, &spot_light->position);
                    mat4_t_mul(&light_transform, &model_transform, &light_transform);

                    draw_list->commands[0].start = r_spot_light_model->model_start;
                    draw_list->commands[0].count = r_spot_light_model->model_count;
                    draw_list->size = 1.0;
                    draw_list->indexed = 1;

                    cmd_type = R_I_DRAW_CMD_TRIANGLE_LIST;
                    r_i_SetRasterizer(GL_FALSE, GL_BACK, GL_LINE);
                }
                break;

                case R_LIGHT_TYPE_POINT:
                {
                    mat3_t orientation;
                    mat3_t_identity(&orientation);
                    orientation.rows[0].x = light->range;
                    orientation.rows[1].y = light->range;
                    orientation.rows[2].z = light->range;
                    mat4_t_comp(&light_transform, &orientation, &light->position);

                    draw_list->commands[0].start = r_point_light_model->model_start;
                    draw_list->commands[0].count = r_point_light_model->model_count;
                    draw_list->size = 1.0;
                    draw_list->indexed = 1;

                    cmd_type = R_I_DRAW_CMD_LINE_LIST;
                    r_i_SetRasterizer(GL_FALSE, GL_BACK, GL_FILL);
                }
                break;
            }

            r_i_SetModelMatrix(&light_transform);
            r_i_DrawImmediate(cmd_type, draw_list);
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
                cluster->point_start = cluster_offset * R_MAX_CLUSTER_LIGHTS;
                cluster->point_count = 0;
                cluster->spot_start = cluster->point_start + R_MAX_CLUSTER_POINT_LIGHTS;
                cluster->spot_count = 0;
            }
        }
    }

    for(uint32_t visible_index = 0; visible_index < r_visible_lights.cursor; visible_index++)
    {
        struct r_light_t *light = *(struct r_light_t **)ds_list_get_element(&r_visible_lights, visible_index);

        uint32_t shadow_map_count;
        uint32_t *shadow_maps;

        switch(light->type)
        {
            case R_LIGHT_TYPE_POINT:
            {
                struct r_point_light_t *point_light = (struct r_point_light_t *)light;
                shadow_map_count = 6;
                shadow_maps = point_light->shadow_maps;
            }
            break;

            case R_LIGHT_TYPE_SPOT:
            {
                struct r_spot_light_t *spot_light = (struct r_spot_light_t *)light;
                shadow_map_count = 1;
                shadow_maps = &spot_light->shadow_map;
            }
            break;
        }

        if(light->shadow_map_res != R_SHADOW_BUCKET_RESOLUTION(shadow_maps[0]))
        {
            r_FreeShadowMaps(light);
            r_AllocShadowMaps(light, light->shadow_map_res);
        }

        for(uint32_t shadow_map_index = 0; shadow_map_index < shadow_map_count; shadow_map_index++)
        {
            struct r_shadow_map_t *shadow_map = r_GetShadowMap(shadow_maps[shadow_map_index]);
            r_shadow_map_buffer[light->shadow_map_buffer_index + shadow_map_index] = *shadow_map;
        }

        /* FIXME: this whole loop could be duplicated inside each switch statement, instead
        of having it inside the loop. The light type won't change for all its duration, so
        it's useless to keep retesting every time. The branch predictor will do a good job,
        but still, unnecessary branching. */
        for(uint32_t slice_index = light->min.z; slice_index <= light->max.z; slice_index++)
        {
            uint32_t slice_offset = slice_index * R_CLUSTER_ROWS * R_CLUSTER_ROW_WIDTH;

            for(uint32_t row_index = light->min.y; row_index <= light->max.y; row_index++)
            {
                uint32_t row_offset = row_index * R_CLUSTER_ROW_WIDTH;

                for(uint32_t cluster_index = light->min.x; cluster_index <= light->max.x; cluster_index++)
                {
                    uint32_t cluster_offset = cluster_index + row_offset + slice_offset;
                    struct r_cluster_t *cluster = r_clusters + cluster_offset;
                    uint32_t start;
                    uint32_t count;

                    switch(light->type)
                    {
                        case R_LIGHT_TYPE_POINT:
                            start = cluster->point_start;
                            count = cluster->point_count;
                            cluster->point_count++;
                        break;

                        case R_LIGHT_TYPE_SPOT:
                            start = cluster->spot_start;
                            count = cluster->spot_count;
                            cluster->spot_count++;
                        break;
                    }

                    r_light_index_buffer[start + count] = light->light_buffer_index;
                }
            }
        }
    }

//    mat4_t_identity(&projection_matrix);
//    r_i_SetViewProjectionMatrix(&projection_matrix);
//    r_i_SetModelMatrix(NULL);
//    r_i_SetShader(NULL);
//
//    #define GRAPH_SIZE 0.9
//    #define RATIO ((float)r_width / (float)r_height)
//
//    r_i_DrawLine(&vec3_t_c(-GRAPH_SIZE / RATIO, GRAPH_SIZE, -0.5), &vec3_t_c(-GRAPH_SIZE / RATIO, -GRAPH_SIZE, -0.5), &vec4_t_c(0.0, 1.0, 0.0, 1.0), 1.0);
//    r_i_DrawLine(&vec3_t_c(-GRAPH_SIZE / RATIO,-GRAPH_SIZE, -0.5), &vec3_t_c( GRAPH_SIZE / RATIO, -GRAPH_SIZE, -0.5), &vec4_t_c(0.0, 1.0, 0.0, 1.0), 1.0);
//    r_i_DrawLine(&vec3_t_c( GRAPH_SIZE / RATIO,-GRAPH_SIZE, -0.5), &vec3_t_c( GRAPH_SIZE / RATIO,  GRAPH_SIZE, -0.5), &vec4_t_c(0.0, 1.0, 0.0, 1.0), 1.0);
//    r_i_DrawLine(&vec3_t_c( GRAPH_SIZE / RATIO, GRAPH_SIZE, -0.5), &vec3_t_c(-GRAPH_SIZE / RATIO,  GRAPH_SIZE, -0.5), &vec4_t_c(0.0, 1.0, 0.0, 1.0), 1.0);
//
//    for(uint32_t index = 0; index < r_visible_lights.cursor; index++)
//    {
//        struct r_light_t *light = *(struct r_light_t **)ds_list_get_element(&r_visible_lights, index);
//        uint32_t shadow_map_count;
//        uint32_t *shadow_map_handles;
//
//        switch(light->type)
//        {
//            case R_LIGHT_TYPE_POINT:
//            {
//                struct r_point_light_t *point_light = (struct r_point_light_t *)light;
//                shadow_map_count = 6;
//                shadow_map_handles = point_light->shadow_maps;
//            }
//            break;
//
//            case R_LIGHT_TYPE_SPOT:
//            {
//                struct r_spot_light_t *spot_light = (struct r_spot_light_t *)light;
//                shadow_map_count = 1;
//                shadow_map_handles = &spot_light->shadow_map;
//            }
//            break;
//        }
//
//        for(uint32_t shadow_map_index = 0; shadow_map_index < shadow_map_count; shadow_map_index++)
//        {
//            struct r_shadow_map_t *shadow_map = r_GetShadowMap(shadow_map_handles[shadow_map_index]);
//
//            float size = (float)light->shadow_map_res / R_SHADOW_MAP_ATLAS_WIDTH;
//            float x0 = ((float)shadow_map->x_coord / (float)R_SHADOW_MAP_ATLAS_WIDTH);
//            float y0 = ((float)shadow_map->y_coord / (float)R_SHADOW_MAP_ATLAS_HEIGHT);
//            float x1 = x0 + size;
//            float y1 = y0 + size;
//
//            x0 = (x0 * 2.0 - 1.0) * GRAPH_SIZE / RATIO;
//            y0 = (y0 * 2.0 - 1.0) * GRAPH_SIZE;
//            x1 = (x1 * 2.0 - 1.0) * GRAPH_SIZE / RATIO;
//            y1 = (y1 * 2.0 - 1.0) * GRAPH_SIZE;
//
//            r_i_DrawLine(&vec3_t_c(x0, y0, -0.5), &vec3_t_c(x0, y1, -0.5), &vec4_t_c(0.0, 0.5, 1.0, 1.0), 2.0);
//            r_i_DrawLine(&vec3_t_c(x0, y1, -0.5), &vec3_t_c(x1, y1, -0.5), &vec4_t_c(0.0, 0.5, 1.0, 1.0), 2.0);
//            r_i_DrawLine(&vec3_t_c(x1, y1, -0.5), &vec3_t_c(x1, y0, -0.5), &vec4_t_c(0.0, 0.5, 1.0, 1.0), 2.0);
//            r_i_DrawLine(&vec3_t_c(x1, y0, -0.5), &vec3_t_c(x0, y0, -0.5), &vec4_t_c(0.0, 0.5, 1.0, 1.0), 2.0);
//        }
//    }
}

void r_VisibleEntities()
{
    for(uint32_t model_index = 0; model_index < e_components[E_COMPONENT_TYPE_MODEL].cursor; model_index++)
    {
        struct e_model_t *model = (struct e_model_t *)e_GetComponent(E_COMPONENT_TYPE_MODEL, model_index);
        struct e_transform_t *transform = model->entity->transform;

        r_DrawEntity(&transform->transform, transform->entity->model->model);
    }
}

void r_VisibleEntitiesOnLights()
{
    for(uint32_t index = 0; index < r_visible_lights.cursor; index++)
    {
        struct r_light_t *light = *(struct r_light_t **)ds_list_get_element(&r_visible_lights, index);
        uint32_t *shadow_maps;
        uint32_t shadow_map_count;

        switch(light->type)
        {
            case R_LIGHT_TYPE_SPOT:
            {
                struct r_spot_light_t *spot_light = (struct r_spot_light_t *)light;

                mat4_t light_view_projection_matrix;
                mat4_t_comp(&light_view_projection_matrix, &spot_light->orientation, &spot_light->position);
                mat4_t_invvm(&light_view_projection_matrix, &light_view_projection_matrix);
                mat4_t_mul(&light_view_projection_matrix, &light_view_projection_matrix, &spot_light->projection_matrix);

                vec3_t light_vec;
                vec3_t_mul(&light_vec, &spot_light->orientation.rows[2], -1.0);

                for(uint32_t entity_index = 0; entity_index < e_components[E_COMPONENT_TYPE_MODEL].cursor; entity_index++)
                {
                    struct e_model_t *model = e_GetComponent(E_COMPONENT_TYPE_MODEL, entity_index);
                    struct e_entity_t *entity = model->entity;
                    struct e_transform_t *transform = entity->transform;

                    vec3_t extents;
                    vec3_t_mul(&extents, &model->extents, 0.5);

                    vec3_t entity_light_vec;
                    vec3_t light_ray_point;
                    vec3_t_sub(&entity_light_vec, &transform->transform.rows[3].xyz, &spot_light->position);

                    float dist = vec3_t_dot(&entity_light_vec, &light_vec);
                    float angle = vec3_t_dot(&entity_light_vec, &light_vec) / dist;
                    float spot_tan = r_spot_light_tan_lut[spot_light->angle - R_SPOT_LIGHT_MIN_ANGLE];
                    vec3_t_fmadd(&light_ray_point, &spot_light->position, &light_vec, dist);
                    vec3_t_sub(&entity_light_vec, &transform->transform.rows[3].xyz, &light_ray_point);
                    float ray_dist = vec3_t_length(&entity_light_vec);
                    vec3_t_div(&entity_light_vec, &entity_light_vec, ray_dist);
                    vec3_t_fabs(&entity_light_vec, &entity_light_vec);
                    float sphere_radius = vec3_t_dot(&extents, &entity_light_vec);

                    if(ray_dist - sphere_radius < dist * spot_tan)
                    {
                        mat4_t model_view_projection_matrix;
                        mat4_t_mul(&model_view_projection_matrix, &transform->transform, &light_view_projection_matrix);
                        struct r_batch_t *batch = (struct r_batch_t *)model->model->batches.buffer;
                        uint32_t count = model->model->indices.buffer_size;
                        r_DrawShadow(&model_view_projection_matrix, spot_light->shadow_map, batch->start, count);
                    }
                }
            }
            break;

            case R_LIGHT_TYPE_POINT:
            {
                struct r_point_light_t *point_light = (struct r_point_light_t *)light;
                mat4_t light_view_projection_matrices[6];
                shadow_maps = point_light->shadow_maps;
                shadow_map_count = 6;

                for(uint32_t face_index = 0; face_index < shadow_map_count; face_index++)
                {
                    mat4_t_identity(&light_view_projection_matrices[face_index]);
                    light_view_projection_matrices[face_index].rows[3].x = -light->position.x;
                    light_view_projection_matrices[face_index].rows[3].y = -light->position.y;
                    light_view_projection_matrices[face_index].rows[3].z = -light->position.z;
                    mat4_t_mul(&light_view_projection_matrices[face_index],
                               &light_view_projection_matrices[face_index],
                               &r_point_shadow_view_projection_matrices[face_index]);
                }


                float light_radius = light->range;
                for(uint32_t entity_index = 0; entity_index < e_components[E_COMPONENT_TYPE_MODEL].cursor; entity_index++)
                {
                    struct e_model_t *model = (struct e_model_t *)e_GetComponent(E_COMPONENT_TYPE_MODEL, entity_index);
                    struct e_transform_t *transform = transform->entity->transform;

                    if(model)
                    {
                        vec3_t light_entity_vec;
                        vec3_t normalized_light_entity_vec;
                        vec3_t model_extents;
                        uint16_t side_mask = 0;

                        vec3_t_mul(&model_extents, &model->extents, 0.5);
                        vec3_t_sub(&light_entity_vec, &transform->transform.rows[3].xyz, &light->position);
                        float light_entity_dist = vec3_t_length(&light_entity_vec);
                        vec3_t_normalize(&normalized_light_entity_vec, &light_entity_vec);
                        vec3_t_fabs(&normalized_light_entity_vec, &normalized_light_entity_vec);

                        float box_radius = vec3_t_dot(&normalized_light_entity_vec, &model_extents);

                        if(box_radius + light_radius > light_entity_dist)
                        {
                            for(uint32_t plane_index = 0; plane_index < 6; plane_index++)
                            {
                                side_mask <<= 2;

                                vec3_t abs_plane_normal = r_point_light_frustum_planes[plane_index];
                                vec3_t_fabs(&abs_plane_normal, &abs_plane_normal);
                                box_radius = vec3_t_dot(&abs_plane_normal, &model_extents);
                                float dist_to_plane = vec3_t_dot(&r_point_light_frustum_planes[plane_index], &light_entity_vec);

                                if(fabs(dist_to_plane) < box_radius)
                                {
                                    side_mask |= R_POINT_LIGHT_FRUSTUM_PLANE_FRONT | R_POINT_LIGHT_FRUSTUM_PLANE_BACK;
                                }
                                else if(dist_to_plane > 0.0)
                                {
                                    side_mask |= R_POINT_LIGHT_FRUSTUM_PLANE_FRONT;
                                }
                                else
                                {
                                    side_mask |= R_POINT_LIGHT_FRUSTUM_PLANE_BACK;
                                }
                            }

                            for(uint32_t face_index = 0; face_index < shadow_map_count; face_index++)
                            {
                                uint16_t mask = r_point_light_frustum_masks[face_index];

                                if((mask & side_mask) == mask)
                                {
                                    struct r_batch_t *batch = (struct r_batch_t *)model->model->batches.buffer;
                                    uint32_t count = model->model->indices.buffer_size;
                                    mat4_t model_view_projection_matrix;
                                    mat4_t_mul(&model_view_projection_matrix, &transform->transform, &light_view_projection_matrices[face_index]);
                                    r_DrawShadow(&model_view_projection_matrix, shadow_maps[face_index], batch->start, count);
                                }
                            }
                        }
                    }
                }
            }
            break;
        }
    }
}

void r_VisibleWorld()
{
    if(l_world_model)
    {
        struct r_batch_t *world_batches = l_world_model->batches.buffer;

        for(uint32_t batch_index = 0; batch_index < l_world_model->batches.buffer_size; batch_index++)
        {
            struct r_batch_t *world_batch = world_batches + batch_index;
            r_DrawWorld(world_batch->material, world_batch->start, world_batch->count);
        }
    }
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






