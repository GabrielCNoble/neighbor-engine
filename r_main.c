#include "r_main.h"
#include "SDL2/SDL.h"
#include "GL/glew.h"
#include "stb/stb_image.h"
#include "dstuff/ds_slist.h"
#include "dstuff/ds_file.h"
#include "dstuff/ds_mem.h"
#include "dstuff/ds_obj.h"
#include "dstuff/ds_alloc.h"
#include "r_draw.h"
#include "stb/stb_include.h"
#include "anim.h"

//extern struct stack_list_t d_shaders;
//extern struct stack_list_t d_textures;
//extern struct stack_list_t d_materials;
//extern struct stack_list_t d_models;
//extern struct d_texture_t *d_default_texture;
//
//extern struct ds_heap_t d_vertex_heap;
//extern struct ds_heap_t d_index_heap;



struct ds_list_t r_world_cmds;
struct ds_list_t r_entity_cmds;
struct ds_list_t r_shadow_cmds;
struct ds_list_t r_immediate_cmds;
struct ds_list_t r_immediate_data;


//struct list_t r_sorted_batches;
//struct list_t r_immediate_batches;
struct ds_slist_t r_shaders;
struct ds_slist_t r_textures;
struct ds_slist_t r_materials;
struct ds_slist_t r_models;
struct ds_slist_t r_lights;
struct ds_slist_t r_vis_items;
struct ds_list_t r_visible_lights;

uint32_t r_vertex_buffer;
struct ds_heap_t r_vertex_heap;
uint32_t r_index_buffer;
struct ds_heap_t r_index_heap;
uint32_t r_immediate_cursor;
uint32_t r_immediate_vertex_buffer;
uint32_t r_immediate_index_buffer;
//struct ds_heap_t r_immediate_heap;

uint32_t r_vao;
struct r_shader_t *r_z_prepass_shader;
struct r_shader_t *r_lit_shader;
struct r_shader_t *r_immediate_shader;
struct r_shader_t *r_current_shader;
struct r_shader_t *r_shadow_shader;

struct r_model_t *test_model;
struct r_texture_t *r_default_texture;
struct r_texture_t *r_default_albedo_texture;
struct r_texture_t *r_default_normal_texture;
struct r_texture_t *r_default_height_texture;
struct r_texture_t *r_default_roughness_texture;
struct r_material_t *r_default_material;

extern mat4_t r_projection_matrix;
extern mat4_t r_camera_matrix;
extern mat4_t r_view_matrix;
extern mat4_t r_view_projection_matrix;

uint32_t r_cluster_texture;
struct r_cluster_t *r_clusters;

uint32_t r_light_data_uniform_buffer;
uint32_t r_light_buffer_cursor;
struct r_l_data_t *r_light_buffer;

uint32_t r_light_index_buffer_cursor;
uint32_t r_light_index_uniform_buffer;
uint32_t *r_light_index_buffer;

//uint32_t r_shadow_map_uniform_buffer;

uint32_t *r_free_shadow_index_buffer;
uint32_t r_free_shadow_index_buffer_cursor;

uint32_t *r_shadow_index_buffer;
uint32_t r_shadow_index_buffer_cursor;
uint32_t r_shadow_index_uniform_buffer;
uint32_t r_shadow_atlas_texture;
uint32_t r_indirect_texture;
uint32_t r_shadow_map_framebuffer;
//mat4_t r_point_shadow_projection_matrices[6];
//mat4_t r_point_shadow_view_matrices[6];

vec2_t r_point_shadow_projection_params;
mat4_t r_point_shadow_view_projection_matrices[6];
vec3_t r_point_shadow_frustum_planes[6];
uint16_t r_point_shadow_frustum_masks[6];

uint32_t r_main_framebuffer;
uint32_t r_main_color_attachment;
uint32_t r_main_depth_attachment;
uint32_t r_z_prepass_framebuffer;

extern struct r_renderer_state_t r_renderer_state;
extern struct r_renderer_stats_t r_renderer_stats;

SDL_Window *r_window;
SDL_GLContext *r_context;
int32_t r_width = 1300;
int32_t r_height = 700;
//uint32_t r_cluster_row_width;
//uint32_t r_cluster_rows;
float r_z_near = 0.1;
float r_z_far = 1000.0;
float r_denom = 0.0;
float r_fov = 0.68;

struct r_named_uniform_t r_default_uniforms[] =
{
    [R_UNIFORM_MODEL_VIEW_PROJECTION_MATRIX] =  (struct r_named_uniform_t){.type = R_UNIFORM_TYPE_MAT4,  .name = "r_model_view_projection_matrix" },
    [R_UNIFORM_VIEW_PROJECTION_MATRIX] =        (struct r_named_uniform_t){.type = R_UNIFORM_TYPE_MAT4,  .name = "r_view_projection_matrix" },
    [R_UNIFORM_MODEL_VIEW_MATRIX] =             (struct r_named_uniform_t){.type = R_UNIFORM_TYPE_MAT4,  .name = "r_model_view_matrix" },
    [R_UNIFORM_VIEW_MATRIX] =                   (struct r_named_uniform_t){.type = R_UNIFORM_TYPE_MAT4,  .name = "r_view_matrix" },
    [R_UNIFORM_CAMERA_MATRIX] =                 (struct r_named_uniform_t){.type = R_UNIFORM_TYPE_MAT4,  .name = "r_camera_matrix" },
    [R_UNIFORM_MODEL_MATRIX] =                  (struct r_named_uniform_t){.type = R_UNIFORM_TYPE_MAT4,  .name = "r_model_matrix" },
    [R_UNIFORM_POINT_PROJ_PARAMS] =             (struct r_named_uniform_t){.type = R_UNIFORM_TYPE_VEC2,  .name = "r_point_proj_params" },
    [R_UNIFORM_TEX0] =                          (struct r_named_uniform_t){.type = R_UNIFORM_TYPE_INT,   .name = "r_tex0" },
    [R_UNIFORM_TEX1] =                          (struct r_named_uniform_t){.type = R_UNIFORM_TYPE_INT,   .name = "r_tex1" },
    [R_UNIFORM_TEX3] =                          (struct r_named_uniform_t){.type = R_UNIFORM_TYPE_INT,   .name = "r_tex3" },
    [R_UNIFORM_TEX2] =                          (struct r_named_uniform_t){.type = R_UNIFORM_TYPE_INT,   .name = "r_tex2" },
    [R_UNIFORM_TEX4] =                          (struct r_named_uniform_t){.type = R_UNIFORM_TYPE_INT,   .name = "r_tex4" },
    [R_UNIFORM_TEX_ALBEDO] =                    (struct r_named_uniform_t){.type = R_UNIFORM_TYPE_INT,   .name = "r_tex_albedo" },
    [R_UNIFORM_TEX_NORMAL] =                    (struct r_named_uniform_t){.type = R_UNIFORM_TYPE_INT,   .name = "r_tex_normal" },
    [R_UNIFORM_TEX_METALNESS] =                 (struct r_named_uniform_t){.type = R_UNIFORM_TYPE_INT,   .name = "r_tex_metalness" },
    [R_UNIFORM_TEX_ROUGHNESS] =                 (struct r_named_uniform_t){.type = R_UNIFORM_TYPE_INT,   .name = "r_tex_roughness" },
    [R_UNIFORM_TEX_HEIGHT] =                    (struct r_named_uniform_t){.type = R_UNIFORM_TYPE_INT,   .name = "r_tex_height" },
    [R_UNIFORM_TEX_CLUSTERS] =                  (struct r_named_uniform_t){.type = R_UNIFORM_TYPE_INT,   .name = "r_tex_clusters" },
    [R_UNIFORM_TEX_SHADOW_ATLAS] =              (struct r_named_uniform_t){.type = R_UNIFORM_TYPE_INT,   .name = "r_tex_shadow_atlas" },
    [R_UNIFORM_TEX_INDIRECT] =                  (struct r_named_uniform_t){.type = R_UNIFORM_TYPE_INT,   .name = "r_tex_indirect" },
    [R_UNIFORM_CLUSTER_DENOM] =                 (struct r_named_uniform_t){.type = R_UNIFORM_TYPE_FLOAT, .name = "r_cluster_denom" },
    [R_UNIFORM_Z_NEAR] =                        (struct r_named_uniform_t){.type = R_UNIFORM_TYPE_FLOAT, .name = "r_z_near" },
    [R_UNIFORM_Z_FAR] =                         (struct r_named_uniform_t){.type = R_UNIFORM_TYPE_FLOAT, .name = "r_z_far" },
    [R_UNIFORM_WIDTH] =                         (struct r_named_uniform_t){.type = R_UNIFORM_TYPE_INT,   .name = "r_width" },
    [R_UNIFORM_HEIGHT] =                        (struct r_named_uniform_t){.type = R_UNIFORM_TYPE_INT,   .name = "r_height" },
};

