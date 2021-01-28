#version 400 core
layout (location = 0)in vec4 r_position;
layout (location = 1)in vec4 r_color;
layout (location = 3)in vec2 r_tex_coords;

uniform mat4 d_mvp;

out vec4 color;

void main()
{
    gl_Position = d_mvp * r_position;
    color = r_color;
}
