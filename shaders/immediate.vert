#version 400 core
layout (location = 0)in vec4 d_position;
layout (location = 1)in vec4 d_color;
layout (location = 2)in vec2 d_tex_coords;

uniform mat4 d_mvp;

out vec4 color;

void main()
{
    gl_Position = d_mvp * d_position;
    color = d_color;
}