//char *r_default_uniform_names[] =
//{
//    [R_UNIFORM_MODEL_VIEW_PROJECTION_MATRIX] = "r_model_view_projection_matrix",
//    [R_UNIFORM_MODEL_VIEW_MATRIX] = "r_model_view_matrix",
//    [R_UNIFORM_VIEW_MATRIX] = "r_view_matrix",
//    [R_UNIFORM_CAMERA_MATRIX] = "r_camera_matrix",
//    [R_UNIFORM_POINT_PROJ_PARAMS] = "r_point_proj_params",
//    [R_UNIFORM_TEX0] = "r_tex0",
//    [R_UNIFORM_TEX1] = "r_tex1",
//    [R_UNIFORM_TEX2] = "r_tex2",
//    [R_UNIFORM_TEX3] = "r_tex3",
//    [R_UNIFORM_TEX4] = "r_tex4",
//    [R_UNIFORM_TEX_ALBEDO] = "r_tex_albedo",
//    [R_UNIFORM_TEX_NORMAL] = "r_tex_normal",
//    [R_UNIFORM_TEX_METALNESS] = "r_tex_metalness",
//    [R_UNIFORM_TEX_ROUGHNESS] = "r_tex_roughness",
//    [R_UNIFORM_TEX_HEIGHT] = "r_tex_height",
//    [R_UNIFORM_TEX_CLUSTERS] = "r_tex_clusters",
//    [R_UNIFORM_TEX_SHADOW_ATLAS] = "r_tex_shadow_atlas",
//    [R_UNIFORM_TEX_INDIRECT] = "r_tex_indirect",
//    [R_UNIFORM_CLUSTER_DENOM] = "r_cluster_denom",
//    [R_UNIFORM_Z_NEAR] = "r_z_near",
//    [R_UNIFORM_Z_FAR] = "r_z_far",
//    [R_UNIFORM_WIDTH] = "r_width",
//    [R_UNIFORM_HEIGHT] = "r_height"
//};
//
//uint32_t r_default_uniform_types[] =
//{
//    [R_UNIFORM_MODEL_VIEW_PROJECTION_MATRIX] = R_UNIFORM_TYPE_MAT4,
//    [R_UNIFORM_MODEL_VIEW_MATRIX] = R_UNIFORM_TYPE_MAT4,
//    [R_UNIFORM_VIEW_MATRIX] = R_UNIFORM_TYPE_MAT4,
//    [R_UNIFORM_CAMERA_MATRIX] = R_UNIFORM_TYPE_MAT4,
//    [R_UNIFORM_POINT_PROJ_PARAMS] = R_UNIFORM_TYPE_VEC2,
//    [R_UNIFORM_TEX0] = R_UNIFORM_TYPE_UINT,
//    [R_UNIFORM_TEX1] = R_UNIFORM_TYPE_UINT,
//    [R_UNIFORM_TEX2] = R_UNIFORM_TYPE_UINT,
//    [R_UNIFORM_TEX3] = R_UNIFORM_TYPE_UINT,
//    [R_UNIFORM_TEX4] = R_UNIFORM_TYPE_UINT,
//    [R_UNIFORM_TEX_ALBEDO] = R_UNIFORM_TYPE_UINT,
//    [R_UNIFORM_TEX_NORMAL] = R_UNIFORM_TYPE_UINT,
//    [R_UNIFORM_TEX_METALNESS] = R_UNIFORM_TYPE_UINT,
//    [R_UNIFORM_TEX_ROUGHNESS] = R_UNIFORM_TYPE_UINT,
//    [R_UNIFORM_TEX_HEIGHT] = R_UNIFORM_TYPE_UINT,
//    [R_UNIFORM_TEX_CLUSTERS] = R_UNIFORM_TYPE_UINT,
//    [R_UNIFORM_TEX_SHADOW_ATLAS] = R_UNIFORM_TYPE_UINT,
//    [R_UNIFORM_TEX_INDIRECT] = R_UNIFORM_TYPE_UINT,
//    [R_UNIFORM_CLUSTER_DENOM] = R_UNIFORM_TYPE_FLOAT,
//    [R_UNIFORM_Z_NEAR] = R_UNIFORM_TYPE_FLOAT,
//    [R_UNIFORM_Z_FAR] = R_UNIFORM_TYPE_FLOAT,
//    [R_UNIFORM_WIDTH] = R_UNIFORM_TYPE_INT,
//    [R_UNIFORM_HEIGHT] = R_UNIFORM_TYPE_INT
//};

uint32_t r_uniform_type_sizes[] =
{
    [R_UNIFORM_TYPE_UINT] = sizeof(uint32_t),
    [R_UNIFORM_TYPE_INT] = sizeof(int32_t),
    [R_UNIFORM_TYPE_FLOAT] = sizeof(float),
    [R_UNIFORM_TYPE_VEC2] = sizeof(vec2_t),
    [R_UNIFORM_TYPE_VEC3] = sizeof(vec3_t),
    [R_UNIFORM_TYPE_VEC4] = sizeof(vec4_t),
    [R_UNIFORM_TYPE_MAT2] = sizeof(mat2_t),
    [R_UNIFORM_TYPE_MAT3] = sizeof(mat3_t),
    [R_UNIFORM_TYPE_MAT4] = sizeof(mat4_t),
};

