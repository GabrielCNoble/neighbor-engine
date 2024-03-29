#version 400 core

uniform int ed_obj_type;
uniform int ed_obj_index;
uniform int ed_obj_extra0;
uniform int ed_obj_extra1;

void main()
{
    gl_FragColor = vec4(intBitsToFloat(ed_obj_index),
                        intBitsToFloat(ed_obj_type),
                        intBitsToFloat(ed_obj_extra0),
                        intBitsToFloat(ed_obj_extra1));
}
