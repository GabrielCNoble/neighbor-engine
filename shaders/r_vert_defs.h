#include "r_default_attribs.h"

out vec2 r_var_tex_coords;
out vec3 r_var_normal;
out vec3 r_var_tangent;
out vec3 r_var_position;


void r_set_position(vec3 position)
{
    r_var_position = position;
}

void r_set_normal(vec3 normal)
{
    r_var_normal = normal;
}

void r_set_tangent(vec3 tangent)
{
    r_var_tangent = tangent;
}

void r_set_tex_coords(vec2 tex_coords)
{
    r_var_tex_coords = tex_coords;
}