void r_Init()
{
    r_world_cmds = ds_list_create(sizeof(struct r_world_cmd_t), 8192);
    r_entity_cmds = ds_list_create(sizeof(struct r_entity_cmd_t), 8192);
    r_shadow_cmds = ds_list_create(sizeof(struct r_shadow_cmd_t), 8192);
    r_immediate_cmds = ds_list_create(sizeof(struct r_i_cmd_t), 4096);
    r_immediate_data = ds_list_create(R_IMMEDIATE_DATA_SLOT_SIZE, 32768);

    r_shaders = ds_slist_create(sizeof(struct r_shader_t), 16);
    r_materials = ds_slist_create(sizeof(struct r_material_t), 32);
    r_textures = ds_slist_create(sizeof(struct r_texture_t), 128);
    r_models = ds_slist_create(sizeof(struct r_model_t), 512);
    r_lights = ds_slist_create(sizeof(struct r_light_t), 512);
    r_visible_lights = ds_list_create(sizeof(struct r_light_t *), 512);
    r_vertex_heap = ds_create_heap(R_VERTEX_BUFFER_SIZE);
    r_index_heap = ds_create_heap(R_INDEX_BUFFER_SIZE);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    r_window = SDL_CreateWindow("doh", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, r_width, r_height, SDL_WINDOW_OPENGL);
    r_context = SDL_GL_CreateContext(r_window);
    SDL_GL_MakeCurrent(r_window, r_context);
    SDL_GL_SetSwapInterval(1);

    GLenum status = glewInit();
    if(status != GLEW_OK)
    {
        printf("oh, fuck...\n");
        printf("%s\n", glewGetErrorString(status));
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    glClearColor(0.01, 0.01, 0.01, 1.0);
    glClearDepth(1.0);
    glClearStencil(0x00);
//    glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);

    glGenBuffers(1, &r_vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, r_vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, R_VERTEX_BUFFER_SIZE, NULL, GL_DYNAMIC_DRAW);

//    glGenBuffers(1, &r_immediate_vertex_buffer);
//    glBindBuffer(GL_ARRAY_BUFFER, r_immediate_vertex_buffer);
//    glBufferData(GL_ARRAY_BUFFER, R_IMMEDIATE_VERTEX_BUFFER_SIZE, NULL, GL_DYNAMIC_DRAW);

    glGenBuffers(1, &r_index_buffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, r_index_buffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, R_INDEX_BUFFER_SIZE, NULL, GL_DYNAMIC_DRAW);

//    glGenBuffers(1, &r_immediate_index_buffer);
//    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, r_immediate_index_buffer);
//    glBufferData(GL_ELEMENT_ARRAY_BUFFER, R_IMMEDIATE_INDEX_BUFFER_SIZE, NULL, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    glGenVertexArrays(1, &r_vao);
    glBindVertexArray(r_vao);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    r_z_prepass_shader = r_LoadShader("shaders/r_prez.vert", "shaders/r_prez.frag");
    r_lit_shader = r_LoadShader("shaders/r_cfwd.vert", "shaders/r_cfwd.frag");
    r_shadow_shader = r_LoadShader("shaders/r_shdw.vert", "shaders/r_shdw.frag");
    r_immediate_shader = r_LoadShader("shaders/r_imm.vert", "shaders/r_imm.frag");
    mat4_t_persp(&r_projection_matrix, r_fov, (float)r_width / (float)r_height, r_z_near, r_z_far);
    mat4_t_identity(&r_view_matrix);

    uint32_t pixels[] =
    {
        0xff777777, 0xff444444, 0xff777777, 0xff444444,
        0xff444444, 0xff777777, 0xff444444, 0xff777777,
        0xff777777, 0xff444444, 0xff777777, 0xff444444,
        0xff444444, 0xff777777, 0xff444444, 0xff777777,
    };
    r_default_albedo_texture = r_CreateTexture("default_albedo", 4, 4, GL_RGBA8, pixels);
    pixels[0] = 0x00ff7f7f;
    r_default_normal_texture = r_CreateTexture("default_normal", 1, 1, GL_RGBA8, pixels);
    pixels[0] = 0x0000003f;
    r_default_roughness_texture = r_CreateTexture("default_roughness", 1, 1, GL_RGBA8, pixels);

    r_default_material = r_CreateMaterial("default", NULL, NULL, NULL);

    r_clusters = mem_Calloc(R_CLUSTER_COUNT, sizeof(struct r_cluster_t));
    r_light_buffer = mem_Calloc(R_MAX_LIGHTS, sizeof(struct r_l_data_t));
    r_light_index_buffer = mem_Calloc(R_CLUSTER_COUNT * R_MAX_CLUSTER_LIGHTS, sizeof(uint32_t));
    r_shadow_index_buffer = mem_Calloc(R_MAX_LIGHTS * 6, sizeof(uint32_t));
    r_free_shadow_index_buffer = mem_Calloc(R_MAX_SHADOW_MAPS, sizeof(uint32_t));

//    r_free_shadow_index_buffer[0] = ((R_SHADOW_MAP_ATLAS_WIDTH / R_SHADOW_MAP_MIN_RESOLUTION) << R_SHADOW_MAP_WIDTH_SHIFT);
//    r_free_shadow_index_buffer[0] |= ((R_SHADOW_MAP_ATLAS_HEIGHT / R_SHADOW_MAP_MIN_RESOLUTION) << R_SHADOW_MAP_HEIGHT_SHIFT);
//    r_free_shadow_index_buffer_cursor = 1;

    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    glGenTextures(1, &r_cluster_texture);
    glBindTexture(GL_TEXTURE_3D, r_cluster_texture);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAX_LEVEL, 0);
    glTexImage3D(GL_TEXTURE_3D, 0, GL_RG32UI, R_CLUSTER_ROW_WIDTH, R_CLUSTER_ROWS, R_CLUSTER_SLICES, 0, GL_RG_INTEGER, GL_UNSIGNED_INT, NULL);

    glGenBuffers(1, &r_light_data_uniform_buffer);
    glBindBuffer(GL_UNIFORM_BUFFER, r_light_data_uniform_buffer);
    glBufferData(GL_UNIFORM_BUFFER, R_MAX_LIGHTS * sizeof(struct r_l_data_t), NULL, GL_DYNAMIC_DRAW);
    glGenBuffers(1, &r_light_index_uniform_buffer);
    glBindBuffer(GL_UNIFORM_BUFFER, r_light_index_uniform_buffer);
    glBufferData(GL_UNIFORM_BUFFER, R_CLUSTER_COUNT * R_MAX_CLUSTER_LIGHTS * sizeof(uint32_t), NULL, GL_DYNAMIC_DRAW);
    glGenBuffers(1, &r_shadow_index_uniform_buffer);
    glBindBuffer(GL_UNIFORM_BUFFER, r_shadow_index_uniform_buffer);
    glBufferData(GL_UNIFORM_BUFFER, R_MAX_LIGHTS * sizeof(uint32_t) * 6, NULL, GL_DYNAMIC_DRAW);

    glGenTextures(1, &r_indirect_texture);
    glBindTexture(GL_TEXTURE_CUBE_MAP, r_indirect_texture);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, 0);

    uint32_t *indirect_pixels = mem_Calloc(1024 * 1024, sizeof(uint32_t));

    /* cubemap lookup used to simplify selecting which shadow map face to sample
    from the atlas, and which faces to sample from when filtering across shadow map faces.
    The texture format is GL_RGBA8, where each component packs the index of the face to
    sample from and an offset to apply to the computed uv coord in that face. The format is:

        bits 0-2: face index
        bit  3  : uv.x offset (either 0 or 1)
        bit  4  : uv.y offset (either 0 or 1)

    The four components are used to obtain four values, that form a 2x2 quad. The uv coord computed in
    during shading is for the bottom-left sample. The order of samples in the components is:

        R: bottom-left
        G: bottom-right
        B: top-left
        A: top-right.

    */

    uint8_t face_data[] =
    {
        /* +X */
        (0 << R_SHADOW_MAP_FACE_INDEX_SHIFT) | (2 << R_SHADOW_MAP_FACE_U_COORD_SHIFT) | (1 << R_SHADOW_MAP_FACE_V_COORD_SHIFT),
        /* -X */
        (1 << R_SHADOW_MAP_FACE_INDEX_SHIFT) | (1 << R_SHADOW_MAP_FACE_U_COORD_SHIFT) | (2 << R_SHADOW_MAP_FACE_V_COORD_SHIFT),
        /* +Y */
        (2 << R_SHADOW_MAP_FACE_INDEX_SHIFT) | (0 << R_SHADOW_MAP_FACE_U_COORD_SHIFT) | (2 << R_SHADOW_MAP_FACE_V_COORD_SHIFT),
        /* -Y */
        (3 << R_SHADOW_MAP_FACE_INDEX_SHIFT) | (2 << R_SHADOW_MAP_FACE_U_COORD_SHIFT) | (0 << R_SHADOW_MAP_FACE_V_COORD_SHIFT),
        /* +Z */
        (4 << R_SHADOW_MAP_FACE_INDEX_SHIFT) | (1 << R_SHADOW_MAP_FACE_U_COORD_SHIFT) | (0 << R_SHADOW_MAP_FACE_V_COORD_SHIFT),
        /* -Z */
        (5 << R_SHADOW_MAP_FACE_INDEX_SHIFT) | (0 << R_SHADOW_MAP_FACE_U_COORD_SHIFT) | (1 << R_SHADOW_MAP_FACE_V_COORD_SHIFT),
    };

    for(uint32_t face_index = 0; face_index < 6; face_index++)
    {
        for(uint32_t row = 0; row < 1024; row++)
        {
            for(uint32_t col = 0; col < 1024; col++)
            {
                uint32_t pixel_value = 0;

                pixel_value |= face_data[face_index];
                pixel_value |= face_data[face_index] << 8;
                pixel_value |= face_data[face_index] << 16;
                pixel_value |= face_data[face_index] << 24;

//                for(int32_t comp = 3; comp >= 0; comp--)
//                {
//                    pixel_value <<= 8;
//                    pixel_value |= face_index | (comp << R_SHADOW_MAP_OFFSET_PACK_SHIFT);
//                }

                indirect_pixels[row * 1024 + col] = pixel_value;
            }
        }

//        switch(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face_index)
//        {
//            case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
//            {
//                /* rightmost column, excluding top pixel, requires sampling between -Z and +X faces.
//                Bottom-left and top-left samples fall onto the -Z face, while the other two fall onto
//                the +X face. */
//                for(uint32_t row_index = 0; row_index < 1024; row_index++)
//                {
//                    /* remove bottom-right and top-right values */
//                    uint32_t pixel_value = indirect_pixels[row_index * 1024 + 1023] & 0x00ff00ff;
//
//                    uint32_t pixel_value = 0;
//                    pixel_value |= 0x5 | (0x5 << 8) | (0x5 << 16) | (0x5 << 24);
//
//                    /* bottom-right sample will sample from the +X face, and will have no
//                    offset. That ends up being a 0, so nothing to do here */
//
//                    /* top-right sample will also sample from the +X face. and will have a uv.y
//                    offset of 1 and a uv.x offset of 0. */
//                    pixel_value |= (0x2 << R_SHADOW_MAP_OFFSET_PACK_SHIFT) << 8;
//                    pixel_value |= (0x2 << R_SHADOW_MAP_OFFSET_PACK_SHIFT) << 24;
//                    indirect_pixels[row_index * 1024 + 1023] = pixel_value;
//                }
//            }
//            break;
//        }

        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face_index, 0, GL_RGBA8UI, 1024, 1024, 0, GL_RGBA_INTEGER, GL_UNSIGNED_BYTE, indirect_pixels);
    }

    mem_Free(indirect_pixels);

    glGenTextures(1, &r_shadow_atlas_texture);
    glBindTexture(GL_TEXTURE_2D, r_shadow_atlas_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, R_SHADOW_MAP_ATLAS_WIDTH, R_SHADOW_MAP_ATLAS_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenFramebuffers(1, &r_shadow_map_framebuffer);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, r_shadow_map_framebuffer);
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, r_shadow_atlas_texture, 0);
    glClear(GL_DEPTH_BUFFER_BIT);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

