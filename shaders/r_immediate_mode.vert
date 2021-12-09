#version 400 core
layout (location = 0)in vec4 r_position;
layout (location = 1)in vec4 r_color;
layout (location = 3)in vec2 r_tex_coords;

uniform mat4 r_model_view_projection_matrix;

out vec4 color;

void main()
{
    gl_Position = r_model_view_projection_matrix * r_position;
    color = r_color;
}
