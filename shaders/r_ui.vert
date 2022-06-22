#include "r_defs.h"
#include "r_vert_defs.h"
#include "r_vert_uniform_defs.h"

out vec4 color;
out vec2 tex_coords;

void main()
{
    gl_Position = r_model_view_projection_matrix * r_position;
    color = r_color;
    tex_coords = r_tex_coords;
}