//    mat4_t reverse_z_proj;
//    mat4_t_identity(&reverse_z_proj);
//
//    reverse_z_proj.comps[2][2] = -1.0;
//    reverse_z_proj.comps[3][2] = 1.0;

    mat4_t point_shadow_projection_matrices[6];
    mat4_t point_shadow_view_matrices[6];

    for(uint32_t face_index = 0; face_index < 6; face_index++)
    {
        mat4_t_identity(&point_shadow_view_matrices[face_index]);
        mat4_t_persp(&point_shadow_projection_matrices[face_index], 3.14159265 * 0.25, 1.0, 0.01, 10000.0);
    }

    r_point_shadow_projection_params.x = point_shadow_projection_matrices[0].rows[2].comps[2];
    r_point_shadow_projection_params.y = point_shadow_projection_matrices[0].rows[3].comps[2];


    mat4_t_rotate_y(&point_shadow_view_matrices[R_SHADOW_MAP_FACE_POS_X], 0.5);
    mat4_t_rotate_z(&point_shadow_view_matrices[R_SHADOW_MAP_FACE_POS_X], 1.0);

    mat4_t_rotate_y(&point_shadow_view_matrices[R_SHADOW_MAP_FACE_NEG_X],-0.5);
    mat4_t_rotate_z(&point_shadow_view_matrices[R_SHADOW_MAP_FACE_NEG_X], -0.5);

    mat4_t_rotate_x(&point_shadow_view_matrices[R_SHADOW_MAP_FACE_POS_Y], -0.5);
    mat4_t_rotate_z(&point_shadow_view_matrices[R_SHADOW_MAP_FACE_POS_Y], 1.0);

    mat4_t_rotate_x(&point_shadow_view_matrices[R_SHADOW_MAP_FACE_NEG_Y], 0.5);
    mat4_t_rotate_z(&point_shadow_view_matrices[R_SHADOW_MAP_FACE_NEG_Y], 0.5);

    mat4_t_rotate_y(&point_shadow_view_matrices[R_SHADOW_MAP_FACE_POS_Z], 1.0);
    mat4_t_rotate_z(&point_shadow_view_matrices[R_SHADOW_MAP_FACE_POS_Z], 0.5);

//    mat4_t_rotate_z(&point_shadow_view_matrices[R_SHADOW_MAP_FACE_NEG_Z], 1.0);

//    point_shadow_projection_matrices[R_SHADOW_MAP_FACE_POS_Z].comps[0][0] *= -1.0;
//    point_shadow_projection_matrices[R_SHADOW_MAP_FACE_POS_X].comps[0][0] *= -1.0;
//    point_shadow_projection_matrices[R_SHADOW_MAP_FACE_POS_Y].comps[0][0] *= -1.0;

    for(uint32_t face_index = 0; face_index < 6; face_index++)
    {
        mat4_t_mul(&r_point_shadow_view_projection_matrices[face_index],
                   &point_shadow_view_matrices[face_index],
                   &point_shadow_projection_matrices[face_index]);

        r_point_shadow_frustum_planes[face_index] = vec3_t_c(0.0, 0.0, 1.0);
    }

    /* <0.707106769, 0.0, 0.707106769> */
    vec3_t_rotate_y(&r_point_shadow_frustum_planes[0], &r_point_shadow_frustum_planes[0], 0.25);

    /* <-0.707106769, 0.0, 0.707106769> */
    vec3_t_rotate_y(&r_point_shadow_frustum_planes[1], &r_point_shadow_frustum_planes[1], -0.25);

    /* <0.0, -0.707106769, 0.707106769> */
    vec3_t_rotate_x(&r_point_shadow_frustum_planes[2], &r_point_shadow_frustum_planes[2], 0.25);

    /* <0.0, 0.707106769, 0.707106769> */
    vec3_t_rotate_x(&r_point_shadow_frustum_planes[3], &r_point_shadow_frustum_planes[3], -0.25);

    /* <0.707106769, -0.707106769, 0.0> */
    vec3_t_rotate_x(&r_point_shadow_frustum_planes[4], &r_point_shadow_frustum_planes[4], 0.25);
    vec3_t_rotate_y(&r_point_shadow_frustum_planes[4], &r_point_shadow_frustum_planes[4], 0.5);

    /* <0.707106769, 0.707106769, 0.0> */
    vec3_t_rotate_x(&r_point_shadow_frustum_planes[5], &r_point_shadow_frustum_planes[5], -0.25);
    vec3_t_rotate_y(&r_point_shadow_frustum_planes[5], &r_point_shadow_frustum_planes[5], 0.5);

    /* +X */
    r_point_shadow_frustum_masks[0] = (R_SHADOW_MAP_PLANE_FRONT << R_SHADOW_MAP_PLANE0_SHIFT) |
                                      (R_SHADOW_MAP_PLANE_BACK << R_SHADOW_MAP_PLANE1_SHIFT) |
                                      (R_SHADOW_MAP_PLANE_FRONT << R_SHADOW_MAP_PLANE4_SHIFT) |
                                      (R_SHADOW_MAP_PLANE_FRONT << R_SHADOW_MAP_PLANE5_SHIFT);

    /* -X */
    r_point_shadow_frustum_masks[1] = (R_SHADOW_MAP_PLANE_BACK << R_SHADOW_MAP_PLANE0_SHIFT) |
                                      (R_SHADOW_MAP_PLANE_FRONT << R_SHADOW_MAP_PLANE1_SHIFT) |
                                      (R_SHADOW_MAP_PLANE_BACK << R_SHADOW_MAP_PLANE4_SHIFT) |
                                      (R_SHADOW_MAP_PLANE_BACK << R_SHADOW_MAP_PLANE5_SHIFT);

    /* +Y */
    r_point_shadow_frustum_masks[2] = (R_SHADOW_MAP_PLANE_BACK << R_SHADOW_MAP_PLANE2_SHIFT) |
                                      (R_SHADOW_MAP_PLANE_FRONT << R_SHADOW_MAP_PLANE3_SHIFT) |
                                      (R_SHADOW_MAP_PLANE_BACK << R_SHADOW_MAP_PLANE4_SHIFT) |
                                      (R_SHADOW_MAP_PLANE_FRONT << R_SHADOW_MAP_PLANE5_SHIFT);

    /* -Y */
    r_point_shadow_frustum_masks[3] = (R_SHADOW_MAP_PLANE_FRONT << R_SHADOW_MAP_PLANE2_SHIFT) |
                                      (R_SHADOW_MAP_PLANE_BACK << R_SHADOW_MAP_PLANE3_SHIFT) |
                                      (R_SHADOW_MAP_PLANE_FRONT << R_SHADOW_MAP_PLANE4_SHIFT) |
                                      (R_SHADOW_MAP_PLANE_BACK << R_SHADOW_MAP_PLANE5_SHIFT);

    /* +Z */
    r_point_shadow_frustum_masks[4] = (R_SHADOW_MAP_PLANE_FRONT << R_SHADOW_MAP_PLANE0_SHIFT) |
                                      (R_SHADOW_MAP_PLANE_FRONT << R_SHADOW_MAP_PLANE1_SHIFT) |
                                      (R_SHADOW_MAP_PLANE_FRONT << R_SHADOW_MAP_PLANE2_SHIFT) |
                                      (R_SHADOW_MAP_PLANE_FRONT << R_SHADOW_MAP_PLANE3_SHIFT);

    /* -Z */
    r_point_shadow_frustum_masks[5] = (R_SHADOW_MAP_PLANE_BACK << R_SHADOW_MAP_PLANE0_SHIFT) |
                                      (R_SHADOW_MAP_PLANE_BACK << R_SHADOW_MAP_PLANE1_SHIFT) |
                                      (R_SHADOW_MAP_PLANE_BACK << R_SHADOW_MAP_PLANE2_SHIFT) |
                                      (R_SHADOW_MAP_PLANE_BACK << R_SHADOW_MAP_PLANE3_SHIFT);

    glGenTextures(1, &r_main_color_attachment);
    glBindTexture(GL_TEXTURE_2D, r_main_color_attachment);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, r_width, r_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

    glGenTextures(1, &r_main_depth_attachment);
    glBindTexture(GL_TEXTURE_2D, r_main_depth_attachment);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, r_width, r_height, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenFramebuffers(1, &r_main_framebuffer);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, r_main_framebuffer);
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, r_main_color_attachment, 0);
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, r_main_depth_attachment, 0);
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, r_main_depth_attachment, 0);

    glGenFramebuffers(1, &r_z_prepass_framebuffer);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, r_z_prepass_framebuffer);
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, r_main_depth_attachment, 0);

    r_renderer_state.use_z_prepass = 1;
    r_renderer_state.max_shadow_res = 8;
}

