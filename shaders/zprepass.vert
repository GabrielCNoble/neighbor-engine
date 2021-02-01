#version 400 core
layout(location = 0)in vec4 r_position;


uniform mat4 d_mvp;


void main()
{
    gl_Position = d_mvp * r_position;
}
