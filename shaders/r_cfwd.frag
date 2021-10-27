#include "r_defs.h"
#include "r_def_cfwd.h"

void main()
{
    vec4 albedo = r_pixel_albedo();
    vec3 normal = r_pixel_normal();
    float roughness = r_pixel_roughness();
    uvec4 cluster = r_pixel_cluster();

    vec4 color = vec4(0.0);

    vec3 view_vec = normalize(-r_var_position);
    albedo /= 3.14159265;

    for(int light_index = 0; light_index < cluster.y; light_index++)
    {
        uint index = indices[cluster.x + light_index].index;
        r_l_data_t light = lights[index];

        vec4 light_color = vec4(light.col_res.rgb, 0.0);
        vec3 light_vec = light.pos_rad.xyz - r_var_position.xyz;
        vec3 orig_light_vec = light_vec;
        float dist = length(light_vec);
        float limit = light.pos_rad.w - dist;
        limit = clamp(limit, 0.0, 1.0);
        light_vec = light_vec / dist;
        dist *= dist;

        float shadow = shadowing(index, r_var_position.xyz, r_var_normal.xyz);
        float spec = clamp(lighting(view_vec, light_vec, normal.xyz, roughness), 0.0, 1.0);
        float diff = (1.0 - spec);
        float c = clamp(dot(normal, light_vec), 0.0, 1.0) * limit;
        color += ((albedo * diff * light_color + light_color * spec) * c) * shadow;
    }

    color.rgb = tonemap(color.rgb * 2.0);
    color.rgb *= 1.0 / tonemap(vec3(W));
    color.a = 1.0;
    gl_FragColor = color + albedo * 0.12;
}




