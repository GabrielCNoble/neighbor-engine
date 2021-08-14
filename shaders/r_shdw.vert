#version 400 core
layout(location = 0)in vec4 r_position;
layout(location = 1)in vec4 r_normal;
layout(location = 2)in vec4 r_tangent;
layout(location = 3)in vec2 r_tex_coords;

uniform mat4 r_model_view_projection_matrix;

void main()
{
    gl_Position = r_model_view_projection_matrix * r_position;
}
