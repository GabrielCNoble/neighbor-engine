#version 400 core

uniform int ed_type;
uniform int ed_index;

void main()
{
    gl_FragColor = vec4(intBitsToFloat(ed_type), intBitsToFloat(ed_index), 0.0, 1.0);
}
