#include "r_defs.h"
#include "r_frag_defs.h"
#include "r_frag_uniform_defs.h"
#include "r_vert_uniform_defs.h"
#include "r_light_defs.h"
#include "r_shadow_defs.h"

uniform sampler2D r_tex0;

#define R_SHADOW_SAMPLE_COUNT 64
//#define OFFSET_SIZE 0.005
//
//vec2 offsets[9] =
//{
//    vec2(0.0, 0.0),
//    vec2(-OFFSET_SIZE, OFFSET_SIZE),
//    vec2(-OFFSET_SIZE, 0.0),
//    vec2( OFFSET_SIZE,-OFFSET_SIZE),
//    vec2( 0.0, OFFSET_SIZE),
//    vec2(-OFFSET_SIZE,-OFFSET_SIZE),
//    vec2(-OFFSET_SIZE, 0.0),
//    vec2( OFFSET_SIZE, OFFSET_SIZE),
//    vec2( 0.0,-OFFSET_SIZE),
//};

//float ditherPattern[4][4] = {{ 0.0f, 0.5f, 0.125f, 0.625f},
//{ 0.75f, 0.22f, 0.875f, 0.375f},
//{ 0.1875f, 0.6875f, 0.0625f, 0.5625},
//{ 0.9375f, 0.4375f, 0.8125f, 0.3125}};

void main()
{
    vec3 view_ray;
    vec2 uv = vec2(gl_FragCoord.x / float(r_width), gl_FragCoord.y / float(r_height));
    view_ray.x = ((uv.x * 2.0 - 1.0) * r_z_near) / r_projection_matrix[0][0];
    view_ray.y = ((uv.y * 2.0 - 1.0) * r_z_near) / r_projection_matrix[1][1];
    view_ray.z = -r_z_near;
    view_ray = normalize(view_ray);
    vec3 color = vec3(0);
    float density = 0.012;

    float pixel_z = -(texture(r_tex0, uv).r * 2.0 - 1.0);
    pixel_z = r_projection_matrix[3][2] / (pixel_z - r_projection_matrix[2][2]);

    /*
        cone intersection based on
        https://www.geometrictools.com/Documentation/IntersectionLineCone.pdf
    */

    int dither_x = int(gl_FragCoord.x) % 4;
    int dither_y = int(gl_FragCoord.y) % 4;
//    float dither = r_dither_pattern[dither_x][dither_y];
    float dither = 0.0;

    for(int index = 0; index < r_spot_light_count; index++)
    {
        r_spot_data_t light = spot_lights[index];
        vec3 spot_pos = light.pos_rad.xyz;
        vec3 spot_vec = -light.rot2.xyz;
        float spot_angle = light.rot0_angle.w;
        float spot_range = light.pos_rad.w;

        float d_dot_u = dot(spot_vec, view_ray);
        vec3 abs_view_ray = view_ray;
        float abs_d_dot_u = d_dot_u;
//        float alpha = 0.0;

//        if(abs_d_dot_u < 0.0)
//        {
//            abs_d_dot_u = -abs_d_dot_u;
//            abs_view_ray = -abs_view_ray;
//        }

        vec3 delta = -spot_pos;
        float d_dot_delta = dot(spot_vec, delta);
        float cos_sqrd = spot_angle * spot_angle;

        float c2 = abs_d_dot_u * abs_d_dot_u - cos_sqrd;
        float c1 = abs_d_dot_u * d_dot_delta - cos_sqrd * dot(abs_view_ray, delta);
        float c0 = d_dot_delta * d_dot_delta - cos_sqrd * dot(delta, delta);

        float disc = c1 * c1 - c0 * c2;

        /* there are some extra edge cases that we just won't handle because it doesn't seem to show up in
        the result. */
        if(disc > 0.0)
        {
            disc = sqrt(disc);
            float t1 = (-c1 + disc) / c2;
            float t0 = (-c1 - disc) / c2;

            float dist0 = dot(view_ray * t0 - spot_pos, spot_vec);
            float dist1 = dot(view_ray * t1 - spot_pos, spot_vec);

            /* skip this light if both hits are with the negative cone or beyond the base cap
            of the positive cone */
            if(!(dist0 < 0.0 && dist1 < 0.0) && !(dist0 > spot_range && dist1 > spot_range))
            {
                if(dist0 < 0.0)
                {
                    t0 = max(t1, r_z_near);
                    t1 = (spot_range - d_dot_delta) / abs_d_dot_u;
                }
                else if(dist1 < 0.0)
                {
                    t1 = max(t0, r_z_near);
                    t0 = (spot_range - d_dot_delta) / abs_d_dot_u;
                }

                dist0 = dot(view_ray * t0 - spot_pos, spot_vec);
                dist1 = dot(view_ray * t1 - spot_pos, spot_vec);

                if(dist1 > spot_range)
                {
                    t1 = (spot_range - d_dot_delta) / abs_d_dot_u;
                }
                if (dist0 > spot_range)
                {
                    t0 = (spot_range - d_dot_delta) / abs_d_dot_u;
                }

                t0 = max(t0, r_z_near);
                t1 = max(t1, r_z_near);

                float start = min(t0, t1);
                float end = max(t0, t1);
                float end_z = max(view_ray.z * end, pixel_z);

                vec3 point = view_ray * start;

                float alpha = abs(t1 - t0);
                float alpha_step = alpha / R_SHADOW_SAMPLE_COUNT;

                for(int sample_index = 0; sample_index < R_SHADOW_SAMPLE_COUNT && point.z >= end_z; sample_index++)
                {
                    float fallof = length(point - spot_pos);
                    float shadow = r_SpotShadow(index, point);
                    color += light.col_shd.rgb * density * alpha_step * shadow * (1.0 / fallof);
                    point += view_ray * (alpha_step + dither * fallof * 0.5);
                }
            }
        }
    }

    gl_FragColor = vec4(color, 1.0);
}
