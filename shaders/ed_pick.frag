#version 400 core

uniform int ed_obj_type;
uniform int ed_obj_index;

void main()
{
    gl_FragColor = vec4(intBitsToFloat(ed_obj_index), intBitsToFloat(ed_obj_type), 0.0, 1.0);
}
