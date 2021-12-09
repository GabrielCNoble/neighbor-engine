#include "r_defs.h"
#include "r_frag_defs.h"
#include "r_frag_uniform_defs.h"
#include "r_vert_uniform_defs.h"
#include "r_light_defs.h"

uniform sampler2D r_tex0;

void main()
{
    vec3 view_ray;
    vec2 uv = vec2(gl_FragCoord.x / float(r_width), gl_FragCoord.y / float(r_height));
    view_ray.x = ((uv.x * 2.0 - 1.0) * r_z_near) / r_projection_matrix[0][0];
    view_ray.y = ((uv.y * 2.0 - 1.0) * r_z_near) / r_projection_matrix[1][1];
    view_ray.z = -r_z_near;
    view_ray = normalize(view_ray);
    vec3 color = vec3(0);
    float density = 0.009;

    float pixel_z = -(texture(r_tex0, uv).r * 2.0 - 1.0);
    pixel_z = r_projection_matrix[3][2] / (pixel_z - r_projection_matrix[2][2]);

    for(int index = 0; index < r_spot_light_count; index++)
    {
        r_spot_data_t light = spot_lights[index];
        vec3 spot_pos = -light.pos_rad.xyz;
        vec3 spot_vec = -light.rot2.xyz;
        float spot_angle = light.rot0_angle.w;
        float spot_range = light.pos_rad.w;

        float d_dot_v = dot(view_ray, spot_vec);
        float c_dot_v = dot(spot_pos, spot_vec);
        float cos_sqrd = spot_angle * spot_angle;
        float a = d_dot_v * d_dot_v - cos_sqrd;
        float b = 2 * (d_dot_v * c_dot_v - dot(view_ray, spot_pos) * cos_sqrd);
        float c = c_dot_v * c_dot_v - dot(spot_pos, spot_pos) * cos_sqrd;

        float disc = b * b - 4 * a * c;

        if(disc >= 0.0)
        {
            if(disc > 0.0)
            {
                disc = sqrt(disc);
            }

            a *= 2;
            float t0 = max((-b + disc) / a, r_z_near);
            float t1 = max((-b - disc) / a, r_z_near);

            if(t0 < 0.0)
            {
                t0 = 0.0;
            }

            vec3 int0 = view_ray * t0;
            vec3 int1 = view_ray * t1;

            float dist0 = dot(int0 + spot_pos, spot_vec);
            float dist1 = dot(int1 + spot_pos, spot_vec);

            if(dist0 >= 0.0)
            {
                if(dist0 <= spot_range && dist1 > spot_range)
                {
                    float diff = abs(dist1 - spot_range) / abs(dist1 - dist0);
                    t1 -= diff * abs(t1 - t0);
                    dist1 = spot_range;
                }

                if(int0.z > pixel_z)
                {
                    if(dist0 <= spot_range && dist1 <= spot_range)
                    {
                        color += light.col_shd.rgb * abs(t1 - t0) * density;
                    }
                }
            }
        }
    }

    gl_FragColor = vec4(color, 1.0);
}
