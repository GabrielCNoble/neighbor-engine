#version 400 core
#extension GL_ARB_shading_language_420pack : require
#extension GL_ARB_enhanced_layouts : require

#define R_CLUSTER_ROW_WIDTH 32
#define R_CLUSTER_SLICES 16
#define R_CLUSTER_ROWS 16
#define R_SHADOW_MAP_MIN_RESOLUTION 64
#define R_SHADOW_MAP_X_COORD_SHIFT 0
#define R_SHADOW_MAP_Y_COORD_SHIFT 8
#define R_SHADOW_MAP_RES_SHIFT 16

#define R_SHADOW_MAP_FACE_INDEX_SHIFT 0x00
#define R_SHADOW_MAP_FACE_INDEX_MASK 0x07
#define R_SHADOW_MAP_OFFSET_PACK_SHIFT 0x03
#define R_SHADOW_MAP_FACE_UV_COORD_MASK 0x03
#define R_SHADOW_MAP_FACE_U_COORD_SHIFT 0x03
#define R_SHADOW_MAP_FACE_V_COORD_SHIFT 0x05

#define R_SHADOW_MAP_OFFSET_UNPACK_MASK 0x01
#define R_SHADOW_MAP_OFFSET_X_COORD_UNPACK_SHIFT 0x03
#define R_SHADOW_MAP_OFFSET_Y_COORD_UNPACK_SHIFT 0x04

#define R_CLUSTER_MAX_X (R_CLUSTER_ROW_WIDTH - 1)
#define R_CLUSTER_MAX_Y (R_CLUSTER_ROWS - 1)
#define R_CLUSTER_MAX_Z (R_CLUSTER_SLICES - 1)

//#define WIDTH 1300
//#define HEIGHT 760

in vec2 var_tex_coords;
in vec3 var_normal;
in vec3 var_tangent;
in vec3 var_position;

uniform float r_z_near;
uniform float r_cluster_denom;
uniform int r_width;
uniform int r_height;
uniform vec2 r_point_proj_params;
//uniform int r_cluster_row_width;
//uniform int r_cluster_rows;

uniform mat4 r_camera_matrix;

uniform sampler2D r_tex_albedo;
uniform sampler2D r_tex_normal;
uniform sampler2D r_tex_metalness;
uniform sampler2D r_tex_roughness;
uniform usampler3D r_tex_clusters;
uniform sampler2D r_tex_shadow_atlas;
uniform usamplerCube r_indirect_texture;

struct r_l_data_t
{
    vec4 pos_rad;
    vec4 col_res;
};

struct r_index_t
{
    uint index;
};

uniform r_lights
{
    r_l_data_t lights[];
};

uniform r_light_indices
{
    r_index_t indices[];
};

uniform r_shadow_indices
{
    r_index_t shadow_indices[];
};

ivec3 get_cluster_coord()
{
    float x = gl_FragCoord.x / float(r_width);
    float y = gl_FragCoord.y / float(r_height);
    float z = -var_position.z;
    float num = log(z / r_z_near);

    ivec3 cluster_coord;

    cluster_coord.x = int(floor(R_CLUSTER_ROW_WIDTH * x));
    cluster_coord.y = int(floor(R_CLUSTER_ROWS * y));
    cluster_coord.z = int(floor(R_CLUSTER_SLICES * (num / r_cluster_denom)));

    return cluster_coord;
}

uvec4 get_cluster()
{
    float x = gl_FragCoord.x / float(r_width);
    float y = gl_FragCoord.y / float(r_height);
    float z = -var_position.z;
    float num = log(z / r_z_near);

    ivec3 cluster_coord;

    cluster_coord.x = int(floor(R_CLUSTER_ROW_WIDTH * x));
    cluster_coord.y = int(floor(R_CLUSTER_ROWS * y));
    cluster_coord.z = int(floor(R_CLUSTER_SLICES * (num / r_cluster_denom)));

    return texelFetch(r_tex_clusters, cluster_coord, 0);
}

float ggx_x(float d)
{
    return d > 0.0 ? 1.0 : 0.0;
}

