#version 400 core

uniform int ed_type;
uniform int ed_index;

void main()
{
    gl_FragColor = vec4(intBitsToFloat(ed_index), intBitsToFloat(ed_type), 0.0, 1.0);
}
