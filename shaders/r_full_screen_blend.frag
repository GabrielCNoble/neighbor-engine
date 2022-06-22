#include "r_defs.h"
#include "r_frag_uniform_defs.h"

uniform sampler2D r_tex0;

void main()
{
    vec2 uv = vec2(gl_FragCoord.x / float(r_width), gl_FragCoord.y / float(r_height));
    gl_FragColor = texture(r_tex0, uv);
}