float ggx_d(vec3 dir, vec3 h, vec3 normal, float a)
{
//    float asqrd = a * a;
//    float ddn = dot(dir, normal);
//    float ddnsqrd = ddn * ddn;
//    float l  = ddnsqrd * (asqrd + (1.0 - ddnsqrd) / ddnsqrd);
//    return asqrd / (3.14159265 * l * l);

    float noh = dot(normal, h);
    float alpha2 = a * a * a;
    float noh2 = noh * noh;
    float den = noh2 * alpha2 + (1.0 - noh2);
    return ( alpha2) / (3.14159265 * den * den);
}

float ggx_pg(vec3 dir, vec3 normal, vec3 half_vec, float a)
{
    float voh2 = clamp(dot(dir, half_vec), 0.0, 1.0);
    float chi = ggx_x(voh2 / clamp(dot(dir, normal), 0.0, 1.0));
    voh2 *= voh2;
    float tan2 = (1.0 - voh2) / voh2;
    return (2.0 ) / (1.0 + sqrt(1 + a * a * tan2));

//    float ddn = dot(dir, normal);
//    float ddnsqrd = ddn * ddn;
//    float asqrd = a * a;
//    return (2.0 * ggx_x(ddn)) / (1.0 + sqrt(1.0 + asqrd * ((1.0 - ddnsqrd) / ddnsqrd )));
//    return (2 * ddn) / (ddn + sqrt(asqrd + (1.0 - asqrd) * ddn * ddn));
}

float ggx_g(vec3 view, vec3 light, vec3 normal, vec3 half_vec, float a)
{
    return ggx_pg(view, normal, half_vec, a) * ggx_pg(light, normal, half_vec, a);
}
//
//vec3 schlick_fresnel(float c, vec3 f0)
//{
//    return f0 + (vec3(1.0) - f0) * pow(1.0 - c, 5);
//}

//float schlick_pg(vec3 dir, vec3 normal, float a)
//{
//    float k = a * sqrt(2 / 3.14159265);
//    float d = dot(dir, normal);
//    return d / (d * (1.0 - k) + k);
//}

//float schlick_g(vec3 view, vec3 light, vec3 normal, float a)
//{
//    return schlick_pg(view, normal, a) * schlick_pg(light, normal, a);
//}

//float cook_torrance_g(vec3 view, vec3 light, vec3 h, vec3 normal)
//{
//    float a = 2.0 * dot(normal, h);
//    float b = dot(view, h);
//    return clamp((a * dot(normal, light)) / b, (a * dot(normal, view)) / b, 1.0);
//}
//
//float kelemen_g(vec3 view, vec3 light, vec3 h, vec3 normal)
//{
//    float a = dot(view, h);
//    return (dot(normal, light) * dot(normal, view)) / (a * a);
//}
//
//float implicit_g(vec3 view, vec3 light, vec3 normal)
//{
//    return dot(normal, light) * dot(normal, view);
//}

float lighting(vec3 view, vec3 light, vec3 normal, float a)
{
    vec3 h = normalize(view + light);
    return (ggx_d(view, h, normal, a) * ggx_g(view, light, normal, h, a)) / (4.0 * dot(normal, light) * dot(normal, view));
}

