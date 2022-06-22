#version 400 core

in vec4 color;
in vec2 tex_coords;

uniform sampler2D r_tex0;

void main()
{
    vec4 text_color = texture(r_tex0, tex_coords);
    gl_FragColor = color * text_color;
//    gl_FragColor = color;
}
