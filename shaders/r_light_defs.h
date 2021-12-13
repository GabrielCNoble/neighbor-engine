#ifndef R_LIGHT_H
#define R_LIGHT_H

struct r_point_data_t
{
    vec4 pos_rad;
    vec4 col_shd;
};

struct r_spot_data_t
{
    vec4 pos_rad;
    vec4 col_shd;
    vec4 rot0_angle;
    vec4 rot1_soft;
    vec4 rot2;
    vec4 proj;
};

struct r_index_t
{
    uint index;
};

layout(std430) buffer r_point_lights
{
    r_point_data_t point_lights[];
};

layout(std430) buffer r_spot_lights
{
    r_spot_data_t spot_lights[];
};

layout(std430) buffer r_light_indices
{
    r_index_t indices[];
};

uniform uint r_spot_light_count;
uniform uint r_point_light_count;

#endif
