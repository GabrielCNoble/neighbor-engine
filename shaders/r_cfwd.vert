#include "r_defs.h"
#include "r_def_vert.h"
#include "r_def_vunifs.h"

//uniform mat4 r_model_view_projection_matrix;
//uniform mat4 r_model_view_matrix;

//out vec2 r_var_tex_coords;
//out vec3 r_var_normal;
//out vec3 r_var_tangent;
//out vec3 r_var_position;


void main()
{
    gl_Position = r_model_view_projection_matrix * r_position;
    r_set_position((r_model_view_matrix * vec4(r_position.xyz, 1.0)).xyz);
    r_set_normal((r_model_view_matrix * vec4(r_normal.xyz, 0.0)).xyz);
    r_set_tangent((r_model_view_matrix * vec4(r_tangent.xyz, 0.0)).xyz);
    r_set_tex_coords(r_tex_coords.xy);

//    r_var_tex_coords = r_tex_coords.xy;
//    r_var_position = (r_model_view_matrix * vec4(r_position.xyz, 1.0)).xyz;
//    r_var_normal = (r_model_view_matrix * vec4(r_normal.xyz, 0.0)).xyz;
//    r_var_tangent = (r_model_view_matrix * vec4(r_tangent.xyz, 0.0)).xyz;
}
