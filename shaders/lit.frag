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

void main()
{
    vec4 albedo = texture(d_tex0, tex_coords.xy);
    uvec4 cluster = texture(r_clusters, ivec3(0, 0, 0));
    vec4 color = vec4(0.0);
    
    for(int light_index = 0; light_index < cluster.y; light_index++)
    {
        r_l_data_t light = lights[cluster.x + light_index];
        
        vec3 light_vec = light.pos_rad.xyz - position.xyz;
        float dist = length(light_vec);
//        float frac = clamp(1.0 - (dist / light.pos_rad.w), 0.0, 1.0);
        float val = (1.0 / (dist));
        color += albedo * vec4(light.col_type.rgb, 0.0) * dot(normalize(light_vec), normal.xyz) * val;
    }
    
    gl_FragColor = color;
}




