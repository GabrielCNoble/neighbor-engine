#version 400 core

layout (location = 0) in vec4 r_position;

void main()
{
    gl_Position = r_position;
}