void r_Shutdown()
{

}

/*
============================================================================
============================================================================
============================================================================
*/

struct r_vis_item_t *r_AllocateVisItem(mat4_t *transform, struct r_model_t *model)
{
    uint32_t index;
    struct r_vis_item_t *item;

    index = ds_slist_add_element(&r_vis_items, NULL);
    item = ds_slist_get_element(&r_vis_items, index);

    item->index = index;
    item->model = model;
    item->transform = transform;

    return item;
}

void r_FreeVisItem(struct r_vis_item_t *item)
{

}

/*
============================================================================
============================================================================
============================================================================
*/

struct r_texture_t *r_LoadTexture(char *file_name, char *name)
{
    struct r_texture_t *texture = NULL;
    if(file_exists(file_name))
    {
        int32_t width;
        int32_t height;
        int32_t channels;
        unsigned char *pixels = stbi_load(file_name, &width, &height, &channels, STBI_rgb_alpha);
        texture = r_CreateTexture(name, width, height, GL_RGBA8, pixels);
        mem_Free(pixels);
    }

    return texture;
}

/*
============================================================================
============================================================================
============================================================================
*/

struct r_texture_t *r_CreateTexture(char *name, uint32_t width, uint32_t height, uint32_t internal_format, void *data)
{
    uint32_t texture_index;
    struct r_texture_t *texture;
    uint32_t format;
    uint32_t type;

    switch(internal_format)
    {
        case GL_RGBA8:
            type = GL_UNSIGNED_BYTE;
            format = GL_RGBA;
        break;

        case GL_RGBA16:
            type = GL_UNSIGNED_SHORT;
            format = GL_RGBA;
        break;

        case GL_RGBA16F:
            type = GL_FLOAT;
            format = GL_RGBA;
        break;

        case GL_RGBA32F:
            type = GL_FLOAT;
            format = GL_RGBA;
        break;

        case GL_RGB8:
            type = GL_UNSIGNED_BYTE;
            format = GL_RGB;
        break;

        case GL_RED:
            type = GL_UNSIGNED_BYTE;
            format = GL_RED;
        break;
    }

    texture_index = ds_slist_add_element(&r_textures, NULL);
    texture = ds_slist_get_element(&r_textures, texture_index);
    texture->index = texture_index;
    texture->name = strdup(name);

    glGenTextures(1, &texture->handle);
    glBindTexture(GL_TEXTURE_2D, texture->handle);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 4);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, 4.0);
    glTexImage2D(GL_TEXTURE_2D, 0, internal_format, width, height, 0, format, type, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);

    return texture;
}

struct r_texture_t *r_GetTexture(char *name)
{
    for(uint32_t texture_index = 0; texture_index < r_textures.cursor; texture_index++)
    {
        struct r_texture_t *texture = ds_slist_get_element(&r_textures, texture_index);
        if(texture->index != 0xffffffff)
        {
            if(!strcmp(texture->name, name))
            {
                return texture;
            }
        }
    }

    return NULL;
}

/*
============================================================================
============================================================================
============================================================================
*/

struct r_material_t *r_CreateMaterial(char *name, struct r_texture_t *diffuse_texture, struct r_texture_t *normal_texture, struct r_texture_t *roughness_texture)
{
    uint32_t material_index;
    struct r_material_t *material;

    material_index = ds_slist_add_element(&r_materials, NULL);
    material = ds_slist_get_element(&r_materials, material_index);

    if(!diffuse_texture)
    {
        diffuse_texture = r_default_albedo_texture;
    }

    if(!normal_texture)
    {
        normal_texture = r_default_normal_texture;
    }

    if(!roughness_texture)
    {
        roughness_texture = r_default_roughness_texture;
    }

    material->index = material_index;
    material->name = strdup(name);
    material->diffuse_texture = diffuse_texture;
    material->normal_texture = normal_texture;
    material->roughness_texture = roughness_texture;

    return material;
}

struct r_material_t *r_GetMaterial(char *name)
{
    for(uint32_t material_index = 0; material_index < r_materials.cursor; material_index++)
    {
        struct r_material_t *material = ds_slist_get_element(&r_materials, material_index);
        if(material->index != 0xffffffff)
        {
            if(!strcmp(material->name, name))
            {
                return material;
            }
        }
    }

    return NULL;
}

struct r_material_t *r_GetDefaultMaterial()
{
    return r_default_material;
}

void r_BindMaterial(struct r_material_t *material)
{
    if(material)
    {
        r_renderer_stats.material_swaps++;

        glActiveTexture(GL_TEXTURE0 + R_ALBEDO_TEX_UNIT);
        glBindTexture(GL_TEXTURE_2D, material->diffuse_texture->handle);
        r_SetDefaultUniformI(R_UNIFORM_TEX_ALBEDO, R_ALBEDO_TEX_UNIT);

        glActiveTexture(GL_TEXTURE0 + R_ROUGHNESS_TEX_UNIT);
        glBindTexture(GL_TEXTURE_2D, material->roughness_texture->handle);
        r_SetDefaultUniformI(R_UNIFORM_TEX_ROUGHNESS, R_ROUGHNESS_TEX_UNIT);

        glActiveTexture(GL_TEXTURE0 + R_NORMAL_TEX_UNIT);
        glBindTexture(GL_TEXTURE_2D, material->normal_texture->handle);
        r_SetDefaultUniformI(R_UNIFORM_TEX_NORMAL, R_NORMAL_TEX_UNIT);
    }
}

/*
============================================================================
============================================================================
============================================================================
*/

struct ds_chunk_h r_AllocateVertices(uint32_t count)
{
    return ds_alloc_chunk(&r_vertex_heap, sizeof(struct r_vert_t) * count, sizeof(struct r_vert_t));
}

void r_FreeVertices(struct ds_chunk_h chunk)
{
    ds_free_chunk(&r_vertex_heap, chunk);
}

void r_FillVertices(struct ds_chunk_h chunk, struct r_vert_t *vertices, uint32_t count)
{
    struct ds_chunk_t *chunk_ptr = ds_get_chunk_pointer(&r_vertex_heap, chunk);
    glBindBuffer(GL_ARRAY_BUFFER, r_vertex_buffer);
    glBufferSubData(GL_ARRAY_BUFFER, chunk_ptr->start, sizeof(struct r_vert_t) * count, vertices);
}

struct ds_chunk_t *r_GetVerticesChunk(struct ds_chunk_h chunk)
{
    return ds_get_chunk_pointer(&r_vertex_heap, chunk);
}

struct ds_chunk_h r_AllocateIndices(uint32_t count)
{
    return ds_alloc_chunk(&r_index_heap, sizeof(uint32_t) * count, sizeof(uint32_t));
}

void r_FreeIndices(struct ds_chunk_h chunk)
{
    ds_free_chunk(&r_index_heap, chunk);
}

void r_FillIndices(struct ds_chunk_h chunk, uint32_t *indices, uint32_t count, uint32_t offset)
{
    struct ds_chunk_t *chunk_ptr = ds_get_chunk_pointer(&r_index_heap, chunk);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, r_index_buffer);
    uint32_t start = chunk_ptr->start / sizeof(uint32_t);
    uint32_t *index_buffer = (uint32_t *)glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_READ_WRITE) + start;

    for(uint32_t index = 0; index < count; index++)
    {
        index_buffer[index] = indices[index] + offset;
    }

    glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
}

struct ds_chunk_t *r_GetIndicesChunk(struct ds_chunk_h chunk)
{
    return ds_get_chunk_pointer(&r_index_heap, chunk);
}

