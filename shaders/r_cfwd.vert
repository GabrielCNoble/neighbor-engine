#version 400 core
layout(location = 0)in vec4 r_position;
layout(location = 1)in vec4 r_normal;
layout(location = 2)in vec4 r_tangent;
layout(location = 3)in vec2 r_tex_coords;

uniform mat4 r_model_view_projection_matrix;
uniform mat4 r_model_view_matrix;

out vec2 var_tex_coords;
out vec3 var_normal;
out vec3 var_tangent;
out vec3 var_position;


void main()
{
    gl_Position = r_model_view_projection_matrix * r_position;
    var_tex_coords = r_tex_coords.xy;
    var_position = (r_model_view_matrix * vec4(r_position.xyz, 1.0)).xyz;
    var_normal = (r_model_view_matrix * vec4(r_normal.xyz, 0.0)).xyz;
    var_tangent = (r_model_view_matrix * vec4(r_tangent.xyz, 0.0)).xyz;
}
