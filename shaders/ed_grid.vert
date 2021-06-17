#version 400 core
layout(location = 0)in vec4 r_position;
layout(location = 3)in vec2 r_tex_coords;

uniform mat4 r_model_view_projection_matrix;

out vec2 tex_coords;
out float position_z;

void main()
{
    gl_Position = r_model_view_projection_matrix * r_position;
    position_z = gl_Position.z;
    tex_coords = r_tex_coords;
}
