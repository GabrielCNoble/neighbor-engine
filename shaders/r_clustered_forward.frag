#include "r_defs.h"
#include "r_clustered_forward_defs.h"

void main()
{
    vec4 albedo = r_pixel_albedo();
    vec3 normal = r_pixel_normal();
    float roughness = r_pixel_roughness();
    float metalness = r_pixel_metalness();
    uvec4 cluster = r_pixel_cluster();

    vec3 color = vec3(0.0);

    vec3 view_vec = normalize(-r_var_position);
    albedo /= 3.14159265;


    uint point_light_start = cluster.x;
    uint point_light_count = cluster.z & 0xffff;

    uint spot_light_start = cluster.y;
    uint spot_light_count = (cluster.z >> 16) & 0xffff;

    for(uint light_index = 0; light_index < point_light_count; light_index++)
    {
        uint index = indices[point_light_start + light_index].index;
        r_point_data_t light = point_lights[index];

        vec3 light_color = light.col_shd.rgb;
        vec3 light_vec = light.pos_rad.xyz - r_var_position.xyz;
        vec3 light_dir = light_vec;
        float dist = length(light_vec);
        float limit = light.pos_rad.w - dist;
        limit = clamp(limit, 0.0, 1.0);
        light_dir = light_vec / dist;
        float fallof = 1.0 / (dist * dist);

        uint shadow_map = floatBitsToUint(light.col_shd.w);
        float shadow = r_CubeShadow(shadow_map, -light_vec);
        light_color *= limit * fallof * shadow;
        vec3 pixel_color = r_pixel_color(albedo.rgb, normal, metalness, 1, roughness, view_vec, light_dir, light_color);
        color += pixel_color;
    }

    for(uint light_index = 0; light_index < spot_light_count; light_index++)
    {
        uint index = indices[spot_light_start + light_index].index;
        r_spot_data_t light = spot_lights[index];

        vec3 light_color = light.col_shd.xyz;
        vec3 light_vec = light.pos_rad.xyz - r_var_position.xyz;
        float dist = length(light_vec);
        float limit = light.pos_rad.w - dist;
        limit = clamp(limit, 0.0, 1.0);
        vec3 light_dir = light_vec / dist;
        float fallof = 1.0 / (dist * dist);

        float edge0 = light.rot0_angle.w;
        float edge1 = light.rot0_angle.w + light.rot1_soft.w;
        limit *= smoothstep(edge0, edge1, dot(light_dir, light.rot2.xyz));
        float shadow = r_SpotShadow(index, r_var_position.xyz);
        light_color *= limit * fallof * shadow;
        vec3 pixel_color = r_pixel_color(albedo.rgb, normal, metalness, 1.0, roughness, view_vec, light_dir, light_color);
        color += pixel_color;
    }

    color = r_gamma_correct(r_uncharted_tonemap(color + albedo.rgb * 0.01));
    gl_FragColor = vec4(color, 1.0);
}




