#ifndef R_LIGHT_H
#define R_LIGHT_H

struct r_point_data_t
{
    vec4 pos_rad;
    vec4 col_res;
};

struct r_spot_data_t
{
    vec4 pos_rad;
    vec4 rot0_r;
    vec4 rot1_g;
    vec4 rot2_b;
};

struct r_index_t
{
    uint index;
};

uniform r_point_lights
{
    r_point_data_t point_lights[];
};

uniform r_spot_lights
{
    r_spot_data_t spot_lights[];
};

uniform r_light_indices
{
    r_index_t indices[];
};

#endif
