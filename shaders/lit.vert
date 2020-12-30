#version 400 core
layout(location = 0)in vec4 d_position;
layout(location = 1)in vec4 d_normal;
layout(location = 2)in vec2 d_tex_coords;

uniform mat4 d_mvp;

out vec2 tex_coords;
out vec4 normal;


void main()
{
    gl_Position = d_mvp * d_position;
    tex_coords = d_tex_coords;
    normal = d_normal;
}
