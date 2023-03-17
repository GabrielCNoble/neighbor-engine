#include "r_defs.h"
#include "r_clustered_forward_defs.h"

//#define R_PARALLAX_SAMPLE_COUNT 8
#define R_PARALLAX_MIN_SAMPLE_COUNT 2
#define R_PARALLAX_MAX_SAMPLE_COUNT 32
void main()
{
    vec3 view_vec = normalize(-r_var_position);

//    vec2 tex_coords = r_pixel_tex_coords();
//    r_tex_coords = r_var_tex_coords;
    vec3 surface_normal = normalize(r_var_normal);
    vec3 surface_tangent = normalize(r_var_tangent);
//    vec3 poff_b = cross(surface_normal, normalize(view_vec));
//    vec3 poff = cross(surface_normal, poff_b);

    mat3 tbn;
    tbn[0] = surface_tangent;
    tbn[2] = surface_normal;
    tbn[1] = (cross(tbn[2], tbn[0]));

    tbn = transpose(tbn);

    vec3 tangent_view_vec = tbn * view_vec;
    vec3 tangent_normal = tbn * surface_normal;
//    vec3 tangent_tangent = tbn * surface_tangent;
//    vec3 tangent_binormal = cross(tangent_tangent, tangent_normal);
//    vec3 poff_b = cross(tangent_normal, tangent_view_vec);
//    vec3 poff = cross(tangent_normal, poff_b);

    vec2 poff = normalize(tangent_view_vec.xy);
    float plen = -(sqrt(1.0 - tangent_view_vec.z * tangent_view_vec.z) / tangent_view_vec.z);
    vec2 parallax_offset_vec = -poff * plen  * 0.02;

    float x;
    float y;
    float xh;
    float yh;


    int sample_count = R_PARALLAX_MIN_SAMPLE_COUNT + int((1 - dot(tangent_view_vec, tangent_normal)) * r_max_parallax_samples);
    float height_step = 1.0 / float(sample_count);
    float cur_view_height = 1.0;
    float cur_parallax_offset = height_step;
    float prev_height_sample;
    float cur_height_sample;

    vec2 tex_coords_dx = dFdx(r_var_tex_coords);
    vec2 tex_coords_dy = dFdy(r_var_tex_coords);

    cur_height_sample = textureGrad(r_tex_height, r_var_tex_coords - parallax_offset_vec * cur_parallax_offset, tex_coords_dx, tex_coords_dy).r;
    prev_height_sample = cur_height_sample;
    for(int index = 0; index <= sample_count; index++)
    {
        if(cur_height_sample > cur_view_height)
        {
            x = cur_view_height - height_step;
            y = cur_view_height;

            xh = cur_height_sample;
            yh = prev_height_sample;
            break;
        }

        cur_view_height -= height_step;
        cur_parallax_offset += height_step;
        prev_height_sample = cur_height_sample;
        cur_height_sample = textureGrad(r_tex_height, r_var_tex_coords - parallax_offset_vec * cur_parallax_offset, tex_coords_dx, tex_coords_dy).r;
    }

//    float h0 = texture(r_tex_height, r_var_tex_coords - parallax_offset * 1.0).r;
//    float h1 = texture(r_tex_height, r_var_tex_coords - parallax_offset * 0.875).r;
//    float h2 = texture(r_tex_height, r_var_tex_coords - parallax_offset * 0.750).r;
//    float h3 = texture(r_tex_height, r_var_tex_coords - parallax_offset * 0.625).r;
//    float h4 = texture(r_tex_height, r_var_tex_coords - parallax_offset * 0.500).r;
//    float h5 = texture(r_tex_height, r_var_tex_coords - parallax_offset * 0.375).r;
//    float h6 = texture(r_tex_height, r_var_tex_coords - parallax_offset * 0.250).r;
//    float h7 = texture(r_tex_height, r_var_tex_coords - parallax_offset * 0.125).r;
//
//    if(h7 > 0.875)      { x = 0.875; y = 1.000; xh = h7; yh = h7; }
//    else if(h6 > 0.750) { x = 0.750; y = 0.875; xh = h6; yh = h7; }
//    else if(h5 > 0.625) { x = 0.625; y = 0.750; xh = h5; yh = h6; }
//    else if(h4 > 0.500) { x = 0.500; y = 0.625; xh = h4; yh = h5; }
//    else if(h3 > 0.375) { x = 0.375; y = 0.500; xh = h3; yh = h4; }
//    else if(h2 > 0.250) { x = 0.250; y = 0.375; xh = h2; yh = h3; }
//    else if(h1 > 0.125) { x = 0.125; y = 0.250; xh = h1; yh = h2; }
//    else                { x = 0.000; y = 0.125; xh = h0; yh = h1; }

    float parallax_amount = (x * (y - yh) - y * (x - xh)) / ((y - yh) - (x - xh));
    vec2 tex_coords = r_var_tex_coords - (parallax_offset_vec * (1 - parallax_amount));

    vec4 albedo = r_pixel_albedo(tex_coords);
    vec3 normal = r_pixel_normal(tex_coords);
    float roughness = r_pixel_roughness(tex_coords);
    float metalness = r_pixel_metalness(tex_coords);
    uvec4 cluster = r_pixel_cluster();

    vec3 color = vec3(0.0);
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