float shadowing(uint light_index, vec3 frag_pos, vec3 frag_normal)
{
    vec3 light_vec = frag_pos - lights[light_index].pos_rad.xyz;
    uint first_tile = light_index * 6;
    vec3 frag_vec = (r_camera_matrix * vec4(light_vec.xyz, 0.0)).xyz;

    float shadow_term = 1.0;
    ivec2 coord_offset;
    ivec2 tile_coord;
    vec2 uv;
    float z_coord;
    float linear_depths[4];
    uvec4 face_indexes = texture(r_indirect_texture, frag_vec);
    uint tile_size = uint(lights[light_index].col_res.w) * R_SHADOW_MAP_MIN_RESOLUTION;
    for(int sample_index = 0; sample_index >= 0; sample_index--)
    {
        uint face_data = face_indexes[sample_index];
        uint face_index = face_data & R_SHADOW_MAP_FACE_INDEX_MASK;
        uint z_coord_index = face_index >> 1;
        uint u_coord_index = (face_data >> R_SHADOW_MAP_FACE_U_COORD_SHIFT) & R_SHADOW_MAP_FACE_UV_COORD_MASK;
        uint v_coord_index = (face_data >> R_SHADOW_MAP_FACE_V_COORD_SHIFT) & R_SHADOW_MAP_FACE_UV_COORD_MASK;

//        coord_offset.x = int((face_index >> R_SHADOW_MAP_OFFSET_X_COORD_UNPACK_SHIFT) & R_SHADOW_MAP_OFFSET_UNPACK_MASK);
//        coord_offset.y = int((face_index >> R_SHADOW_MAP_OFFSET_Y_COORD_UNPACK_SHIFT) & R_SHADOW_MAP_OFFSET_UNPACK_MASK);

//        uint z_coord_index = face_index >> 1;
        uint shadow_map = shadow_indices[first_tile + face_index].index;
        tile_coord.x = int(((shadow_map >> R_SHADOW_MAP_X_COORD_SHIFT) & 0xff) * R_SHADOW_MAP_MIN_RESOLUTION);
        tile_coord.y = int(((shadow_map >> R_SHADOW_MAP_Y_COORD_SHIFT) & 0xff) * R_SHADOW_MAP_MIN_RESOLUTION);

        vec3 cube_vec = vec3(frag_vec[u_coord_index], frag_vec[v_coord_index], -frag_vec[z_coord_index]);
        z_coord = cube_vec.z;
        uv = (cube_vec.xy / z_coord) * 0.5 + vec2(0.5);
        uv = uv * tile_size;

        float light_depth = texelFetch(r_tex_shadow_atlas, ivec2(uv.xy) + tile_coord + coord_offset, 0).x * 2.0 - 1.0;
        linear_depths[sample_index] = -r_point_proj_params.y / (light_depth + r_point_proj_params.x);
    }

    vec2 uv_frac = fract(uv);

//    float lerp0 = mix(linear_depths[0], linear_depths[1], uv_frac.x);
//    float lerp1 = mix(linear_depths[2], linear_depths[3], uv_frac.x);
//    float linear_depth = mix(lerp0, lerp1, uv_frac.y);

    float linear_depth = linear_depths[0];

    if(abs(z_coord) - 0.001 > abs(linear_depth))
    {
        shadow_term = 0.0;
    }

    return shadow_term;
}

float A = 0.15;
float B = 0.50;
float C = 0.10;
float D = 0.20;
float E = 0.02;
float F = 0.30;
float W = 11.2;

vec3 tonemap(vec3 color)
{
    return ((color * (A * color + C * B) + D * E) / (color * (A * color + B) + D * F)) - vec3(E/F);
}

void main()
{
    vec4 albedo = texture(r_tex_albedo, var_tex_coords.xy);
    vec3 normal = normalize(texture(r_tex_normal, var_tex_coords.xy).rgb * 2.0 - vec3(1.0));
    float roughness = texture(r_tex_roughness, var_tex_coords.xy).r;

    uvec4 cluster = get_cluster();
    vec4 color = vec4(0.0);

    mat3 tbn;
    tbn[0] = normalize(var_tangent);
    tbn[2] = normalize(var_normal);
    tbn[1] = cross(tbn[0], tbn[1]);

    normal = normalize(tbn * normal);

    vec3 view_vec = normalize(-var_position);
    albedo /= 3.14159265;

    for(int light_index = 0; light_index < cluster.y; light_index++)
    {
        uint index = indices[cluster.x + light_index].index;
        r_l_data_t light = lights[index];

        vec4 light_color = vec4(light.col_res.rgb, 0.0);
        vec3 light_vec = light.pos_rad.xyz - var_position.xyz;
        vec3 orig_light_vec = light_vec;
        float dist = length(light_vec);
        float limit = light.pos_rad.w - dist;
        limit = clamp(limit, 0.0, 1.0);
        light_vec = light_vec / dist;
        dist *= dist;


        float shadow = shadowing(index, var_position.xyz, var_normal.xyz);
        float spec = clamp(lighting(view_vec, light_vec, normal.xyz, roughness), 0.0, 1.0);
        float diff = (1.0 - spec);
        float c = clamp(dot(normal, light_vec), 0.0, 1.0) * limit;
        color += ((albedo * diff * light_color + light_color * spec) * c) * shadow;
    }

    color.rgb = tonemap(color.rgb * 2.0);
    color.rgb *= 1.0 / tonemap(vec3(W));
    gl_FragColor = color + albedo * 0.12;

//    gl_FragColor = color;
}