struct r_model_t *r_LoadModel(char *file_name)
{
    struct r_model_t *model = NULL;
    if(file_exists(file_name))
    {
        void *file_buffer;
        FILE *file = fopen(file_name, "rb");
        read_file(file, &file_buffer, NULL);
        fclose(file);

        struct a_skeleton_section_t(0, 0) *skeleton_header;
        struct a_weight_section_t(0, 0) *weight_header;
        struct r_vert_section_t *verts;
        struct r_index_section_t *indexes;
        struct r_batch_section_t *batch_section;
        struct r_material_section_t *materials;

        ds_get_section_data(file_buffer, "[skeleton]", (void **)&skeleton_header, NULL);
        ds_get_section_data(file_buffer, "[weights]", (void **)&weight_header, NULL);
        ds_get_section_data(file_buffer, "[vertices]", (void **)&verts, NULL);
        ds_get_section_data(file_buffer, "[indices]", (void **)&indexes, NULL);
        ds_get_section_data(file_buffer, "[batches]", (void **)&batch_section, NULL);
        ds_get_section_data(file_buffer, "[materials]", (void **)&materials, NULL);

        struct r_batch_t *batches = mem_Calloc(batch_section->batch_count, sizeof(struct r_batch_t));

        for(uint32_t batch_index = 0; batch_index < batch_section->batch_count; batch_index++)
        {
            struct r_batch_record_t *batch_record = batch_section->batches + batch_index;
            struct r_material_t *material = r_GetMaterial(batch_record->material);

            if(!material)
            {
                struct r_material_record_t *material_record;
                for(uint32_t material_index = 0; material_index < materials->material_count; material_index++)
                {
                    material_record = materials->materials + material_index;
                    if(!strcmp(material_record->name, batch_record->material))
                    {
                        break;
                    }
                }

                struct r_texture_t *diffuse_texture = r_default_albedo_texture;
                struct r_texture_t *normal_texture = r_default_normal_texture;

                if(material_record->diffuse_texture[0])
                {
                    diffuse_texture = r_GetTexture(material_record->diffuse_texture);

                    if(!diffuse_texture)
                    {
                        diffuse_texture = r_LoadTexture(material_record->diffuse_texture, material_record->diffuse_texture);
                    }
                }

                if(material_record->normal_texture[0])
                {
                    normal_texture = r_GetTexture(material_record->normal_texture);

                    if(!normal_texture)
                    {
                        normal_texture = r_LoadTexture(material_record->normal_texture, material_record->normal_texture);
                    }
                }

                material = r_CreateMaterial(batch_record->material, diffuse_texture, normal_texture, NULL);
            }

            batches[batch_index].start = batch_record->start;
            batches[batch_index].count = batch_record->count;
            batches[batch_index].material = material;
        }

        struct a_skeleton_t *skeleton = NULL;
        struct a_weight_t *weights = NULL;
        struct a_weight_range_t *weight_ranges = NULL;
        uint32_t weight_count = 0;
        uint32_t weight_range_count = 0;

        if(skeleton_header && skeleton_header->bone_count)
        {
            struct a_skeleton_section_t(skeleton_header->bone_count, skeleton_header->bone_names_length) *skeleton_data = (void *)skeleton_header;
            skeleton = a_CreateSkeleton(skeleton_data->bone_count, skeleton_data->bones, skeleton_data->bone_names_length, skeleton_data->bone_names);

            struct a_weight_section_t(weight_header->weight_count, weight_header->range_count) *weight_data = (void *)weight_header;
            weight_count = weight_header->weight_count;
            weight_range_count = weight_header->range_count;
            weights = weight_data->weights;
            weight_ranges = weight_data->ranges;
        }

        struct r_model_geometry_t geometry_data = {};
        struct r_model_skeleton_t skeleton_data = {};

        geometry_data.indices = indexes->indexes;
        geometry_data.index_count = indexes->index_count;
        geometry_data.verts = verts->verts;
        geometry_data.vert_count = verts->vert_count;
        geometry_data.batches = batches;
        geometry_data.batch_count = batch_section->batch_count;
        geometry_data.min = verts->min;
        geometry_data.max = verts->max;

        if(skeleton)
        {
            skeleton_data.skeleton = skeleton;
            skeleton_data.weight_count = weight_count;
            skeleton_data.weights = weights;
            skeleton_data.weight_range_count = weight_range_count;
            skeleton_data.weight_ranges = weight_ranges;

            model = r_CreateModel(&geometry_data, &skeleton_data);
        }
        else
        {
            model = r_CreateModel(&geometry_data, NULL);
        }

        mem_Free(batches);
        mem_Free(file_buffer);
    }

    return model;
}

struct r_model_t *r_CreateModel(struct r_model_geometry_t *geometry, struct r_model_skeleton_t *skeleton)
{
    struct r_model_t *model;
    uint32_t index;

    index = ds_slist_add_element(&r_models, NULL);
    model = ds_slist_get_element(&r_models, index);
    model->index = index;
    model->base = NULL;
    model->verts = ds_buffer_create(sizeof(struct r_vert_t), 0);
    model->indices = ds_buffer_create(sizeof(uint32_t), 0);
    model->batches = ds_buffer_create(sizeof(struct r_batch_t), 0);
    model->index_chunk = DS_INVALID_CHUNK_HANDLE;
    model->vert_chunk = DS_INVALID_CHUNK_HANDLE;

    r_UpdateModelGeometry(model, geometry);

    if(skeleton)
    {
        model->skeleton = skeleton->skeleton;

        if(skeleton->weight_count && skeleton->weight_range_count)
        {
            model->weights = ds_buffer_create(sizeof(struct a_weight_t), skeleton->weight_count);
            memcpy(model->weights.buffer, skeleton->weights, sizeof(struct a_weight_t) * skeleton->weight_count);

            model->weight_ranges = ds_buffer_create(sizeof(struct a_weight_range_t), skeleton->weight_range_count);
            memcpy(model->weight_ranges.buffer, skeleton->weight_ranges, sizeof(struct a_weight_range_t) * skeleton->weight_range_count);
        }
    }

    model->min = geometry->min;
    model->max = geometry->max;

    return model;
}

void r_UpdateModelGeometry(struct r_model_t *model, struct r_model_geometry_t *geometry)
{
    if(geometry->vert_count > model->verts.buffer_size)
    {
        if(model->vert_chunk.index != DS_INVALID_CHUNK_INDEX)
        {
            r_FreeVertices(model->vert_chunk);
        }

        model->vert_chunk = r_AllocateVertices(geometry->vert_count);
    }

    r_FillVertices(model->vert_chunk, geometry->verts, geometry->vert_count);

    struct ds_chunk_t *chunk = ds_get_chunk_pointer(&r_vertex_heap, model->vert_chunk);
    uint32_t start = chunk->start / sizeof(struct r_vert_t);

    if(geometry->index_count > model->indices.buffer_size)
    {
        if(model->index_chunk.index != DS_INVALID_CHUNK_INDEX)
        {
            r_FreeIndices(model->index_chunk);
        }

        model->index_chunk = r_AllocateIndices(geometry->index_count);
    }

    r_FillIndices(model->index_chunk, geometry->indices, geometry->index_count, start);

    chunk = ds_get_chunk_pointer(&r_index_heap, model->index_chunk);
    start = chunk->start / sizeof(uint32_t);

    ds_buffer_fill(&model->verts, 0, geometry->verts, geometry->vert_count);
    ds_buffer_fill(&model->indices, 0, geometry->indices, geometry->index_count);
    ds_buffer_fill(&model->batches, 0, geometry->batches, geometry->batch_count);

    for(uint32_t batch_index = 0; batch_index < geometry->batch_count; batch_index++)
    {
        ((struct r_batch_t *)model->batches.buffer)[batch_index].start += start;
    }
}

struct r_model_t *r_ShallowCopyModel(struct r_model_t *base)
{
    struct r_model_t *copy;
    uint32_t index = ds_slist_add_element(&r_models, NULL);
    struct ds_chunk_t *chunk;
    copy = ds_slist_get_element(&r_models, index);
    memcpy(copy, base, sizeof(struct r_model_t));
    copy->index = index;

    copy->verts = ds_buffer_create(sizeof(struct r_vert_t), base->verts.buffer_size);
    memcpy(copy->verts.buffer, base->verts.buffer, sizeof(struct r_vert_t) * copy->verts.buffer_size);

    copy->vert_chunk = r_AllocateVertices(copy->verts.buffer_size);
    r_FillVertices(copy->vert_chunk, copy->verts.buffer, copy->verts.buffer_size);
    chunk = ds_get_chunk_pointer(&r_vertex_heap, copy->vert_chunk);
    uint32_t new_start = chunk->start / sizeof(struct r_vert_t);

    copy->index_chunk = r_AllocateIndices(copy->indices.buffer_size);
    r_FillIndices(copy->index_chunk, copy->indices.buffer, copy->indices.buffer_size, new_start);
    chunk = ds_get_chunk_pointer(&r_index_heap, copy->index_chunk);
    new_start = chunk->start / sizeof(uint32_t);
    chunk = ds_get_chunk_pointer(&r_index_heap, base->index_chunk);

