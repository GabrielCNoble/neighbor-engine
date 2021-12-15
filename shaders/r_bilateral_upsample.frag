#include "r_defs.h"
#include "r_frag_uniform_defs.h"
#include "r_vert_uniform_defs.h"

uniform sampler2D r_tex0;
uniform sampler2D r_tex1;

//float kernel[3][3] =
//{
//    {0.015824233393775727, 0.016728236160121757, 0.015824233393775727},
//    {0.016728236160121757, 0.01768388256576615, 0.016728236160121757},
//    {0.015824233393775727, 0.016728236160121757, 0.015824233393775727},
//};

//float kernel[5][5] =
//{
//    {0.005424915849561177, 0.005760373910997226, 0.005876741183054534, 0.005760373910997226, 0.005424915849561177},
//    {0.005760373910997226, 0.006116575540463281, 0.006240138562755518, 0.006116575540463281, 0.005760373910997226},
//    {0.005876741183054534, 0.006240138562755518, 0.006366197723675813, 0.006240138562755518, 0.005876741183054534},
//    {0.005760373910997226, 0.006116575540463281, 0.006240138562755518, 0.006116575540463281, 0.005760373910997226},
//    {0.005424915849561177, 0.005760373910997226, 0.005876741183054534, 0.005760373910997226, 0.005424915849561177},
//};
//
//float gaussk(float d, float n)
//{
//    return (1.0 / (2 * 3.14159265 * n * n)) * exp(-(d * d) / (2 * n * n));
//}

void main()
{
//    vec2 uv = vec2(gl_FragCoord.x / float(r_width), gl_FragCoord.y / float(r_height));
//    gl_FragColor = texture(r_tex0, uv);

    ivec2 coords = ivec2(int(gl_FragCoord.x), int(gl_FragCoord.y));
    ivec2 coords2 = coords / 2;
    float proj_a = r_projection_matrix[2][2];
    float proj_b = r_projection_matrix[3][2];
    float depth = abs(-proj_b / ((texelFetch(r_tex1, coords, 0).r * 2.0 - 1.0) + proj_a));
//    float depth = texelFetch(r_tex1, coords, 0).r;

//    int x_offset = 1;
//    int y_offset = 1;
//
//    if(coords.x % 2 == 1)
//    {
//        coords2.x++;
//        x_offset = -1;
//    }
//
//    if(coords.y % 2 == 1)
//    {
//        coords2.y++;
//        y_offset = -1;
//    }

    ivec2 offsets[] =
    {
        ivec2(0, 0),
        ivec2(0, 1),
        ivec2(1, 0),
        ivec2(1, 1),
    };

    vec3 total_color = vec3(0);
    float total_weight = 0.0;

    for(int index = 0; index < 4; index++)
    {
//        ivec2 color_coords = coords2 + offsets[index];
////        vec3 base_color = texelFetch(r_tex0, color_coords, 0).rgb;
//        vec3 color = vec3(0);
//        vec3 total_color_weight;
//
//        color += texelFetch(r_tex0, color_coords, 0).rgb;
//        color += texelFetch(r_tex0, color_coords + ivec2(-1, 0), 0).rgb * 0.5;
//        color += texelFetch(r_tex0, color_coords + ivec2(1, 0), 0).rgb * 0.5;
//        color += texelFetch(r_tex0, color_coords + ivec2(0, -1), 0).rgb * 0.5;
//        color += texelFetch(r_tex0, color_coords + ivec2(0, 1), 0).rgb * 0.5;
//
//        color /= 2.5;


//        for(int x = 0; x < 5; x++)
//        {
//            for(int y = 0; y < 5; y++)
//            {
//                vec3 tap_color = texelFetch(r_tex0, color_coords + ivec2(x - 2, y - 2), 0).rgb;
////                float color_weight = kernel[x][y] * gaussk(length(tap_color - base_color), 1000.1);
//                vec3 color_weight;
////                color_weight.r = kernel[x][y] * gaussk(length(tap_color.r - base_color.r), 1.1);
////                color_weight.g = kernel[x][y] * gaussk(length(tap_color.g - base_color.g), 1.1);
////                color_weight.b = kernel[x][y] * gaussk(length(tap_color.b - base_color.b), 1.1);
//
////                total_color_weight += color_weight;
//                color += tap_color * kernel[x][y] * 5;
//
////                 * kernel[x][y];
////                color += texelFetch(r_tex0, color_coords + ivec2(x - 1, y - 1), 0).rgb * kernel[x][y];
//            }
//        }

//        color.r /= total_color_weight.r;
//        color.g /= total_color_weight.g;
//        color.b /= total_color_weight.b;

        vec3 color = texelFetch(r_tex0, coords2 + offsets[index], 0).rgb;
        float depth2 = abs(-proj_b / ((texelFetch(r_tex1, coords + offsets[index], 0).r * 2.0 - 1.0) + proj_a));
        float diff = abs(depth2 - depth);
        float weight = max(0.0, 1.0 - diff * diff);
        total_color += color * weight;
        total_weight += weight;
    }

    total_color /= (total_weight + 0.0001);
    gl_FragColor = vec4(total_color, 1.0);
}
