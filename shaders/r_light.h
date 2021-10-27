#ifndef R_LIGHT_H
#define R_LIGHT_H

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

#endif