    copy->batches = ds_buffer_create(sizeof(struct r_batch_t), base->batches.buffer_size);
    for(uint32_t batch_index = 0; batch_index < copy->batches.buffer_size; batch_index++)
    {
        ((struct r_batch_t *)copy->batches.buffer)[batch_index] = ((struct r_batch_t *)base->batches.buffer)[batch_index];
        ((struct r_batch_t *)copy->batches.buffer)[batch_index].start += new_start;
    }

    copy->base = base;

    return copy;
}

void r_DestroyModel(struct r_model_t *model)
{
    if(model)
    {
        r_FreeIndices(model->index_chunk);
        r_FreeVertices(model->vert_chunk);

        ds_buffer_destroy(&model->verts);
        ds_buffer_destroy(&model->indices);
        ds_buffer_destroy(&model->batches);

        if(model->base)
        {
            r_DestroyModel(model->base);
        }
        else if(model->skeleton)
        {
            ds_buffer_destroy(&model->weights);
            ds_buffer_destroy(&model->weight_ranges);
            a_DestroySkeleton(model->skeleton);
        }
    }
}

/*
============================================================================
============================================================================
============================================================================
*/

struct r_light_t *r_CreateLight(uint32_t type, vec3_t *position, vec3_t *color, float radius, float energy)
{
    struct r_light_t *light;
    uint32_t index;

    index = ds_slist_add_element(&r_lights, NULL);
    light = ds_slist_get_element(&r_lights, index);

    light->index = index;
    light->data.pos_rad = vec4_t_c(position->x, position->y, position->z, radius);
    light->data.color_res = vec4_t_c(color->x, color->y, color->z, (float)type);
//    light->data.color = *color;
//    light->data.type = type;
    light->energy = energy;

    return light;
}

void r_AllocShadowMaps(struct r_light_t *light, uint32_t resolution)
{
    for(uint32_t face_index = 0; face_index < 6; face_index++)
    {

    }
}

struct r_light_t *r_GetLight(uint32_t light_index)
{
    struct r_light_t *light;
    light = ds_slist_get_element(&r_lights, light_index);
    if(light && light->index == 0xffffffff)
    {
        light = NULL;
    }

    return light;
}

void r_DestroyLight(struct r_light_t *light)
{
    if(light && light->index != 0xffffffff)
    {
        ds_slist_remove_element(&r_lights, light->index);
        light->index = 0xffffffff;
    }
}

/*
============================================================================
============================================================================
============================================================================
*/

struct r_shader_t *r_LoadShader(char *vertex_file_name, char *fragment_file_name)
{
    FILE *shader_file;
    uint32_t vertex_shader;
    uint32_t fragment_shader;
    uint32_t shader_program;
    char *shader_source;
    struct r_shader_t *shader;
    int32_t compilation_status;
    int32_t info_log_length;
    char *info_log;
    char include_error[256];

    if(!file_exists(vertex_file_name))
    {
        printf("couldn't load vertex shader %s\n", vertex_file_name);
        return NULL;
    }

    if(!file_exists(fragment_file_name))
    {
        printf("couldn't load fragment shader %s\n", fragment_file_name);
        return NULL;
    }


    shader_file = fopen(vertex_file_name, "rb");
    read_file(shader_file, (void **)&shader_source, NULL);
    fclose(shader_file);

    char *preprocessed_shader_source = stb_include_string(shader_source, NULL, "shaders", NULL, include_error);

    if(!preprocessed_shader_source)
    {
        printf("preprocessor error: %s\n", include_error);
        mem_Free(shader_source);
        return NULL;
    }

    vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, (const GLchar * const *)&preprocessed_shader_source, NULL);
    free(preprocessed_shader_source);
    mem_Free(shader_source);
    glCompileShader(vertex_shader);
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &compilation_status);

    if(!compilation_status)
    {
        glGetShaderiv(vertex_shader, GL_INFO_LOG_LENGTH, &info_log_length);
        info_log = mem_Calloc(1, info_log_length);
        glGetShaderInfoLog(vertex_shader, info_log_length, NULL, info_log);
        printf("vertex shader compilation for shader %s failed!\n", vertex_file_name);
        printf("info log:\n %s\n", info_log);
        mem_Free(info_log);
        glDeleteShader(vertex_shader);
        return NULL;
    }

    shader_file = fopen(fragment_file_name, "rb");
    read_file(shader_file, (void **)&shader_source, NULL);
    fclose(shader_file);
    preprocessed_shader_source = stb_include_string(shader_source, NULL, "shaders", NULL, include_error);

    if(!preprocessed_shader_source)
    {
        printf("preprocessor error: %s\n", include_error);
        glDeleteShader(vertex_shader);
        mem_Free(shader_source);
    }


    fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, (const GLchar * const *)&preprocessed_shader_source, NULL);
    free(preprocessed_shader_source);
    mem_Free(shader_source);
    glCompileShader(fragment_shader);
    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &compilation_status);
    if(!compilation_status)
    {
        glGetShaderiv(fragment_shader, GL_INFO_LOG_LENGTH, &info_log_length);
        info_log = mem_Calloc(1, info_log_length);
        glGetShaderInfoLog(fragment_shader, info_log_length, NULL, info_log);
        printf("fragment shader compilation for shader %s failed!\n", fragment_file_name);
        printf("info log:\n %s\n", info_log);
        mem_Free(info_log);
        glDeleteShader(vertex_shader);
        glDeleteShader(fragment_shader);
        return NULL;
    }


    shader_program = glCreateProgram();
    glAttachShader(shader_program, vertex_shader);
    glAttachShader(shader_program, fragment_shader);
    glLinkProgram(shader_program);
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    glGetProgramiv(shader_program, GL_LINK_STATUS, &compilation_status);
    if(!compilation_status)
    {
        glGetProgramiv(shader_program, GL_INFO_LOG_LENGTH, &info_log_length);
        info_log = mem_Calloc(1, info_log_length);
        glGetProgramInfoLog(shader_program, info_log_length, NULL, info_log);
        printf("program linking failed for shaders %s and %s!\n", vertex_file_name, fragment_file_name);
        printf("info log:\n %s\n", info_log);
        mem_Free(info_log);
        glDeleteProgram(shader_program);
        return NULL;
    }

    uint32_t shader_index = ds_slist_add_element(&r_shaders, NULL);
    shader = ds_slist_get_element(&r_shaders, shader_index);
    shader->index = shader_index;
    shader->handle = shader_program;

    for(uint32_t uniform_index = 0; uniform_index < R_UNIFORM_LAST; uniform_index++)
    {
        shader->uniforms[uniform_index].location = glGetUniformLocation(shader_program, r_default_uniforms[uniform_index].name);
    }

    uint32_t lights_uniform_block = glGetUniformBlockIndex(shader_program, "r_lights");
    uint32_t light_indices_uniform_block = glGetUniformBlockIndex(shader_program, "r_light_indices");
    uint32_t shadow_indices_uniform_block = glGetUniformBlockIndex(shader_program, "r_shadow_indices");

    if(lights_uniform_block != 0xffffffff)
    {
        glUniformBlockBinding(shader_program, lights_uniform_block, R_LIGHTS_UNIFORM_BUFFER_BINDING);
    }

    if(light_indices_uniform_block != 0xffffffff)
    {
        glUniformBlockBinding(shader_program, light_indices_uniform_block, R_LIGHT_INDICES_UNIFORM_BUFFER_BINDING);
    }

    if(shadow_indices_uniform_block != 0xffffffff)
    {
        glUniformBlockBinding(shader_program, shadow_indices_uniform_block, R_SHADOW_INDICES_BUFFER_BINDING);
    }

    shader->attribs = 0;

    if(glGetAttribLocation(shader_program, "r_position") != -1)
    {
        shader->attribs |= R_ATTRIB_POSITION;
        glBindAttribLocation(shader_program, R_POSITION_LOCATION, "r_position");
    }

    if(glGetAttribLocation(shader_program, "r_tex_coords") != -1)
    {
        shader->attribs |= R_ATTRIB_TEX_COORDS;
        glBindAttribLocation(shader_program, R_TEX_COORDS_LOCATION, "r_tex_coords");
    }

    if(glGetAttribLocation(shader_program, "r_normal") != -1)
    {
        shader->attribs |= R_ATTRIB_NORMAL;
        glBindAttribLocation(shader_program, R_NORMAL_LOCATION, "r_normal");
    }

    if(glGetAttribLocation(shader_program, "r_tangent") != -1)
    {
        shader->attribs |= R_ATTRIB_TANGENT;
        glBindAttribLocation(shader_program, R_TANGENT_LOCATION, "r_tangent");
    }

    if(glGetAttribLocation(shader_program, "r_color") != -1)
    {
        shader->attribs |= R_ATTRIB_COLOR;
        glBindAttribLocation(shader_program, R_COLOR_LOCATION, "r_color");
    }

    return shader;
}

