#version 400 core

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

void main()
{
    gl_FragColor = vec4(1.0);
}
