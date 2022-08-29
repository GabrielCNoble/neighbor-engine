#include "r_defs.h"
#include "r_default_attribs.h"

uniform mat4 r_model_view_projection_matrix;

out vec2 tex_coords;
out float position_z;

void main()
{
    vec4 position = r_model_view_projection_matrix * r_position;
    gl_Position = position;
    position_z = position.z;
    tex_coords = r_tex_coords;
}