void r_FreeShader(struct r_shader_t *shader)
{
    if(shader && shader->handle < 0xffffffff)
    {
        glDeleteProgram(shader->handle);
        shader->handle = 0xffffffff;
        ds_slist_remove_element(&r_shaders, shader->index);
    }
}

void r_BindShader(struct r_shader_t *shader)
{
    if(shader)
    {
        r_renderer_stats.shader_swaps++;

        r_current_shader = shader;
        glUseProgram(shader->handle);

        r_SetDefaultUniformI(R_UNIFORM_WIDTH, r_width);
        r_SetDefaultUniformI(R_UNIFORM_HEIGHT, r_height);
        r_SetDefaultUniformF(R_UNIFORM_Z_NEAR, r_z_near);
        r_SetDefaultUniformF(R_UNIFORM_CLUSTER_DENOM, r_denom);

        if(shader->attribs & R_ATTRIB_POSITION)
        {
            glEnableVertexArrayAttrib(r_vao, R_POSITION_LOCATION);
            glVertexAttribPointer(R_POSITION_LOCATION, 3, GL_FLOAT, GL_FALSE, sizeof(struct r_vert_t), (void *)offsetof(struct r_vert_t, pos));
        }
        else
        {
            glDisableVertexArrayAttrib(r_vao, R_POSITION_LOCATION);
        }

        if(shader->attribs & R_ATTRIB_NORMAL)
        {
            glEnableVertexArrayAttrib(r_vao, R_NORMAL_LOCATION);
            glVertexAttribPointer(R_NORMAL_LOCATION, 4, GL_FLOAT, GL_FALSE, sizeof(struct r_vert_t), (void *)offsetof(struct r_vert_t, normal));
        }
        else
        {
            glDisableVertexArrayAttrib(r_vao, R_NORMAL_LOCATION);
        }

        if(shader->attribs & R_ATTRIB_TANGENT)
        {
            glEnableVertexArrayAttrib(r_vao, R_TANGENT_LOCATION);
            glVertexAttribPointer(R_TANGENT_LOCATION, 3, GL_FLOAT, GL_FALSE, sizeof(struct r_vert_t), (void *)offsetof(struct r_vert_t, tangent));
        }
        else
        {
            glDisableVertexArrayAttrib(r_vao, R_TANGENT_LOCATION);
        }

        if(shader->attribs & R_ATTRIB_TEX_COORDS)
        {
            glEnableVertexArrayAttrib(r_vao, R_TEX_COORDS_LOCATION);
            glVertexAttribPointer(R_TEX_COORDS_LOCATION, 2, GL_FLOAT, GL_FALSE, sizeof(struct r_vert_t), (void *)offsetof(struct r_vert_t, tex_coords));
        }
        else
        {
            glDisableVertexArrayAttrib(r_vao, R_TEX_COORDS_LOCATION);
        }
    }
}

struct r_named_uniform_t *r_GetNamedUniform(struct r_shader_t *shader, char *name)
{
    if(shader)
    {
        if(!shader->named_uniforms.elem_size)
        {
            shader->named_uniforms = ds_list_create(sizeof(struct r_named_uniform_t), 8);
        }
    }

    for(uint32_t uniform_index = 0; uniform_index < shader->named_uniforms.cursor; uniform_index++)
    {
        struct r_named_uniform_t *uniform = ds_list_get_element(&shader->named_uniforms, uniform_index);

        if(!strcmp(uniform->name, name))
        {
            return uniform;
        }
    }

    struct r_named_uniform_t *uniform = NULL;
    uint32_t uniform_location = glGetUniformLocation(shader->handle, name);
    if(uniform_location != GL_INVALID_INDEX)
    {
        GLenum type;
        GLint size;
        uint32_t uniform_type;
        glGetActiveUniform(shader->handle, uniform_location, 0, NULL, &size, &type, NULL);

        switch(type)
        {
            case GL_FLOAT:
                uniform_type = R_UNIFORM_TYPE_FLOAT;
            break;

            case GL_INT:
            case GL_SAMPLER_1D:
            case GL_SAMPLER_2D:
            case GL_SAMPLER_3D:
            case GL_SAMPLER_CUBE:
            case GL_SAMPLER_1D_SHADOW:
            case GL_SAMPLER_2D_SHADOW:
            case GL_SAMPLER_1D_ARRAY:
            case GL_SAMPLER_2D_ARRAY:
            case GL_UNSIGNED_INT_SAMPLER_1D:
            case GL_UNSIGNED_INT_SAMPLER_2D:
            case GL_UNSIGNED_INT_SAMPLER_3D:
            case GL_UNSIGNED_INT_SAMPLER_CUBE:
                uniform_type = R_UNIFORM_TYPE_INT;
            break;

            case GL_UNSIGNED_INT:
                uniform_type = R_UNIFORM_TYPE_UINT;
            break;

            case GL_FLOAT_VEC2:
                uniform_type = R_UNIFORM_TYPE_VEC2;
            break;

            case GL_FLOAT_VEC3:
                uniform_type = R_UNIFORM_TYPE_VEC3;
            break;

            case GL_FLOAT_VEC4:
                uniform_type = R_UNIFORM_TYPE_VEC4;
            break;

            case GL_FLOAT_MAT2:
                uniform_type = R_UNIFORM_TYPE_MAT2;
            break;

            case GL_FLOAT_MAT3:
                uniform_type = R_UNIFORM_TYPE_MAT3;
            break;

            case GL_FLOAT_MAT4:
                uniform_type = R_UNIFORM_TYPE_MAT4;
            break;

            default:
                uniform_type = R_UNIFORM_TYPE_UNKNOWN;
            break;
        }

        if(uniform_type != R_UNIFORM_TYPE_UNKNOWN)
        {
            uint32_t index = ds_list_add_element(&shader->named_uniforms, NULL);
            uniform = ds_list_get_element(&shader->named_uniforms, index);
            uniform->uniform.location = uniform_location;
            uniform->type = uniform_type;
            uniform->name = mem_Calloc(1, strlen(name) + 1);
            strcpy(uniform->name, name);
        }
    }

    return uniform;
}

uint32_t r_GetUniformIndex(struct r_shader_t *shader, char *name)
{
    return glGetUniformLocation(shader->handle, name);
}

void r_SetDefaultUniformI(uint32_t uniform, int32_t value)
{
    if(r_current_shader->uniforms[uniform].location != GL_INVALID_INDEX)
    {
        glUniform1i(r_current_shader->uniforms[uniform].location, value);
    }
}

void r_SetDefaultUniformF(uint32_t uniform, float value)
{
    if(r_current_shader->uniforms[uniform].location != GL_INVALID_INDEX)
    {
        glUniform1f(r_current_shader->uniforms[uniform].location, value);
    }
}

void r_SetDefaultUniformVec2(uint32_t uniform, vec2_t *value)
{
    if(r_current_shader->uniforms[uniform].location != GL_INVALID_INDEX)
    {
        glUniform2fv(r_current_shader->uniforms[uniform].location, 1, value->comps);
    }
}

void r_SetDefaultUniformMat4(uint32_t uniform, mat4_t *matrix)
{
    if(r_current_shader->uniforms[uniform].location != GL_INVALID_INDEX)
    {
        glUniformMatrix4fv(r_current_shader->uniforms[uniform].location, 1, GL_FALSE, (const float *)matrix->comps);
    }
}

void r_SetNamedUniformByName(char *uniform, void *value)
{
    struct r_named_uniform_t *named_uniform = r_GetNamedUniform(r_current_shader, uniform);
    r_SetNamedUniform(named_uniform, value);
}

void r_SetNamedUniform(struct r_named_uniform_t *uniform, void *value)
{
    if(uniform)
    {
        switch(uniform->type)
        {
            case R_UNIFORM_TYPE_VEC4:
                r_SetNamedUniformVec4(uniform, value);
            break;
        }
    }
}

void r_SetNamedUniformI(struct r_named_uniform_t *uniform, int32_t value)
{
    if(uniform && uniform->uniform.location != GL_INVALID_INDEX)
    {
        glUniform1i(uniform->uniform.location, value);
    }
}

void r_SetNamedUniformVec4(struct r_named_uniform_t *uniform, vec4_t *value)
{
    if(uniform && uniform->uniform.location != GL_INVALID_INDEX)
    {
        glUniform4fv(uniform->uniform.location, 1, (const float *)value->comps);
    }
}

/*
============================================================================
============================================================================
============================================================================
*/
