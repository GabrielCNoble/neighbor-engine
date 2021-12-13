#include "r_defs.h"
#include "r_clustered_forward_defs.h"

void main()
{
    vec4 albedo = r_pixel_albedo();
    vec3 normal = r_pixel_normal();
    float roughness = r_pixel_roughness();
    uvec4 cluster = r_pixel_cluster();

    vec4 color = vec4(0.0);

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

        vec4 light_color = vec4(light.col_shd.rgb, 0.0);
        vec3 light_vec = light.pos_rad.xyz - r_var_position.xyz;
        vec3 orig_light_vec = light_vec;
        float dist = length(light_vec);
        float limit = light.pos_rad.w - dist;
        limit = clamp(limit, 0.0, 1.0);
        light_vec = light_vec / dist;
        dist *= dist;

        uint shadow_map = floatBitsToUint(light.col_shd.w);
        float shadow = r_CubeShadow(shadow_map, -orig_light_vec);
        float spec = clamp(lighting(view_vec, light_vec, normal.xyz, roughness), 0.0, 1.0);
        float diff = (1.0 - spec);
        float c = clamp(dot(normal, light_vec), 0.0, 1.0) * limit;
        color += ((albedo * diff * light_color + light_color * spec) * c) * shadow;
    }

    for(uint light_index = 0; light_index < spot_light_count; light_index++)
    {
        uint index = indices[spot_light_start + light_index].index;
        r_spot_data_t light = spot_lights[index];

        vec4 light_color = vec4(light.col_shd.xyz, 0.0);
        vec3 light_vec = light.pos_rad.xyz - r_var_position.xyz;
        float dist = length(light_vec);
        float limit = light.pos_rad.w - dist;
        limit = clamp(limit, 0.0, 1.0);
        vec3 normalized_light_vec = light_vec / dist;
        dist *= dist;

        float edge0 = light.rot0_angle.w;
        float edge1 = light.rot0_angle.w + light.rot1_soft.w;
        limit *= smoothstep(edge0, edge1, dot(normalized_light_vec, light.rot2.xyz));
//        vec2 uv;
        float shadow = r_SpotShadow(index, r_var_position.xyz);

//        color = vec4(uv, shadow, 0.0);

        float spec = clamp(lighting(view_vec, normalized_light_vec, normal.xyz, roughness), 0.0, 1.0);
        float diff = (1.0 - spec);
        float c = clamp(dot(normal, normalized_light_vec), 0.0, 1.0) * limit;
        color += ((albedo * diff * light_color + light_color * spec) * c) * shadow;
    }

    color.rgb = tonemap(color.rgb * 2.0);
    color.rgb *= 1.0 / tonemap(vec3(W));
    color.a = 1.0;
    gl_FragColor = color + albedo * 0.22;

//    gl_FragColor = vec4(r_var_tex_coords.xy, 0.0, 1.0);
}




