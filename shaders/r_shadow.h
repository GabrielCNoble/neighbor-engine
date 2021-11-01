#ifndef R_SHDW_H
#define R_SHDW_H

#include "r_def_vunifs.h"
#include "r_light.h"

#define R_SHADOW_MAP_MIN_RESOLUTION 64
#define R_SHADOW_MAP_X_COORD_SHIFT 0
#define R_SHADOW_MAP_Y_COORD_SHIFT 8
#define R_SHADOW_MAP_RES_SHIFT 16

#define R_SHADOW_MAP_FACE_INDEX_SHIFT 0x00
#define R_SHADOW_MAP_FACE_INDEX_MASK 0x07
#define R_SHADOW_MAP_OFFSET_PACK_SHIFT 0x03
#define R_SHADOW_MAP_FACE_UV_COORD_MASK 0x03
#define R_SHADOW_MAP_FACE_U_COORD_SHIFT 0x03
#define R_SHADOW_MAP_FACE_V_COORD_SHIFT 0x05

#define R_SHADOW_MAP_OFFSET_UNPACK_MASK 0x01
#define R_SHADOW_MAP_OFFSET_X_COORD_UNPACK_SHIFT 0x03
#define R_SHADOW_MAP_OFFSET_Y_COORD_UNPACK_SHIFT 0x04

uniform usamplerCube r_indirect_texture;
uniform sampler2D r_tex_shadow_atlas;
uniform vec2 r_point_proj_params;

uniform r_shadow_indices
{
    r_index_t shadow_indices[];
};

float shadowing(uint light_index, vec3 frag_pos, vec3 frag_normal)
{
    vec3 light_vec = frag_pos - lights[light_index].pos_rad.xyz;
    uint first_tile = light_index * 6;
    vec3 frag_vec = (r_camera_matrix * vec4(light_vec.xyz, 0.0)).xyz;

    float shadow_term = 1.0;
    ivec2 coord_offset;
    ivec2 tile_coord;
    vec2 uv;
    float z_coord;
    float linear_depths[4];
    uvec4 face_indexes = texture(r_indirect_texture, frag_vec);
    uint tile_size = uint(lights[light_index].col_res.w) * R_SHADOW_MAP_MIN_RESOLUTION;
    for(int sample_index = 0; sample_index >= 0; sample_index--)
    {
        uint face_data = face_indexes[sample_index];
        uint face_index = face_data & R_SHADOW_MAP_FACE_INDEX_MASK;
        uint z_coord_index = face_index >> 1;
        uint u_coord_index = (face_data >> R_SHADOW_MAP_FACE_U_COORD_SHIFT) & R_SHADOW_MAP_FACE_UV_COORD_MASK;
        uint v_coord_index = (face_data >> R_SHADOW_MAP_FACE_V_COORD_SHIFT) & R_SHADOW_MAP_FACE_UV_COORD_MASK;

//        coord_offset.x = int((face_index >> R_SHADOW_MAP_OFFSET_X_COORD_UNPACK_SHIFT) & R_SHADOW_MAP_OFFSET_UNPACK_MASK);
//        coord_offset.y = int((face_index >> R_SHADOW_MAP_OFFSET_Y_COORD_UNPACK_SHIFT) & R_SHADOW_MAP_OFFSET_UNPACK_MASK);

//        uint z_coord_index = face_index >> 1;
        uint shadow_map = shadow_indices[first_tile + face_index].index;
        tile_coord.x = int(((shadow_map >> R_SHADOW_MAP_X_COORD_SHIFT) & 0xff) * R_SHADOW_MAP_MIN_RESOLUTION);
        tile_coord.y = int(((shadow_map >> R_SHADOW_MAP_Y_COORD_SHIFT) & 0xff) * R_SHADOW_MAP_MIN_RESOLUTION);

        vec3 cube_vec = vec3(frag_vec[u_coord_index], frag_vec[v_coord_index], -frag_vec[z_coord_index]);
        z_coord = cube_vec.z;
        uv = (cube_vec.xy / z_coord) * 0.5 + vec2(0.5);
        uv = uv * tile_size;

        float light_depth = texelFetch(r_tex_shadow_atlas, ivec2(uv.xy) + tile_coord + coord_offset, 0).x * 2.0 - 1.0;
        linear_depths[sample_index] = -r_point_proj_params.y / (light_depth + r_point_proj_params.x);
    }

    vec2 uv_frac = fract(uv);

//    float lerp0 = mix(linear_depths[0], linear_depths[1], uv_frac.x);
//    float lerp1 = mix(linear_depths[2], linear_depths[3], uv_frac.x);
//    float linear_depth = mix(lerp0, lerp1, uv_frac.y);

    float linear_depth = linear_depths[0];

    if(abs(z_coord) - 0.001 > abs(linear_depth))
    {
        shadow_term = 0.0;
    }

    return shadow_term;
}

#endif