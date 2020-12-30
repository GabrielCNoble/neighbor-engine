#version 400 core

in vec2 tex_coords;
in vec4 normal;

uniform sampler2D d_tex0;

void main()
{
    gl_FragColor = texture(d_tex0, tex_coords.xy);
//    gl_FragColor = vec4(tex_coords.xy, 0.0, 1.0);
//    gl_FragColor = abs(normal);
}
