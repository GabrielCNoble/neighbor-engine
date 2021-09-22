#version 400 core
layout(location = 0)in vec4 r_position;

uniform mat4 r_model_view_projection_matrix;
uniform mat4 r_view_projection_matrix;
uniform mat4 r_model_matrix;

void main()
{
    gl_Position = r_view_projection_matrix * r_model_matrix * r_position;
}
