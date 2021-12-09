#version 400 core
layout(location = 0)in vec4 r_position;


uniform mat4 r_model_view_projection_matrix;


void main()
{
    gl_Position = r_model_view_projection_matrix * r_position;
}
