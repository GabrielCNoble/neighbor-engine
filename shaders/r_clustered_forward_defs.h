#include "r_frag_uniform_defs.h"
#include "r_vert_uniform_defs.h"
#include "r_frag_defs.h"
#include "r_shadow_defs.h"
#include "r_light_defs.h"

#define R_CLUSTER_ROW_WIDTH 32
#define R_CLUSTER_SLICES 16
#define R_CLUSTER_ROWS 16

#define R_CLUSTER_MAX_X (R_CLUSTER_ROW_WIDTH - 1)
#define R_CLUSTER_MAX_Y (R_CLUSTER_ROWS - 1)
#define R_CLUSTER_MAX_Z (R_CLUSTER_SLICES - 1)

#define R_MIN_PARALLAX_SAMPLES 2

uniform float       r_cluster_denom;
uniform sampler2D   r_tex_albedo;
uniform sampler2D   r_tex_normal;
uniform sampler2D   r_tex_metalness;
uniform sampler2D   r_tex_roughness;
uniform sampler2D   r_tex_height;
uniform usampler3D  r_tex_clusters;
uniform int         r_parallax_samples;
uniform float       r_parallax_scale;

ivec3 get_cluster_coord(float x_coord, float y_coord, float z_coord)
{
    float x = x_coord / float(r_width);
    float y = y_coord / float(r_height);
    float z = -z_coord;
    float num = log(z / r_z_near);

    ivec3 cluster_coord;

    cluster_coord.x = int(floor(R_CLUSTER_ROW_WIDTH * x));
    cluster_coord.y = int(floor(R_CLUSTER_ROWS * y));
    cluster_coord.z = int(floor(R_CLUSTER_SLICES * (num / r_cluster_denom)));

    return cluster_coord;
}

uvec4 get_cluster(float x_coord, float y_coord, float z_coord)
{
    float x = x_coord / float(r_width);
    float y = y_coord / float(r_height);
    float z = -z_coord;
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

float r_ggx_d(float n_dot_h, float roughness)
{
    float noh = n_dot_h;
    float alpha2 = roughness * roughness * roughness;
    float noh2 = noh * noh;
    float den = noh2 * alpha2 + (1.0 - noh2);
    return ( alpha2) / (3.14159265 * den * den);
}

float r_ggx_pg(float v_dot_h, float v_dot_n, float roughness)
{
    float voh2 = v_dot_h;
    float chi = ggx_x(voh2 / v_dot_n);
    voh2 *= voh2;
    float tan2 = (1.0 - voh2) / voh2;
    float x = ggx_x(v_dot_h / v_dot_n);
    return (2.0) / (1.0 + sqrt(1 + roughness * roughness * tan2));
}

float r_ggx_g(float v_dot_h, float v_dot_n, float l_dot_h, float roughness)
{
    return r_ggx_pg(v_dot_h, v_dot_n, roughness) * r_ggx_pg(l_dot_h, v_dot_n, roughness);
}

vec3 r_schlick_fresnel(float c, vec3 f0)
{
    return f0 + (vec3(1.0) - f0) * pow(1.0 - c, 5);
}

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

vec3 r_specular_term(vec3 albedo, vec3 normal, float metalness, float roughness, vec3 view_vec, vec3 light_vec)
{
    vec3 half_vec = normalize(view_vec + light_vec);
    float v_dot_h = clamp(dot(view_vec, half_vec), 0, 1);
    float v_dot_n = clamp(dot(view_vec, normal), 0, 1);
    float l_dot_h = clamp(dot(light_vec, half_vec), 0, 1);
    float n_dot_h = clamp(dot(normal, half_vec), 0, 1);
    float d = clamp(r_ggx_d(n_dot_h, roughness), 0, 1);
    float g = clamp(r_ggx_g(v_dot_h, v_dot_n, l_dot_h, roughness), 0, 1);
    vec3 fresnel_tint = mix(albedo, vec3(1.0), metalness);
    vec3 f = r_schlick_fresnel(v_dot_h, fresnel_tint);
    float denom = clamp(4.0 * clamp(dot(normal, light_vec), 0, 1) * v_dot_n + 0.05, 0, 1);
    return (d * g * f) / denom;
}

float A = 0.15;
float B = 0.50;
float C = 0.10;
float D = 0.20;
float E = 0.02;
float F = 0.30;
float W = 11.2;

#define R_TONEMAP_OPS(color) (((color * (A * color + C * B) + D * E) / (color * (A * color + B) + D * F)) - vec3(E/F))

vec3 r_uncharted_tonemap(vec3 color)
{
    color = R_TONEMAP_OPS(color);
    vec3 denom = vec3(W);
    denom = R_TONEMAP_OPS(denom);
    color *= 1.0 / denom;
    return color;
}

vec3 r_gamma_correct(vec3 color)
{
    return pow(color, vec3(1.0 / 2.2));
}

//vec2 r_pixel_tex_coords()
//{
//    r_tex_coords = r_var_tex_coords;
//
//    return r_tex_coords;
//}

vec3 r_pixel_color(vec3 albedo, vec3 normal, float metalness, float height, float roughness, vec3 view_vec, vec3 light_vec, vec3 light_color)
{
    vec3 specular = r_specular_term(albedo, normal, metalness, roughness, view_vec, light_vec);
    vec3 diffuse = vec3(1.0) - specular;
    float n_dot_l = clamp(dot(normal, light_vec), 0.0, 1.0);
    return (albedo * diffuse * light_color + light_color * specular) * n_dot_l;
}

vec4 r_pixel_albedo(vec2 tex_coords)
{
    vec4 albedo = pow(texture(r_tex_albedo, tex_coords.xy), vec4(2.2));
    albedo.a = 1.0;
    return albedo;
}

vec3 r_pixel_normal(vec2 tex_coords)
{
    return normalize(texture(r_tex_normal, tex_coords.xy).rgb * 2.0 - vec3(1.0));

//    mat3 tbn;
//    tbn[0] = normalize(r_var_tangent);
//    tbn[2] = normalize(r_var_normal);
//    tbn[1] = cross(tbn[0], tbn[2]);
//
//    return normalize(tbn * normal);
}

float r_pixel_roughness(vec2 tex_coords)
{
    return texture(r_tex_roughness, tex_coords.xy).r;
}

float r_pixel_metalness(vec2 tex_coords)
{
    return 0.0;
}

uvec4 r_pixel_cluster()
{
    return get_cluster(gl_FragCoord.x, gl_FragCoord.y, r_var_position.z);
}














