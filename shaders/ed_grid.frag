#version 400 core

in vec2 tex_coords;
in float position_z;

float grid0_divs = 500.0;
float grid1_divs = grid0_divs * 10.0;
float grid2_divs = grid1_divs * 10.0;
float line_width = 2;

#define MAX_ALPHA 0.35

void main()
{
    vec2 tex_coords_dx = abs(dFdx(tex_coords));
    vec2 tex_coords_dy = abs(dFdy(tex_coords));

    vec2 grid0_tex_coord = vec2(fract(tex_coords.x * grid0_divs), fract(tex_coords.y * grid0_divs));
    vec2 grid1_tex_coord = vec2(fract(tex_coords.x * grid1_divs), fract(tex_coords.y * grid1_divs));
    vec2 grid2_tex_coord = vec2(fract(tex_coords.x * grid2_divs), fract(tex_coords.y * grid2_divs));

    float alpha = MAX_ALPHA;
    vec3 color = vec3(0.8);

    float tex_coords_delta_x = max(tex_coords_dx.x, tex_coords_dy.x) * line_width;
    float tex_coords_delta_y = max(tex_coords_dx.y, tex_coords_dy.y) * line_width;

    if(grid0_tex_coord.x > tex_coords_delta_x * grid0_divs && grid0_tex_coord.y > tex_coords_delta_y * grid0_divs)
    {
        if(grid1_tex_coord.x > tex_coords_delta_x * grid1_divs && grid1_tex_coord.y > tex_coords_delta_y * grid1_divs)
        {
            if(grid2_tex_coord.x > tex_coords_delta_x * grid2_divs && grid2_tex_coord.y > tex_coords_delta_y * grid2_divs)
            {
                discard;
            }

            alpha = clamp(MAX_ALPHA - position_z * 1.0, 0.0, MAX_ALPHA);
            color = vec3(0.4);
        }
        else
        {
            alpha = clamp(MAX_ALPHA - position_z * 0.3, 0.0, MAX_ALPHA);
            color = vec3(0.6);
        }
    }
    else
    {
        alpha = clamp(MAX_ALPHA - position_z * 0.005, 0.0, MAX_ALPHA);
    }

    if(alpha == 0.0)
    {
        discard;
    }

    gl_FragColor = vec4(color, alpha);
}
