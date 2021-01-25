#version 400 core
#extension GL_ARB_shading_language_420pack : require

#define R_CLUSTER_ROW_WIDTH 24
#define R_CLUSTER_SLICES 16
#define R_CLUSTER_ROWS 16

in vec2 tex_coords;
in vec4 normal;
in vec4 position;

uniform sampler2D d_tex0;
uniform usampler3D r_clusters;

struct r_l_data_t
{
    vec4 pos_rad;
    vec4 col_type;
};

uniform r_lights
{
    r_l_data_t lights[];
};

uniform r_light_indices
{
    int indices[];
};

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
    vec4 albedo = texture(d_tex0, tex_coords.xy);
    uvec4 cluster = texture(r_clusters, ivec3(0, 0, 0));
    vec4 color = vec4(0.0);
    
    vec3 nnormal = normalize(normal.xyz);
    vec3 view_vec = normalize(-position.xyz);
    for(int light_index = 0; light_index < cluster.y; light_index++)
    {
        r_l_data_t light = lights[cluster.x + light_index];
        vec4 light_color = vec4(light.col_type.rgb, 0.0);
        vec3 light_vec = light.pos_rad.xyz - position.xyz;
        float dist = length(light_vec);
        light_vec = light_vec / dist;
        dist *= dist;

        float x = clamp(dot(nnormal, light_vec), 0.0, 1.0) * 10.0;
        float spec = clamp(lighting(view_vec, light_vec, nnormal.xyz, 0.1), 0.0, 1.0);
        float diff = 1.0 - spec;
        color += (((albedo / 3.14159265) * diff * light_color + light_color * spec) * x) / dist ;
    }
    
    color.rgb = tonemap(color.rgb * 2.0);
    color.rgb *= 1.0 / tonemap(vec3(W));
//    float A = 0.15;
//    float B = 0.50;
//    float C = 0.10;
//    float D = 0.20;
//    float E = 0.02;
//    float F = 0.30;
//    float W = 11.2;
//    
//    color *= 1.0 / (((W * (A * W + C * B) + D * E) / (W * (A * W + B) + D * F)) - E/F);
    gl_FragColor = color;
//    gl_FragColor = vec4(normal.xyz, 1.0);
}




