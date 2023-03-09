#include "r_main.h"
#include "SDL2/SDL.h"
#include "GL/glew.h"
#include "stb/stb_image.h"
#include "dstuff/ds_slist.h"
#include "dstuff/ds_path.h"
#include "dstuff/ds_file.h"
#include "dstuff/ds_mem.h"
#include "dstuff/ds_obj.h"
#include "dstuff/ds_alloc.h"
#include "r_draw.h"
#include "stb/stb_include.h"
#include "anim.h"
#include "log.h"

//extern struct stack_list_t d_shaders;
//extern struct stack_list_t d_textures;
//extern struct stack_list_t d_materials;
//extern struct stack_list_t d_models;
//extern struct d_texture_t *d_default_texture;
//
//extern struct ds_heap_t d_vertex_heap;
//extern struct ds_heap_t d_index_heap;



struct ds_list_t r_world_cmds;
//struct ds_list_t r_entity_cmds;
struct r_cmd_buffer_t r_entity_cmd_buffer;
struct r_cmd_buffer_t r_shadow_cmd_buffer;
struct r_i_cmd_buffer_t r_immediate_cmd_buffer;
//struct ds_list_t r_shadow_cmds;
//struct ds_list_t r_immediate_cmds;
//struct ds_list_t r_immediate_data;


//struct list_t r_sorted_batches;
//struct list_t r_immediate_batches;
struct ds_slist_t r_shaders;
struct ds_slist_t r_textures;
struct ds_slist_t r_texture_descs;
struct ds_slist_t r_materials;
struct ds_slist_t r_models;
struct ds_slist_t r_lights[R_LIGHT_TYPE_LAST];
struct ds_slist_t r_vis_items;
struct ds_slist_t r_framebuffers;
struct ds_slist_t r_views;
struct ds_list_t r_visible_lights;

uint32_t r_vertex_buffer;
struct ds_heap_t r_vertex_heap;
uint32_t r_index_buffer;
struct ds_heap_t r_index_heap;
uint32_t r_immediate_cursor;
uint32_t r_immediate_vertex_buffer;
uint32_t r_immediate_index_buffer;
struct r_vert_t *r_vert_copy_buffer;

uint32_t r_vao;
struct r_shader_t *r_z_prepass_shader;
struct r_shader_t *r_lit_shader;
struct r_shader_t *r_immediate_shader;
struct r_shader_t *r_current_shader;
struct r_shader_t *r_shadow_shader;
struct r_shader_t *r_volumetric_shader;
struct r_shader_t *r_bilateral_blend_shader;
struct r_shader_t *r_full_screen_blend_shader;
struct r_shader_t *r_blend_ui_debug_shader;

//struct r_shader_t *r_default_shaders[];

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

uint32_t r_point_light_data_uniform_buffer;
uint32_t r_point_light_buffer_cursor;
struct r_point_data_t *r_point_light_buffer;
struct r_model_t *r_point_light_model;

uint32_t r_spot_light_data_uniform_buffer;
uint32_t r_spot_light_buffer_cursor;
struct r_spot_data_t *r_spot_light_buffer;
struct r_model_t *r_spot_light_model;

uint32_t r_light_index_buffer_cursor;
uint32_t r_light_index_uniform_buffer;
uint32_t *r_light_index_buffer;

struct r_shadow_map_t *r_shadow_map_buffer;
uint32_t r_shadow_map_buffer_cursor;
uint32_t r_shadow_map_uniform_buffer;

uint32_t r_shadow_atlas_texture;
uint32_t r_indirect_texture;
uint32_t r_shadow_map_framebuffer;
struct r_shadow_bucket_t r_shadow_buckets[R_SHADOW_BUCKET_COUNT];
struct ds_chunk_h r_screen_tri_chunk;
uint32_t r_screen_tri_start;
uint32_t r_immediate_cmd_id = 0;

char *r_light_type_names[R_LIGHT_TYPE_LAST] =
{
    [R_LIGHT_TYPE_POINT] = "Point",
    [R_LIGHT_TYPE_SPOT] = "Spot"
};


vec2_t r_point_shadow_projection_params;
mat4_t r_point_shadow_view_projection_matrices[6];
vec3_t r_point_light_frustum_planes[6];
uint16_t r_point_light_frustum_masks[6];

vec4_t r_clear_color;

//uint32_t r_main_framebuffer;
//uint32_t r_main_color_attachment;
//uint32_t r_main_depth_attachment;
//uint32_t r_z_prepass_framebuffer;

struct r_framebuffer_t *r_active_framebuffer;
//struct r_framebuffer_t *r_debug_framebuffer;
struct r_framebuffer_t *r_main_framebuffer;
struct r_framebuffer_t *r_debug_framebuffer;
struct r_framebuffer_t *r_ui_framebuffer;
struct r_framebuffer_t *r_volume_framebuffer;
//struct r_framebuffer_t *r_z_prepass_framebuffer;

extern struct r_renderer_state_t r_renderer_state;
//extern struct r_renderer_stats_t r_renderer_stats;

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

//struct r_named_uniform_t r_default_uniforms[] =
//{
//    [R_UNIFORM_MODEL_VIEW_PROJECTION_MATRIX] =  (struct r_named_uniform_t){.type = R_UNIFORM_TYPE_MAT4,  .name = "r_model_view_projection_matrix" },
//    [R_UNIFORM_VIEW_PROJECTION_MATRIX] =        (struct r_named_uniform_t){.type = R_UNIFORM_TYPE_MAT4,  .name = "r_view_projection_matrix" },
//    [R_UNIFORM_PROJECTION_MATRIX] =             (struct r_named_uniform_t){.type = R_UNIFORM_TYPE_MAT4,  .name = "r_projection_matrix" },
//    [R_UNIFORM_MODEL_VIEW_MATRIX] =             (struct r_named_uniform_t){.type = R_UNIFORM_TYPE_MAT4,  .name = "r_model_view_matrix" },
//    [R_UNIFORM_VIEW_MATRIX] =                   (struct r_named_uniform_t){.type = R_UNIFORM_TYPE_MAT4,  .name = "r_view_matrix" },
//    [R_UNIFORM_CAMERA_MATRIX] =                 (struct r_named_uniform_t){.type = R_UNIFORM_TYPE_MAT4,  .name = "r_camera_matrix" },
//    [R_UNIFORM_MODEL_MATRIX] =                  (struct r_named_uniform_t){.type = R_UNIFORM_TYPE_MAT4,  .name = "r_model_matrix" },
//    [R_UNIFORM_POINT_PROJ_PARAMS] =             (struct r_named_uniform_t){.type = R_UNIFORM_TYPE_VEC2,  .name = "r_point_proj_params" },
//    [R_UNIFORM_TEX0] =                          (struct r_named_uniform_t){.type = R_UNIFORM_TYPE_INT,   .name = "r_tex0" },
//    [R_UNIFORM_TEX1] =                          (struct r_named_uniform_t){.type = R_UNIFORM_TYPE_INT,   .name = "r_tex1" },
//    [R_UNIFORM_TEX3] =                          (struct r_named_uniform_t){.type = R_UNIFORM_TYPE_INT,   .name = "r_tex3" },
//    [R_UNIFORM_TEX2] =                          (struct r_named_uniform_t){.type = R_UNIFORM_TYPE_INT,   .name = "r_tex2" },
//    [R_UNIFORM_TEX4] =                          (struct r_named_uniform_t){.type = R_UNIFORM_TYPE_INT,   .name = "r_tex4" },
//    [R_UNIFORM_TEX_ALBEDO] =                    (struct r_named_uniform_t){.type = R_UNIFORM_TYPE_INT,   .name = "r_tex_albedo" },
//    [R_UNIFORM_TEX_NORMAL] =                    (struct r_named_uniform_t){.type = R_UNIFORM_TYPE_INT,   .name = "r_tex_normal" },
//    [R_UNIFORM_TEX_METALNESS] =                 (struct r_named_uniform_t){.type = R_UNIFORM_TYPE_INT,   .name = "r_tex_metalness" },
//    [R_UNIFORM_TEX_ROUGHNESS] =                 (struct r_named_uniform_t){.type = R_UNIFORM_TYPE_INT,   .name = "r_tex_roughness" },
//    [R_UNIFORM_TEX_HEIGHT] =                    (struct r_named_uniform_t){.type = R_UNIFORM_TYPE_INT,   .name = "r_tex_height" },
//    [R_UNIFORM_TEX_CLUSTERS] =                  (struct r_named_uniform_t){.type = R_UNIFORM_TYPE_INT,   .name = "r_tex_clusters" },
//    [R_UNIFORM_TEX_SHADOW_ATLAS] =              (struct r_named_uniform_t){.type = R_UNIFORM_TYPE_INT,   .name = "r_tex_shadow_atlas" },
//    [R_UNIFORM_TEX_INDIRECT] =                  (struct r_named_uniform_t){.type = R_UNIFORM_TYPE_INT,   .name = "r_tex_indirect" },
//    [R_UNIFORM_CLUSTER_DENOM] =                 (struct r_named_uniform_t){.type = R_UNIFORM_TYPE_FLOAT, .name = "r_cluster_denom" },
//    [R_UNIFORM_SPOT_LIGHT_COUNT] =              (struct r_named_uniform_t){.type = R_UNIFORM_TYPE_UINT,  .name = "r_spot_light_count" },
//    [R_UNIFORM_POINT_LIGHT_COUNT] =             (struct r_named_uniform_t){.type = R_UNIFORM_TYPE_UINT,  .name = "r_point_light_count" },
//    [R_UNIFORM_Z_NEAR] =                        (struct r_named_uniform_t){.type = R_UNIFORM_TYPE_FLOAT, .name = "r_z_near" },
//    [R_UNIFORM_Z_FAR] =                         (struct r_named_uniform_t){.type = R_UNIFORM_TYPE_FLOAT, .name = "r_z_far" },
//    [R_UNIFORM_WIDTH] =                         (struct r_named_uniform_t){.type = R_UNIFORM_TYPE_INT,   .name = "r_width" },
//    [R_UNIFORM_HEIGHT] =                        (struct r_named_uniform_t){.type = R_UNIFORM_TYPE_INT,   .name = "r_height" },
//};

float r_spot_light_tan_lut[1 + (R_SPOT_LIGHT_MAX_ANGLE - R_SPOT_LIGHT_MIN_ANGLE)];
float r_spot_light_cos_lut[1 + (R_SPOT_LIGHT_MAX_ANGLE - R_SPOT_LIGHT_MIN_ANGLE)];
uint32_t r_light_shadow_map_count[] =
{
    [R_LIGHT_TYPE_POINT] = 6,
    [R_LIGHT_TYPE_SPOT] = 1
};

uint32_t r_uniform_type_sizes[] =
{
    [R_UNIFORM_TYPE_UINT] =     sizeof(uint32_t),
    [R_UNIFORM_TYPE_INT] =      sizeof(int32_t),
    [R_UNIFORM_TYPE_FLOAT] =    sizeof(float),
    [R_UNIFORM_TYPE_VEC2] =     sizeof(vec2_t),
    [R_UNIFORM_TYPE_VEC3] =     sizeof(vec3_t),
    [R_UNIFORM_TYPE_VEC4] =     sizeof(vec4_t),
    [R_UNIFORM_TYPE_MAT2] =     sizeof(mat2_t),
    [R_UNIFORM_TYPE_MAT3] =     sizeof(mat3_t),
    [R_UNIFORM_TYPE_MAT4] =     sizeof(mat4_t),
    [R_UNIFORM_TYPE_TEXTURE] =  sizeof(uint32_t)
};

struct r_gl_format_info_t r_gl_format_info[] =
{
    [R_FORMAT_RG8] =                {.type = GL_UNSIGNED_BYTE,      .size = 2, .data_format = GL_RG, .internal_format = GL_RG8},
    [R_FORMAT_RG8I] =               {.type = GL_BYTE,               .size = 2, .data_format = GL_RG_INTEGER, .internal_format = GL_RG8I},
    [R_FORMAT_RG8UI] =              {.type = GL_UNSIGNED_BYTE,      .size = 2, .data_format = GL_RG_INTEGER, .internal_format = GL_RG8UI},

    [R_FORMAT_RG16] =               {.type = GL_UNSIGNED_SHORT,     .size = 2, .data_format = GL_RG, .internal_format = GL_RG16},
    [R_FORMAT_RG16I] =              {.type = GL_SHORT,              .size = 2, .data_format = GL_RG_INTEGER, .internal_format = GL_RG16I},
    [R_FORMAT_RG16UI] =             {.type = GL_UNSIGNED_SHORT,     .size = 2, .data_format = GL_RG_INTEGER, .internal_format = GL_RG16UI},
    [R_FORMAT_RG16F] =              {.type = GL_HALF_FLOAT,         .size = 2, .data_format = GL_RG, .internal_format = GL_RG16F},

    [R_FORMAT_RG32] =               {.type = GL_UNSIGNED_INT,       .size = 2, .data_format = GL_RG_INTEGER, .internal_format = GL_RG32I},
    [R_FORMAT_RG32I] =              {.type = GL_INT,                .size = 2, .data_format = GL_RG_INTEGER, .internal_format = GL_RG32I},
    [R_FORMAT_RG32UI] =             {.type = GL_UNSIGNED_INT,       .size = 2, .data_format = GL_RG_INTEGER, .internal_format = GL_RG32UI},
    [R_FORMAT_RG32F] =              {.type = GL_FLOAT,              .size = 2, .data_format = GL_RG, .internal_format = GL_RG32F},

    [R_FORMAT_RGB8] =               {.type = GL_UNSIGNED_BYTE,      .size = 3, .data_format = GL_RGB, .internal_format = GL_RGB8},
    [R_FORMAT_RGB8I] =              {.type = GL_BYTE,               .size = 3, .data_format = GL_RGB_INTEGER, .internal_format = GL_RGB8I},
    [R_FORMAT_RGB8UI] =             {.type = GL_UNSIGNED_BYTE,      .size = 3, .data_format = GL_RGB_INTEGER, .internal_format = GL_RGB8UI},

    [R_FORMAT_RGB16] =              {.type = GL_UNSIGNED_SHORT,     .size = 3, .data_format = GL_RGB, .internal_format = GL_RGB16},
    [R_FORMAT_RGB16I] =             {.type = GL_SHORT,              .size = 3, .data_format = GL_RGB_INTEGER, .internal_format = GL_RGB16I},
    [R_FORMAT_RGB16UI] =            {.type = GL_UNSIGNED_SHORT,     .size = 3, .data_format = GL_RGB_INTEGER, .internal_format = GL_RGB16UI},
    [R_FORMAT_RGB16F] =             {.type = GL_HALF_FLOAT,         .size = 3, .data_format = GL_RGB, .internal_format = GL_RGB16F},

    [R_FORMAT_RGB32] =              {.type = GL_UNSIGNED_INT,       .size = 3, .data_format = GL_RGB, .internal_format = GL_RGB32I},
    [R_FORMAT_RGB32I] =             {.type = GL_INT,                .size = 3, .data_format = GL_RGB_INTEGER, .internal_format = GL_RGB32I},
    [R_FORMAT_RGB32UI] =            {.type = GL_UNSIGNED_INT,       .size = 3, .data_format = GL_RGB_INTEGER, .internal_format = GL_RGB32UI},
    [R_FORMAT_RGB32F] =             {.type = GL_FLOAT,              .size = 3, .data_format = GL_RGB, .internal_format = GL_RGB32F},

    [R_FORMAT_RGBA8] =              {.type = GL_UNSIGNED_BYTE,      .size = 4, .data_format = GL_RGBA, .internal_format = GL_RGBA8},
    [R_FORMAT_RGBA8I] =             {.type = GL_BYTE,               .size = 4, .data_format = GL_RGBA_INTEGER, .internal_format = GL_RGBA8I},
    [R_FORMAT_RGBA8UI] =            {.type = GL_UNSIGNED_BYTE,      .size = 4, .data_format = GL_RGBA_INTEGER, .internal_format = GL_RGBA8UI},

    [R_FORMAT_RGBA16] =             {.type = GL_UNSIGNED_SHORT,     .size = 4, .data_format = GL_RGBA, .internal_format = GL_RGBA16},
    [R_FORMAT_RGBA16I] =            {.type = GL_SHORT,              .size = 4, .data_format = GL_RGBA_INTEGER, .internal_format = GL_RGBA16I},
    [R_FORMAT_RGBA16UI] =           {.type = GL_UNSIGNED_SHORT,     .size = 4, .data_format = GL_RGBA_INTEGER, .internal_format = GL_RGBA16UI},
    [R_FORMAT_RGBA16F] =            {.type = GL_HALF_FLOAT,         .size = 4, .data_format = GL_RGBA, .internal_format = GL_RGBA16F},

    [R_FORMAT_RGBA32] =             {.type = GL_UNSIGNED_INT,       .size = 4, .data_format = GL_RGBA, .internal_format = GL_RGBA32I},
    [R_FORMAT_RGBA32I] =            {.type = GL_INT,                .size = 4, .data_format = GL_RGBA_INTEGER, .internal_format = GL_RGBA32I},
    [R_FORMAT_RGBA32UI] =           {.type = GL_UNSIGNED_INT,       .size = 4, .data_format = GL_RGBA_INTEGER, .internal_format = GL_RGBA32UI},
    [R_FORMAT_RGBA32F] =            {.type = GL_FLOAT,              .size = 4, .data_format = GL_RGBA, .internal_format = GL_RGBA32F},

    [R_FORMAT_DEPTH16] =            {.type = GL_UNSIGNED_SHORT,     .size = 1, .data_format = GL_DEPTH_COMPONENT, .internal_format = GL_DEPTH_COMPONENT16},
    [R_FORMAT_DEPTH32] =            {.type = GL_UNSIGNED_INT,       .size = 1, .data_format = GL_DEPTH_COMPONENT, .internal_format = GL_DEPTH_COMPONENT32},
    [R_FORMAT_DEPTH32F] =           {.type = GL_FLOAT,              .size = 1, .data_format = GL_DEPTH_COMPONENT, .internal_format = GL_DEPTH_COMPONENT32F},
    [R_FORMAT_DEPTH24_STENCIL8] =   {.type = GL_UNSIGNED_INT_24_8,  .size = 1, .data_format = GL_DEPTH_STENCIL, .internal_format = GL_DEPTH24_STENCIL8}
};

//case GL_DEPTH_COMPONENT16:
//            *data_type = GL_UNSIGNED_SHORT;
//            *data_format = GL_DEPTH_COMPONENT;
//        break;
//
//        case GL_DEPTH_COMPONENT32:
//            *data_type = GL_UNSIGNED_INT;
//            *data_format = GL_DEPTH_COMPONENT;
//        break;
//
//        case GL_DEPTH_COMPONENT32F:
//            *data_type = GL_FLOAT;
//            *data_format = GL_DEPTH_COMPONENT;
//        break;
//
//        case GL_DEPTH24_STENCIL8:
//            *data_type = GL_UNSIGNED_INT_24_8;
//            *data_format = GL_DEPTH_STENCIL;
//        break;

//struct r_vertex_layout_t r_default_vertex_layout =
//{
//    .stride = sizeof(struct r_vert_t),
//    .attrib_count = 4,
//    .attribs = (union r_vertex_attrib_t[])
//    {
//        {.format = R_FORMAT_RGB32F,   .offset = offsetof(struct r_vert_t, pos),         .location = R_POSITION_LOCATION},
//        {.format = R_FORMAT_RGBA32F,  .offset = offsetof(struct r_vert_t, normal),      .location = R_NORMAL_LOCATION},
//        {.format = R_FORMAT_RGB32F,   .offset = offsetof(struct r_vert_t, tangent),     .location = R_TANGENT_LOCATION},
//        {.format = R_FORMAT_RG32F,    .offset = offsetof(struct r_vert_t, tex_coords),  .location = R_TEX_COORDS_LOCATION},
//    }
//};

#ifdef __cplusplus
extern "C"
{
#endif

void r_Init()
{
    log_ScopedLogMessage(LOG_TYPE_NOTICE, "Initializing renderer...");

    r_world_cmds = ds_list_create(sizeof(struct r_world_cmd_t), 8192);
//    r_entity_cmds = ds_list_create(sizeof(struct r_entity_cmd_t), 8192);
    r_entity_cmd_buffer = r_AllocCmdBuffer(sizeof(struct r_entity_cmd_t));
    r_shadow_cmd_buffer = r_AllocCmdBuffer(sizeof(struct r_shadow_cmd_t));
    r_immediate_cmd_buffer = r_AllocImmediateCmdBuffer(sizeof(struct r_i_cmd_t));
//    r_shadow_cmds = ds_list_create(sizeof(struct r_shadow_cmd_t), 8192);
//    r_immediate_cmds = ds_list_create(sizeof(struct r_immediate_cmd_t), 4096);
//    r_immediate_data = ds_list_create(R_IMMEDIATE_DATA_SLOT_SIZE, 32768);

    r_shaders = ds_slist_create(sizeof(struct r_shader_t), 16);
    r_materials = ds_slist_create(sizeof(struct r_material_t), 32);
    r_textures = ds_slist_create(sizeof(struct r_texture_t), 128);
    r_texture_descs = ds_slist_create(sizeof(struct r_texture_desc_t), 128);
    r_models = ds_slist_create(sizeof(struct r_model_t), 512);
    r_framebuffers = ds_slist_create(sizeof(struct r_framebuffer_t), 8);
    r_lights[R_LIGHT_TYPE_POINT] = ds_slist_create(sizeof(struct r_point_light_t), 512);
    r_lights[R_LIGHT_TYPE_SPOT] = ds_slist_create(sizeof(struct r_spot_light_t), 512);
    r_visible_lights = ds_list_create(sizeof(struct r_light_t *), 512);
    r_vertex_heap = ds_create_heap(R_VERTEX_BUFFER_SIZE);
    r_index_heap = ds_create_heap(R_INDEX_BUFFER_SIZE);
//    r_vert_copy_buffer = mem_Calloc(R_VERT_COPY_BUFFER_SIZE, sizeof(struct r_vert_t));

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    printf("shit\n");
    r_window = SDL_CreateWindow("doh", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, r_width, r_height, SDL_WINDOW_OPENGL);
    printf("ass\n");
    r_context = SDL_GL_CreateContext(r_window);
    SDL_GL_MakeCurrent(r_window, r_context);
    SDL_GL_SetSwapInterval(1);
    printf("nuts\n");

    GLenum status = glewInit();
    if(status != GLEW_OK)
    {
        log_ScopedLogMessage(LOG_TYPE_FATAL, "Couldn't initialize GLEW!\nError message: %s");
        exit(-1);
    }

    log_ScopedLogMessage(LOG_TYPE_NOTICE, "Vendor: %s", glGetString(GL_VENDOR));
    log_ScopedLogMessage(LOG_TYPE_NOTICE, "Renderer: %s", glGetString(GL_RENDERER));
    log_ScopedLogMessage(LOG_TYPE_NOTICE, "Version: %s", glGetString(GL_VERSION));
    log_ScopedLogMessage(LOG_TYPE_NOTICE, "GLSL: %s", glGetString(GL_SHADING_LANGUAGE_VERSION));

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    r_SetClearColor(0.0, 0.0, 0.0, 1.0);

    glClearDepth(1.0);
    glClearStencil(0x00);

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

    struct r_shader_desc_t shader_desc;

    shader_desc = (struct r_shader_desc_t){
        .vertex_code = "shaders/r_z_prepass.vert",
        .fragment_code = "shaders/r_z_prepass.frag",
        .vertex_layout = &R_DEFAULT_VERTEX_LAYOUT,
        .uniforms = R_DEFAULT_UNIFORMS,
        .uniform_count = sizeof(R_DEFAULT_UNIFORMS) / sizeof(R_DEFAULT_UNIFORMS[0])
    };
    r_z_prepass_shader = r_LoadShader(&shader_desc);

    shader_desc = (struct r_shader_desc_t){
        .vertex_code = "shaders/r_clustered_forward.vert",
        .fragment_code = "shaders/r_clustered_forward.frag",
        .vertex_layout = &R_DEFAULT_VERTEX_LAYOUT,
        .uniforms = R_DEFAULT_UNIFORMS,
        .uniform_count = sizeof(R_DEFAULT_UNIFORMS) / sizeof(R_DEFAULT_UNIFORMS[0])
    };
    r_lit_shader = r_LoadShader(&shader_desc);

    shader_desc = (struct r_shader_desc_t){
        .vertex_code = "shaders/r_shadow.vert",
        .fragment_code = "shaders/r_shadow.frag",
        .vertex_layout = &R_DEFAULT_VERTEX_LAYOUT,
        .uniforms = R_DEFAULT_UNIFORMS,
        .uniform_count = sizeof(R_DEFAULT_UNIFORMS) / sizeof(R_DEFAULT_UNIFORMS[0])
    };
    r_shadow_shader = r_LoadShader(&shader_desc);

    shader_desc = (struct r_shader_desc_t){
        .vertex_code = "shaders/r_immediate_mode.vert",
        .fragment_code = "shaders/r_immediate_mode.frag",
        .vertex_layout = &R_DEFAULT_VERTEX_LAYOUT,
        .uniforms = R_DEFAULT_UNIFORMS,
        .uniform_count = sizeof(R_DEFAULT_UNIFORMS) / sizeof(R_DEFAULT_UNIFORMS[0])
    };
    r_immediate_shader = r_LoadShader(&shader_desc);

    shader_desc = (struct r_shader_desc_t){
        .vertex_code = "shaders/r_volumetric_lights.vert",
        .fragment_code = "shaders/r_volumetric_lights.frag",
        .vertex_layout = &R_DEFAULT_VERTEX_LAYOUT,
        .uniforms = R_DEFAULT_UNIFORMS,
        .uniform_count = sizeof(R_DEFAULT_UNIFORMS) / sizeof(R_DEFAULT_UNIFORMS[0])
    };
    r_volumetric_shader = r_LoadShader(&shader_desc);

    shader_desc = (struct r_shader_desc_t){
        .vertex_code = "shaders/r_bilateral_upsample.vert",
        .fragment_code = "shaders/r_bilateral_upsample.frag",
        .vertex_layout = &R_DEFAULT_VERTEX_LAYOUT,
        .uniforms = R_DEFAULT_UNIFORMS,
        .uniform_count = sizeof(R_DEFAULT_UNIFORMS) / sizeof(R_DEFAULT_UNIFORMS[0])
    };
    r_bilateral_blend_shader = r_LoadShader(&shader_desc);

    shader_desc = (struct r_shader_desc_t){
        .vertex_code = "shaders/r_full_screen_blend.vert",
        .fragment_code = "shaders/r_full_screen_blend.frag",
        .vertex_layout = &R_DEFAULT_VERTEX_LAYOUT,
        .uniforms = R_DEFAULT_UNIFORMS,
        .uniform_count = sizeof(R_DEFAULT_UNIFORMS) / sizeof(R_DEFAULT_UNIFORMS[0])
    };
    r_full_screen_blend_shader = r_LoadShader(&shader_desc);

    shader_desc = (struct r_shader_desc_t){
        .vertex_code = "shaders/r_blend_ui_debug.vert",
        .fragment_code = "shaders/r_blend_ui_debug.frag",
        .vertex_layout = &R_DEFAULT_VERTEX_LAYOUT,
        .uniforms = R_DEFAULT_UNIFORMS,
        .uniform_count = sizeof(R_DEFAULT_UNIFORMS) / sizeof(R_DEFAULT_UNIFORMS[0])
    };
    r_blend_ui_debug_shader = r_LoadShader(&shader_desc);

//    r_z_prepass_shader = r_LoadShader("shaders/r_z_prepass.vert", "shaders/r_z_prepass.frag");
//    r_lit_shader = r_LoadShader("shaders/r_clustered_forward.vert", "shaders/r_clustered_forward.frag");
//    r_shadow_shader = r_LoadShader("shaders/r_shadow.vert", "shaders/r_shadow.frag");
//    r_immediate_shader = r_LoadShader("shaders/r_immediate_mode.vert", "shaders/r_immediate_mode.frag");
//    r_volumetric_shader = r_LoadShader("shaders/r_volumetric_lights.vert", "shaders/r_volumetric_lights.frag");
//    r_bilateral_blend_shader = r_LoadShader("shaders/r_bilateral_upsample.vert", "shaders/r_bilateral_upsample.frag");
//    r_full_screen_blend_shader = r_LoadShader("shaders/r_full_screen_blend.vert", "shaders/r_full_screen_blend.frag");
//    r_blend_ui_debug_shader = r_LoadShader("shaders/r_blend_ui_debug.vert", "shaders/r_blend_ui_debug.frag");
    mat4_t_persp(&r_projection_matrix, r_fov, (float)r_width / (float)r_height, r_z_near, r_z_far);
    mat4_t_identity(&r_view_matrix);

    uint32_t pixels[] =
    {
        0xff777777, 0xff444444, 0xff777777, 0xff444444,
        0xff444444, 0xff777777, 0xff444444, 0xff777777,
        0xff777777, 0xff444444, 0xff777777, 0xff444444,
        0xff444444, 0xff777777, 0xff444444, 0xff777777,
    };

    struct r_texture_desc_t texture_desc;

    texture_desc = (struct r_texture_desc_t) {
        .format = R_FORMAT_RGBA8,
        .width = 4,
        .height = 4,
        .min_filter = GL_NEAREST,
        .mag_filter = GL_NEAREST,
    };
//    r_default_albedo_texture = r_CreateTexture("default_albedo", 4, 4, R_FORMAT_RGBA8, GL_NEAREST, GL_NEAREST, pixels);
    r_default_albedo_texture = r_CreateTexture("default_albedo", &texture_desc, pixels);

    pixels[0] = 0x00ff7f7f;
    texture_desc = (struct r_texture_desc_t) {
        .format = R_FORMAT_RGBA8,
        .width = 1,
        .height = 1,
        .min_filter = GL_LINEAR,
        .mag_filter = GL_LINEAR,
    };
//    r_default_normal_texture = r_CreateTexture("default_normal", 1, 1, R_FORMAT_RGBA8, GL_LINEAR, GL_LINEAR, pixels);
    r_default_normal_texture = r_CreateTexture("default_normal", &texture_desc, pixels);

    pixels[0] = 0x0000003f;
    pixels[1] = 0x0000009f;
    pixels[2] = 0x0000003f;
    pixels[3] = 0x0000009f;

    pixels[4] = 0x0000009f;
    pixels[5] = 0x0000003f;
    pixels[6] = 0x0000009f;
    pixels[7] = 0x0000003f;

    pixels[8] = 0x0000003f;
    pixels[9] = 0x0000009f;
    pixels[10] = 0x0000003f;
    pixels[11] = 0x0000009f;

    pixels[12] = 0x0000009f;
    pixels[13] = 0x0000003f;
    pixels[14] = 0x0000009f;
    pixels[15] = 0x0000003f;

    texture_desc = (struct r_texture_desc_t) {
        .format = R_FORMAT_RGBA8,
        .width = 1,
        .height = 1,
        .min_filter = GL_NEAREST,
        .mag_filter = GL_NEAREST,
    };
//    r_default_roughness_texture = r_CreateTexture("default_roughness", 4, 4, R_FORMAT_RGBA8, GL_NEAREST, GL_NEAREST, pixels);
    r_default_roughness_texture = r_CreateTexture("default_roughness", &texture_desc, pixels);
    r_default_material = r_CreateMaterial("default", NULL, NULL, NULL);

    r_clusters = mem_Calloc(R_CLUSTER_COUNT, sizeof(struct r_cluster_t));
    r_point_light_buffer = mem_Calloc(R_MAX_LIGHTS, sizeof(struct r_point_data_t));
    r_spot_light_buffer = mem_Calloc(R_MAX_LIGHTS, sizeof(struct r_spot_data_t));
    r_light_index_buffer = mem_Calloc(R_CLUSTER_COUNT * R_MAX_CLUSTER_LIGHTS, sizeof(uint32_t));
    r_shadow_map_buffer = mem_Calloc(R_MAX_LIGHTS * 6, sizeof(struct r_shadow_map_t));

//    r_shadow_index_buffer = mem_Calloc(R_MAX_LIGHTS * 6, sizeof(uint32_t));

    uint32_t cur_resolution = R_SHADOW_MAP_MIN_RESOLUTION;
    for(uint32_t bucket_index = 0; bucket_index < R_SHADOW_BUCKET_COUNT; bucket_index++)
    {
        uint32_t width = R_SHADOW_MAP_ATLAS_WIDTH / cur_resolution;
        uint32_t height = R_SHADOW_MAP_ATLAS_HEIGHT / cur_resolution;
        uint32_t tile_count = (width * height) / 4;

        struct r_shadow_bucket_t *bucket = r_shadow_buckets + bucket_index;
        bucket->tiles = mem_Calloc(tile_count, sizeof(struct r_shadow_tile_t));
        bucket->cur_free = 0xffff;
        bucket->cur_src = 0xffff;

        for(uint32_t tile_index = 0; tile_index < tile_count; tile_index++)
        {
            struct r_shadow_tile_t *tile = bucket->tiles + tile_index;
            tile->next = bucket->cur_free;
            tile->prev = 0xffff;

            if(bucket->cur_free != 0xffff)
            {
                bucket->tiles[bucket->cur_free].prev = tile_index;
            }

            bucket->cur_free = tile_index;
        }

        cur_resolution <<= 1;
    }

    uint32_t width = R_SHADOW_MAP_ATLAS_WIDTH / R_SHADOW_BUCKET4_RES;
    uint32_t height = R_SHADOW_MAP_ATLAS_HEIGHT / R_SHADOW_BUCKET4_RES;
    uint32_t tile_count = (width * height) / 4;

    uint32_t shadow_map_x = 0;
    uint32_t shadow_map_y = 0;
    struct r_shadow_bucket_t *bucket = r_shadow_buckets + R_SHADOW_BUCKET4;
    bucket->cur_free = 0xffff;

    for(uint32_t tile_index = 0; tile_index < tile_count; tile_index++)
    {
        struct r_shadow_tile_t *shadow_tile = bucket->tiles + tile_index;
        shadow_tile->used = 0;
        shadow_tile->next = bucket->cur_src;
        if(bucket->cur_src != 0xffff)
        {
            bucket->tiles[bucket->cur_src].prev = tile_index;
        }
        bucket->cur_src = tile_index;

        struct r_shadow_map_t *shadow_map = shadow_tile->shadow_maps;

//        shadow_map->coords = R_SHADOW_MAP_COORDS(shadow_map_x, shadow_map_y);
        shadow_map->x_coord = shadow_map_x;
        shadow_map->y_coord = shadow_map_y;
        shadow_map++;

//        shadow_map->coords = R_SHADOW_MAP_COORDS(shadow_map_x + R_SHADOW_BUCKET4_RES, shadow_map_y);
        shadow_map->x_coord = shadow_map_x + R_SHADOW_BUCKET4_RES;
        shadow_map->y_coord = shadow_map_y;
        shadow_map++;

//        shadow_map->coords = R_SHADOW_MAP_COORDS(shadow_map_x, shadow_map_y + R_SHADOW_BUCKET4_RES);
        shadow_map->x_coord = shadow_map_x;
        shadow_map->y_coord = shadow_map_y + R_SHADOW_BUCKET4_RES;
        shadow_map++;

//        shadow_map->coords = R_SHADOW_MAP_COORDS(shadow_map_x + R_SHADOW_BUCKET4_RES, shadow_map_y + R_SHADOW_BUCKET4_RES);
        shadow_map->x_coord = shadow_map_x + R_SHADOW_BUCKET4_RES;
        shadow_map->y_coord = shadow_map_y + R_SHADOW_BUCKET4_RES;
        shadow_map++;

        shadow_map_x += R_SHADOW_BUCKET4_RES * 2;
        if(shadow_map_x >= R_SHADOW_MAP_ATLAS_WIDTH)
        {
            shadow_map_x = 0;
            shadow_map_y += R_SHADOW_BUCKET4_RES * 2;
        }
    }


    /****************************************************************************/

    struct r_vert_t *light_model_verts;
    uint32_t *light_model_indices;

    float cur_angle = 0;
    float angle_increment = (3.14159265 * 2.0) / (float)R_SPOT_LIGHT_BASE_VERTS;
    uint32_t light_model_vert_count = R_SPOT_LIGHT_BASE_VERTS + 1;
    uint32_t light_model_index_count = 0;
    struct r_batch_t light_model_batch = {};
    struct r_model_geometry_t light_model_geometry = {};
    light_model_verts = mem_Calloc(light_model_vert_count, sizeof(struct r_vert_t));
    light_model_indices = mem_Calloc(R_SPOT_LIGHT_BASE_VERTS * 3, sizeof(uint32_t));

    struct r_vert_t *vert = light_model_verts;
    vert->pos = vec3_t_c(0.0, 0.0, 0.0);
    vert->color = vec4_t_c(0.8, 0.8, 1.0, 1.0);
    vert++;

    for(uint32_t vert_index = 0; vert_index < R_SPOT_LIGHT_BASE_VERTS;)
    {
        float cur_s = sin(cur_angle);
        float cur_c = cos(cur_angle);
        cur_angle += angle_increment;

        vert->pos = vec3_t_c(cur_c, cur_s, -1.0);
        vert->color = vec4_t_c(0.8, 0.8, 1.0, 1.0);
        vert++;

        vert_index++;

        light_model_indices[light_model_index_count++] = 0;
        light_model_indices[light_model_index_count++] = vert_index;
        light_model_indices[light_model_index_count++] = 1 + (vert_index % R_SPOT_LIGHT_BASE_VERTS);
    }

    light_model_batch.material = NULL;
    light_model_batch.start = 0;
    light_model_batch.count = light_model_index_count;

    light_model_geometry.batches = &light_model_batch;
    light_model_geometry.batch_count = 1;
    light_model_geometry.indices = light_model_indices;
    light_model_geometry.index_count = light_model_index_count;
    light_model_geometry.verts = light_model_verts;
    light_model_geometry.vert_count = light_model_vert_count;

    r_spot_light_model = r_CreateModel(&light_model_geometry, NULL, "spot_light_model");

    mem_Free(light_model_verts);
    mem_Free(light_model_indices);


    light_model_vert_count = R_POINT_LIGHT_VERTS * 3;
    light_model_verts = mem_Calloc(light_model_vert_count, sizeof(struct r_vert_t));

    light_model_index_count = light_model_vert_count * 2;
    light_model_indices = mem_Calloc(light_model_index_count, sizeof(uint32_t));

    cur_angle = 0.0;
    angle_increment = (3.14159265 * 2.0) / (float)R_POINT_LIGHT_VERTS;

    struct r_vert_t *vert0 = light_model_verts;
    struct r_vert_t *vert1 = vert0 + R_POINT_LIGHT_VERTS;
    struct r_vert_t *vert2 = vert1 + R_POINT_LIGHT_VERTS;

    uint32_t *index0 = light_model_indices;
    uint32_t *index1 = index0 + R_POINT_LIGHT_VERTS * 2;
    uint32_t *index2 = index1 + R_POINT_LIGHT_VERTS * 2 ;

    for(uint32_t vert_index = 0; vert_index < R_POINT_LIGHT_VERTS; vert_index++)
    {
        float cur_s = sin(cur_angle);
        float cur_c = cos(cur_angle);
        cur_angle += angle_increment;

        vert0->pos = vec3_t_c(cur_c, cur_s, 0.0);
        vert0->color = vec4_t_c(0.8, 0.8, 1.0, 1.0);

        vert1->pos.x = vert0->pos.x;
        vert1->pos.y = 0.0;
        vert1->pos.z = vert0->pos.y;
        vert1->color = vert0->color;

        vert2->pos.x = 0.0;
        vert2->pos.y = vert0->pos.y;
        vert2->pos.z = vert0->pos.x;
        vert2->color = vert0->color;

        vert0++;
        vert1++;
        vert2++;

        index0[0] = vert_index;
        index1[0] = index0[0] + R_POINT_LIGHT_VERTS;
        index2[0] = index1[0] + R_POINT_LIGHT_VERTS;

        index0[1] = (vert_index + 1) % R_POINT_LIGHT_VERTS;
        index1[1] = index0[1] + R_POINT_LIGHT_VERTS;
        index2[1] = index1[1] + R_POINT_LIGHT_VERTS;

        index0 += 2;
        index1 += 2;
        index2 += 2;
    }

    light_model_batch.start = 0;
    light_model_batch.count = light_model_vert_count;

    light_model_geometry.batches = &light_model_batch;
    light_model_geometry.batch_count = 1;
    light_model_geometry.indices = light_model_indices;
    light_model_geometry.index_count = light_model_index_count;
    light_model_geometry.verts = light_model_verts;
    light_model_geometry.vert_count = light_model_vert_count;

    r_point_light_model = r_CreateModel(&light_model_geometry, NULL, "point_light_model");

    mem_Free(light_model_verts);
    mem_Free(light_model_indices);

/****************************************************************************/


//    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    glGenTextures(1, &r_cluster_texture);
    glBindTexture(GL_TEXTURE_3D, r_cluster_texture);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAX_LEVEL, 0);
    glTexImage3D(GL_TEXTURE_3D, 0, GL_RGB32UI, R_CLUSTER_ROW_WIDTH, R_CLUSTER_ROWS, R_CLUSTER_SLICES, 0, GL_RGB_INTEGER, GL_UNSIGNED_INT, NULL);

    glGenBuffers(1, &r_point_light_data_uniform_buffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, r_point_light_data_uniform_buffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, R_MAX_LIGHTS * sizeof(struct r_point_data_t), NULL, GL_DYNAMIC_DRAW);

    glGenBuffers(1, &r_spot_light_data_uniform_buffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, r_spot_light_data_uniform_buffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, R_MAX_LIGHTS * sizeof(struct r_spot_data_t), NULL, GL_DYNAMIC_DRAW);

    glGenBuffers(1, &r_light_index_uniform_buffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, r_light_index_uniform_buffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, R_CLUSTER_COUNT * R_MAX_CLUSTER_LIGHTS * sizeof(uint32_t), NULL, GL_DYNAMIC_DRAW);

    width = R_SHADOW_MAP_ATLAS_WIDTH / R_SHADOW_BUCKET0_RES;
    height = R_SHADOW_MAP_ATLAS_HEIGHT / R_SHADOW_BUCKET0_RES;
    glGenBuffers(1, &r_shadow_map_uniform_buffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, r_shadow_map_uniform_buffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(struct r_shadow_map_t) * R_MAX_LIGHTS * 6, NULL, GL_DYNAMIC_DRAW);

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
        (0 << R_SHADOW_CUBEMAP_FACE_INDEX_SHIFT) | (2 << R_SHADOW_CUBEMAP_FACE_U_COORD_SHIFT) | (1 << R_SHADOW_CUBEMAP_FACE_V_COORD_SHIFT),
        /* -X */
        (1 << R_SHADOW_CUBEMAP_FACE_INDEX_SHIFT) | (1 << R_SHADOW_CUBEMAP_FACE_U_COORD_SHIFT) | (2 << R_SHADOW_CUBEMAP_FACE_V_COORD_SHIFT),
        /* +Y */
        (2 << R_SHADOW_CUBEMAP_FACE_INDEX_SHIFT) | (0 << R_SHADOW_CUBEMAP_FACE_U_COORD_SHIFT) | (2 << R_SHADOW_CUBEMAP_FACE_V_COORD_SHIFT),
        /* -Y */
        (3 << R_SHADOW_CUBEMAP_FACE_INDEX_SHIFT) | (2 << R_SHADOW_CUBEMAP_FACE_U_COORD_SHIFT) | (0 << R_SHADOW_CUBEMAP_FACE_V_COORD_SHIFT),
        /* +Z */
        (4 << R_SHADOW_CUBEMAP_FACE_INDEX_SHIFT) | (1 << R_SHADOW_CUBEMAP_FACE_U_COORD_SHIFT) | (0 << R_SHADOW_CUBEMAP_FACE_V_COORD_SHIFT),
        /* -Z */
        (5 << R_SHADOW_CUBEMAP_FACE_INDEX_SHIFT) | (0 << R_SHADOW_CUBEMAP_FACE_U_COORD_SHIFT) | (1 << R_SHADOW_CUBEMAP_FACE_V_COORD_SHIFT),
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
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
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

        r_point_light_frustum_planes[face_index] = vec3_t_c(0.0, 0.0, 1.0);
    }

    /* <0.707106769, 0.0, 0.707106769> */
    vec3_t_rotate_y(&r_point_light_frustum_planes[0], &r_point_light_frustum_planes[0], 0.25);

    /* <-0.707106769, 0.0, 0.707106769> */
    vec3_t_rotate_y(&r_point_light_frustum_planes[1], &r_point_light_frustum_planes[1], -0.25);

    /* <0.0, -0.707106769, 0.707106769> */
    vec3_t_rotate_x(&r_point_light_frustum_planes[2], &r_point_light_frustum_planes[2], 0.25);

    /* <0.0, 0.707106769, 0.707106769> */
    vec3_t_rotate_x(&r_point_light_frustum_planes[3], &r_point_light_frustum_planes[3], -0.25);

    /* <0.707106769, -0.707106769, 0.0> */
    vec3_t_rotate_x(&r_point_light_frustum_planes[4], &r_point_light_frustum_planes[4], 0.25);
    vec3_t_rotate_y(&r_point_light_frustum_planes[4], &r_point_light_frustum_planes[4], 0.5);

    /* <0.707106769, 0.707106769, 0.0> */
    vec3_t_rotate_x(&r_point_light_frustum_planes[5], &r_point_light_frustum_planes[5], -0.25);
    vec3_t_rotate_y(&r_point_light_frustum_planes[5], &r_point_light_frustum_planes[5], 0.5);

    /* +X */
    r_point_light_frustum_masks[0] = (R_POINT_LIGHT_FRUSTUM_PLANE_FRONT << R_POINT_LIGHT_FRUSTUM_PLANE0_SHIFT) |
                                      (R_POINT_LIGHT_FRUSTUM_PLANE_BACK << R_POINT_LIGHT_FRUSTUM_PLANE1_SHIFT) |
                                      (R_POINT_LIGHT_FRUSTUM_PLANE_FRONT << R_POINT_LIGHT_FRUSTUM_PLANE4_SHIFT) |
                                      (R_POINT_LIGHT_FRUSTUM_PLANE_FRONT << R_POINT_LIGHT_FRUSTUM_PLANE5_SHIFT);

    /* -X */
    r_point_light_frustum_masks[1] = (R_POINT_LIGHT_FRUSTUM_PLANE_BACK << R_POINT_LIGHT_FRUSTUM_PLANE0_SHIFT) |
                                      (R_POINT_LIGHT_FRUSTUM_PLANE_FRONT << R_POINT_LIGHT_FRUSTUM_PLANE1_SHIFT) |
                                      (R_POINT_LIGHT_FRUSTUM_PLANE_BACK << R_POINT_LIGHT_FRUSTUM_PLANE4_SHIFT) |
                                      (R_POINT_LIGHT_FRUSTUM_PLANE_BACK << R_POINT_LIGHT_FRUSTUM_PLANE5_SHIFT);

    /* +Y */
    r_point_light_frustum_masks[2] = (R_POINT_LIGHT_FRUSTUM_PLANE_BACK << R_POINT_LIGHT_FRUSTUM_PLANE2_SHIFT) |
                                      (R_POINT_LIGHT_FRUSTUM_PLANE_FRONT << R_POINT_LIGHT_FRUSTUM_PLANE3_SHIFT) |
                                      (R_POINT_LIGHT_FRUSTUM_PLANE_BACK << R_POINT_LIGHT_FRUSTUM_PLANE4_SHIFT) |
                                      (R_POINT_LIGHT_FRUSTUM_PLANE_FRONT << R_POINT_LIGHT_FRUSTUM_PLANE5_SHIFT);

    /* -Y */
    r_point_light_frustum_masks[3] = (R_POINT_LIGHT_FRUSTUM_PLANE_FRONT << R_POINT_LIGHT_FRUSTUM_PLANE2_SHIFT) |
                                      (R_POINT_LIGHT_FRUSTUM_PLANE_BACK << R_POINT_LIGHT_FRUSTUM_PLANE3_SHIFT) |
                                      (R_POINT_LIGHT_FRUSTUM_PLANE_FRONT << R_POINT_LIGHT_FRUSTUM_PLANE4_SHIFT) |
                                      (R_POINT_LIGHT_FRUSTUM_PLANE_BACK << R_POINT_LIGHT_FRUSTUM_PLANE5_SHIFT);

    /* +Z */
    r_point_light_frustum_masks[4] = (R_POINT_LIGHT_FRUSTUM_PLANE_FRONT << R_POINT_LIGHT_FRUSTUM_PLANE0_SHIFT) |
                                      (R_POINT_LIGHT_FRUSTUM_PLANE_FRONT << R_POINT_LIGHT_FRUSTUM_PLANE1_SHIFT) |
                                      (R_POINT_LIGHT_FRUSTUM_PLANE_FRONT << R_POINT_LIGHT_FRUSTUM_PLANE2_SHIFT) |
                                      (R_POINT_LIGHT_FRUSTUM_PLANE_FRONT << R_POINT_LIGHT_FRUSTUM_PLANE3_SHIFT);

    /* -Z */
    r_point_light_frustum_masks[5] = (R_POINT_LIGHT_FRUSTUM_PLANE_BACK << R_POINT_LIGHT_FRUSTUM_PLANE0_SHIFT) |
                                      (R_POINT_LIGHT_FRUSTUM_PLANE_BACK << R_POINT_LIGHT_FRUSTUM_PLANE1_SHIFT) |
                                      (R_POINT_LIGHT_FRUSTUM_PLANE_BACK << R_POINT_LIGHT_FRUSTUM_PLANE2_SHIFT) |
                                      (R_POINT_LIGHT_FRUSTUM_PLANE_BACK << R_POINT_LIGHT_FRUSTUM_PLANE3_SHIFT);


    struct r_framebuffer_desc_t framebuffer_desc;
    framebuffer_desc = (struct r_framebuffer_desc_t) {
        .color_attachments[0] = &(struct r_texture_desc_t){
            .format = R_FORMAT_RGBA8,
            .min_filter = GL_NEAREST,
            .mag_filter = GL_NEAREST
        },
        .depth_attachment = &(struct r_texture_desc_t){
            .format = R_FORMAT_DEPTH24_STENCIL8,
            .min_filter = GL_NEAREST,
            .mag_filter = GL_NEAREST
        },
        .width = r_width,
        .height = r_height
    };
//    r_main_framebuffer = r_CreateFramebuffer(r_width, r_height);
    r_main_framebuffer = r_CreateFramebuffer(&framebuffer_desc);
//    r_AddAttachment(r_main_framebuffer, GL_COLOR_ATTACHMENT0, R_FORMAT_RGBA8, GL_NEAREST, GL_NEAREST);
//    r_AddAttachment(r_main_framebuffer, GL_DEPTH_STENCIL_ATTACHMENT, R_FORMAT_DEPTH24_STENCIL8, GL_NEAREST, GL_NEAREST);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, r_main_framebuffer->handle);
    printf("%x\n", glCheckFramebufferStatus(GL_READ_FRAMEBUFFER));

//    r_debug_framebuffer = r_CreateFramebuffer(r_width, r_height);
//    r_AddAttachment(r_debug_framebuffer, GL_COLOR_ATTACHMENT0, GL_RGBA8, GL_NEAREST, GL_NEAREST);
//    r_AddAttachment(r_debug_framebuffer, GL_DEPTH_STENCIL_ATTACHMENT, GL_DEPTH24_STENCIL8, GL_NEAREST, GL_NEAREST);

//    r_ui_framebuffer = r_CreateFramebuffer(r_width, r_height);
//    r_AddAttachment(r_ui_framebuffer, GL_COLOR_ATTACHMENT0, GL_RGBA8, GL_NEAREST, GL_NEAREST);
    framebuffer_desc = (struct r_framebuffer_desc_t) {
        .color_attachments[0] = &(struct r_texture_desc_t){
            .format = R_FORMAT_RGBA8,
            .min_filter = GL_NEAREST,
            .mag_filter = GL_NEAREST
        },
        .width = r_width / 2,
        .height = r_height / 2
    };
//    r_volume_framebuffer = r_CreateFramebuffer(r_width / 2, r_height / 2);
    r_volume_framebuffer = r_CreateFramebuffer(&framebuffer_desc);
//    r_AddAttachment(r_volume_framebuffer, GL_COLOR_ATTACHMENT0, R_FORMAT_RGBA8, GL_NEAREST, GL_NEAREST);

//
//    glGenFramebuffers(1, &r_main_framebuffer);
//    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, r_main_framebuffer);
//    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, r_main_color_attachment, 0);
//    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, r_main_depth_attachment, 0);
//    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, r_main_depth_attachment, 0);

//    glGenFramebuffers(1, &r_z_prepass_framebuffer);
//    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, r_z_prepass_framebuffer);
//    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, r_main_framebuffer->depth_attachment->handle, 0);

    for(uint32_t angle = R_SPOT_LIGHT_MIN_ANGLE; angle <= R_SPOT_LIGHT_MAX_ANGLE; angle++)
    {
        float rad_angle = 3.14159265 * ((float)angle / 180.0);
        r_spot_light_tan_lut[angle - R_SPOT_LIGHT_MIN_ANGLE] = tanf(rad_angle);
        r_spot_light_cos_lut[angle - R_SPOT_LIGHT_MIN_ANGLE] = cosf(rad_angle);
    }

    r_renderer_state.use_z_prepass = 1;
    r_renderer_state.max_shadow_res = 8;
    r_renderer_state.draw_lights = 1;


    struct r_vert_t screen_tri[] =
    {
        {.pos = vec3_t_c(-10.0, -10.0, 0.0)},
        {.pos = vec3_t_c(10.0, -10.0, 0.0)},
        {.pos = vec3_t_c(0.0, 10.0, 0.0)},
    };

    r_screen_tri_chunk = r_AllocateVertices(3);
    r_FillVertices(r_screen_tri_chunk, screen_tri, 3);
    struct ds_chunk_t *tri_chunk = ds_get_chunk_pointer(&r_vertex_heap, r_screen_tri_chunk);
    r_screen_tri_start = tri_chunk->start / sizeof(struct r_vert_t);

    log_ScopedLogMessage(LOG_TYPE_NOTICE, "Renderer initialized!");
}

void r_Shutdown()
{
    log_ScopedLogMessage(LOG_TYPE_NOTICE, "Shutting down renderer...");
    log_ScopedLogMessage(LOG_TYPE_NOTICE, "Renderer shut down!");
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
    view
============================================================================
*/

struct r_view_t *r_CreateView(struct r_view_desc_t *view_desc)
{
    uint32_t index = ds_slist_add_element(&r_views, NULL);
    struct r_view_t *view = ds_slist_get_element(&r_views, index);

    view->index = index;
    view->projection_matrix = view_desc->projection_matrix;
    view->transform = view_desc->transform;
    view->framebuffer = view_desc->framebuffer;

    view->entity_cmd_buffer = r_AllocCmdBuffer(sizeof(struct r_entity_cmd_t));
    view->world_cmd_buffer = r_AllocCmdBuffer(sizeof(struct r_world_cmd_t));
    view->immediate_cmd_buffer = r_AllocImmediateCmdBuffer(sizeof(struct r_i_cmd_t));
    view->lights = ds_list_create(sizeof(struct r_light_t *), 512);

    return view;
}

void r_DestroyView(struct r_view_t *view)
{
    if(view && view->index != 0xffffffff)
    {
        r_FreeCmdBuffer(&view->entity_cmd_buffer);
        r_FreeCmdBuffer(&view->world_cmd_buffer);
        r_FreeImmediateCmdBuffer(&view->immediate_cmd_buffer);
        ds_list_destroy(&view->lights);
        ds_slist_remove_element(&r_views, view->index);
        view->index = 0xffffffff;
    }
}

/*
============================================================================
    cmd buffer
============================================================================
*/

struct r_cmd_buffer_t r_AllocCmdBuffer(uint32_t cmd_size)
{
    struct r_cmd_buffer_t cmd_buffer = {};
    cmd_buffer.cmds = ds_list_create(cmd_size, 8192);
//    cmd_buffer.data = ds_list_create(R_CMD_DATA_SLOT_SIZE, 8192);
    return cmd_buffer;
}

void r_ResetCmdBuffer(struct r_cmd_buffer_t *cmd_buffer)
{
    if(cmd_buffer)
    {
        cmd_buffer->cmds.cursor = 0;
    }
}

void r_FreeCmdBuffer(struct r_cmd_buffer_t *cmd_buffer)
{
    if(cmd_buffer && cmd_buffer->cmds.buffers)
    {
        r_ResetCmdBuffer(cmd_buffer);
        ds_list_destroy(&cmd_buffer->cmds);
        cmd_buffer->cmds.buffers = NULL;
    }
}

void *r_AllocCmd(struct r_cmd_buffer_t *cmd_buffer)
{
    uint32_t index = ds_list_add_element(&cmd_buffer->cmds, NULL);
    void *cmd = ds_list_get_element(&cmd_buffer->cmds, index);
//    cmd->data = NULL;
    return cmd;
}

struct r_i_cmd_buffer_t r_AllocImmediateCmdBuffer(uint32_t cmd_size)
{
    struct r_i_cmd_buffer_t cmd_buffer = {};
    cmd_buffer.base = r_AllocCmdBuffer(cmd_size);
//    cmd_buffer.cmds = ds_list_create(cmd_size, 8192);
    cmd_buffer.data = ds_list_create(R_I_CMD_DATA_SLOT_SIZE, 8192);
    return cmd_buffer;
}

void r_ResetImmediateCmdBuffer(struct r_i_cmd_buffer_t *cmd_buffer)
{
    if(cmd_buffer)
    {
        r_ResetCmdBuffer(&cmd_buffer->base);
        cmd_buffer->data.cursor = 0;
        cmd_buffer->draw_state = NULL;
        cmd_buffer->uniforms = NULL;
        cmd_buffer->dirty_shader = 0;

        if(cmd_buffer->big_allocs)
        {
            struct r_i_cmd_data_t *cmd_data = cmd_buffer->big_allocs;

            while(cmd_data)
            {
                struct r_i_cmd_data_t *next_cmd_data = cmd_data->next;
                mem_Free(cmd_data);
                cmd_data = next_cmd_data;
            }

            cmd_buffer->big_allocs = NULL;
        }
    }
}

void r_FreeImmediateCmdBuffer(struct r_i_cmd_buffer_t *cmd_buffer)
{
    if(cmd_buffer && cmd_buffer->base.cmds.buffers)
    {
        r_ResetImmediateCmdBuffer(cmd_buffer);
        r_FreeCmdBuffer(&cmd_buffer->base);
    }
}

//struct r_i_cmd_t *r_AllocImmediateCmd(struct r_i_cmd_buffer_t *cmd_buffer)
//{
//    return (struct r_i_cmd_t *)r_AllocCmd(&cmd_buffer->base);
//}

void r_IssueImmediateCmd(struct r_i_cmd_buffer_t *cmd_buffer, uint32_t type, void *data)
{
    struct r_i_cmd_t *cmd = (struct r_i_cmd_t *)r_AllocCmd(&cmd_buffer->base);
    cmd->id = r_immediate_cmd_id;
    cmd->type = type;
    cmd->data = data;
    r_immediate_cmd_id++;
}

void *r_AllocImmediateCmdData(struct r_i_cmd_buffer_t *cmd_buffer, uint32_t size)
{
    struct r_i_cmd_data_t *data;

    uint32_t required_size = R_I_CMD_DATA_SLOT_SIZE + size;

    if(required_size <= cmd_buffer->data.buffer_size)
    {
        uint32_t required_slots = required_size / cmd_buffer->data.elem_size;
        uint32_t available_slots = cmd_buffer->data.buffer_size - (cmd_buffer->data.cursor % cmd_buffer->data.buffer_size);

        if(required_size % R_I_CMD_DATA_SLOT_SIZE)
        {
            required_slots++;
        }

        if(required_slots > available_slots)
        {
             cmd_buffer->data.cursor += available_slots;
        }

        uint32_t data_index = ds_list_add_element(&cmd_buffer->data, NULL);
        data = ds_list_get_element(&cmd_buffer->data, data_index);
        required_slots--;
        cmd_buffer->data.cursor += required_slots;
    }
    else
    {
        data = mem_Calloc(1, required_size);
        data->next = cmd_buffer->big_allocs;
        cmd_buffer->big_allocs = data;
    }

    /* this is guaranteed to give a properly aligned address for any data type */
    void *user_data = (char *)data + R_I_CMD_DATA_SLOT_SIZE;
    memset(user_data, 0, size);

    return user_data;
}

//void r_BeginCmdBuffer(struct r_cmd_buffer_t *cmd_buffer)
//{
//    r_ResetCmdBuffer(cmd_buffer);
//}
//
//void r_EndCmdBuffer(struct r_cmd_buffer_t *cmd_buffer)
//{
//
//}

/*
============================================================================
    texture
============================================================================
*/

struct r_texture_t *r_LoadTexture(char *file_name)
{
    struct r_texture_t *texture = NULL;
    char full_path[PATH_MAX];

    strcpy(full_path, file_name);

    if(!strstr(full_path, "textures"))
    {
        ds_path_append_end("textures", full_path, full_path, PATH_MAX);
    }

    if(!strstr(full_path, ".png"))
    {
        ds_path_set_ext(full_path, "png", full_path, PATH_MAX);
    }

    if(!ds_file_exists(full_path))
    {
        log_ScopedLogMessage(LOG_TYPE_ERROR, "Couldn't load texture [%s]", full_path);
        return NULL;
    }

    int32_t width;
    int32_t height;
    int32_t channels;
    unsigned char *pixels = stbi_load(full_path, &width, &height, &channels, STBI_rgb_alpha);

    ds_path_get_end(full_path, full_path, PATH_MAX);
    ds_path_drop_ext(full_path, full_path, PATH_MAX);
    struct r_texture_desc_t texture_desc = {
        .format = R_FORMAT_RGBA8,
        .width = width,
        .height = height,
        .min_filter = GL_LINEAR,
        .mag_filter = GL_LINEAR,
        .max_level = 4,
    };
//    texture = r_CreateTexture(full_path, width, height, R_FORMAT_RGBA8, GL_LINEAR, GL_LINEAR, pixels);
    texture = r_CreateTexture(full_path, &texture_desc, pixels);
    mem_Free(pixels);

    log_ScopedLogMessage(LOG_TYPE_NOTICE, "Texture [%s] loaded!", full_path);

    return texture;
}

//void r_TextureDataTypeAndFormat(uint32_t format, uint32_t *data_type, uint32_t *data_format)
//{
//    switch(format)
//    {
//        case GL_RGBA8:
//            *data_type = GL_UNSIGNED_BYTE;
//            *data_format = GL_RGBA;
//        break;
//
//        case GL_RGBA16:
//            *data_type = GL_UNSIGNED_SHORT;
//            *data_format = GL_RGBA;
//        break;
//
//        case GL_RGBA16F:
//            *data_type = GL_FLOAT;
//            *data_format = GL_RGBA;
//        break;
//
//        case GL_RGBA32F:
//            *data_type = GL_FLOAT;
//            *data_format = GL_RGBA;
//        break;
//
//        case GL_RGB8:
//            *data_type = GL_UNSIGNED_BYTE;
//            *data_format = GL_RGB;
//        break;
//
//        case GL_RG8:
//
//        break;
//
//        case GL_RG8I:
//
//        break;
//
//        case GL_RG8UI:
//
//        break;
//
//        case GL_RG16:
//
//        break;
//
//        case GL_RG16F:
//
//        break;
//
////        case GL_RG
//
//        case GL_RED:
//            *data_type = GL_UNSIGNED_BYTE;
//            *data_format = GL_RED;
//        break;
//
//        case GL_DEPTH_COMPONENT16:
//            *data_type = GL_UNSIGNED_SHORT;
//            *data_format = GL_DEPTH_COMPONENT;
//        break;
//
//        case GL_DEPTH_COMPONENT32:
//            *data_type = GL_UNSIGNED_INT;
//            *data_format = GL_DEPTH_COMPONENT;
//        break;
//
//        case GL_DEPTH_COMPONENT32F:
//            *data_type = GL_FLOAT;
//            *data_format = GL_DEPTH_COMPONENT;
//        break;
//
//        case GL_DEPTH24_STENCIL8:
//            *data_type = GL_UNSIGNED_INT_24_8;
//            *data_format = GL_DEPTH_STENCIL;
//        break;
//    }
//}

//struct r_texture_t *r_CreateTexture(char *name, uint32_t width, uint32_t height, uint32_t format, uint32_t min_filter, uint32_t mag_filter, void *data)
struct r_texture_t* r_CreateTexture(char *name, struct r_texture_desc_t *desc, void *data)
{
    uint32_t texture_index;
    struct r_texture_t *texture;
    struct r_texture_desc_t *texture_desc;
//    uint32_t data_format;
//    uint32_t data_type;

    texture_index = ds_slist_add_element(&r_textures, NULL);
    texture = ds_slist_get_element(&r_textures, texture_index);

    ds_slist_add_element(&r_texture_descs, NULL);
    texture_desc = ds_slist_get_element(&r_texture_descs, texture_index);

    texture->index = texture_index;
//    texture->format = format_info-internal_format;
    texture->desc = texture_desc;

    *texture_desc = *desc;
    struct r_gl_format_info_t format_info = r_gl_format_info[texture_desc->format];



    if(name)
    {
        strncpy(texture->name, name, sizeof(texture->name));
    }

    if(!texture_desc->min_filter)
    {
        texture_desc->min_filter = GL_LINEAR_MIPMAP_LINEAR;
    }

    if(!texture_desc->mag_filter)
    {
        texture_desc->mag_filter = GL_LINEAR_MIPMAP_LINEAR;
    }

    if(!texture_desc->addr_s)
    {
        texture_desc->addr_s = GL_REPEAT;
    }

    if(!texture_desc->addr_t)
    {
        texture_desc->addr_t = GL_REPEAT;
    }

    while(glGetError() != GL_NO_ERROR);
    glGenTextures(1, &texture->handle);
    glBindTexture(GL_TEXTURE_2D, texture->handle);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, texture_desc->min_filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, texture_desc->mag_filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, texture_desc->base_level);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, texture_desc->max_level);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, texture_desc->addr_s);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, texture_desc->addr_t);
    glTexImage2D(GL_TEXTURE_2D, texture_desc->base_level, format_info.internal_format,
                 texture_desc->width, texture_desc->height, 0, format_info.data_format, format_info.type, data);

    if(texture_desc->max_level > texture_desc->base_level)
    {
        glGenerateMipmap(GL_TEXTURE_2D);
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    return texture;
}

void r_ResizeTexture(struct r_texture_t *texture, uint32_t width, uint32_t height)
{
    if(texture && texture->index != 0xffff)
    {
        struct r_gl_format_info_t format_info = r_gl_format_info[texture->desc->format];
        glBindTexture(GL_TEXTURE_2D, texture->handle);
        glTexImage2D(GL_TEXTURE_2D, 0, texture->desc->format, width, height, 0, format_info.data_format, format_info.type, NULL);
    }
}

struct r_texture_t *r_GetTexture(uint32_t index)
{
    struct r_texture_t *texture = NULL;

    texture = ds_slist_get_element(&r_textures, index);

    if(texture && texture->index == 0xffff)
    {
        texture = NULL;
    }

    return texture;
}

struct r_texture_t *r_FindTexture(char *name)
{
    for(uint32_t texture_index = 0; texture_index < r_textures.cursor; texture_index++)
    {
        struct r_texture_t *texture = r_GetTexture(texture_index);

        if(texture && !strcmp(texture->name, name))
        {
            return texture;
        }
    }

    return NULL;
}

void r_BindTexture(struct r_texture_t *texture, uint32_t tex_unit)
{
    if(texture && texture->index != 0xffff)
    {
        glActiveTexture(tex_unit);
        glBindTexture(GL_TEXTURE_2D, texture->handle);
    }
}

void r_DestroyTexture(struct r_texture_t *texture)
{
    if(texture && texture->index != 0xffff)
    {
        glDeleteTextures(1, &texture->handle);
        ds_slist_remove_element(&r_textures, texture->index);
        ds_slist_remove_element(&r_texture_descs, texture->index);
        texture->index = 0xffff;
    }
}

/*
============================================================================
    framebuffer
============================================================================
*/

//struct r_framebuffer_t *r_CreateFramebuffer(uint16_t width, uint16_t height)
struct r_framebuffer_t *r_CreateFramebuffer(struct r_framebuffer_desc_t *desc)
{
    uint32_t index = ds_slist_add_element(&r_framebuffers, NULL);
    struct r_framebuffer_t *framebuffer = ds_slist_get_element(&r_framebuffers, index);
    framebuffer->index = index;
    framebuffer->width = desc->width;
    framebuffer->height = desc->height;
    glGenFramebuffers(1, &framebuffer->handle);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer->handle);

//    for(uint32_t attachment_index = 0; attachment_index < R_MAX_COLOR_ATTACHMENTS; attachment_index++)
//    {
//        framebuffer->color_attachments[attachment_index] = NULL;
//    }

    uint32_t attachment_count = 0;
    for(uint32_t attachment_index = 0; attachment_index < R_MAX_COLOR_ATTACHMENTS; attachment_index++)
    {
        framebuffer->color_attachments[attachment_index] = NULL;

        if(desc->color_attachments[attachment_index])
        {
            struct r_texture_desc_t texture_desc = *desc->color_attachments[attachment_index];
            texture_desc.width = desc->width;
            texture_desc.height = desc->height;
            struct r_texture_t *texture = r_CreateTexture(NULL, &texture_desc, NULL);
            framebuffer->color_attachments[attachment_count] = texture;
            attachment_count++;
            glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + attachment_index, GL_TEXTURE_2D, texture->handle, 0);
        }
    }

    framebuffer->depth_attachment = NULL;

    if(desc->depth_attachment)
    {
        struct r_texture_desc_t texture_desc = *desc->depth_attachment;
        texture_desc.width = desc->width;
        texture_desc.height = desc->height;
        struct r_texture_t *texture = r_CreateTexture(NULL, &texture_desc, NULL);
        framebuffer->depth_attachment = texture;
        glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, texture->handle, 0);

        if(texture_desc.format == R_FORMAT_DEPTH24_STENCIL8)
        {
            glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, texture->handle, 0);
        }
    }

    return framebuffer;
}

struct r_framebuffer_t *r_GetActiveFramebuffer()
{
    return r_active_framebuffer;
}

struct r_framebuffer_t *r_GetFramebuffer(uint32_t index)
{
    struct r_framebuffer_t *framebuffer = ds_slist_get_element(&r_framebuffers, index);

    if(framebuffer && framebuffer->index == 0xffff)
    {
        framebuffer = NULL;
    }

    return framebuffer;
}

void r_DestroyFramebuffer(struct r_framebuffer_t *framebuffer)
{
    if(framebuffer && framebuffer->index != R_INVALID_FRAMEBUFFER_INDEX)
    {
        for(uint32_t index = 0; index < R_MAX_COLOR_ATTACHMENTS; index++)
        {
            if(framebuffer->color_attachments[index] == NULL)
            {
                r_DestroyTexture(framebuffer->color_attachments[index]);
                framebuffer->color_attachments[index] = NULL;
            }
        }

        if(framebuffer->depth_attachment == NULL)
        {
            r_DestroyTexture(framebuffer->depth_attachment);
        }

        glDeleteFramebuffers(1, &framebuffer->handle);

        framebuffer->index = R_INVALID_FRAMEBUFFER_INDEX;
    }
}

//void r_AddAttachment(struct r_framebuffer_t *framebuffer, uint16_t attachment, uint16_t format, uint16_t min_filter, uint16_t mag_filter)
//{
//    if(framebuffer && framebuffer->index != R_INVALID_FRAMEBUFFER_INDEX)
//    {
//        glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer->handle);
//        struct r_texture_t *texture;
//        uint32_t target_attachment;
//
//        struct r_texture_desc_t texture_desc = {
//            .format = format,
//            .width = framebuffer->width,
//            .height = framebuffer->height,
//            .min_filter = min_filter,
//            .mag_filter = mag_filter,
//        };
//
//        if(attachment == GL_DEPTH_ATTACHMENT || attachment == GL_DEPTH_STENCIL_ATTACHMENT)
//        {
//            if(framebuffer->depth_attachment)
//            {
//                r_DestroyTexture(framebuffer->depth_attachment);
//            }
////            framebuffer->depth_attachment = r_CreateTexture(NULL, framebuffer->width, framebuffer->height, format, GL_LINEAR, GL_LINEAR, NULL);
//            framebuffer->depth_attachment = r_CreateTexture(NULL, &texture_desc, NULL);
//            texture = framebuffer->depth_attachment;
//            target_attachment = GL_DEPTH_ATTACHMENT;
//        }
//        else
//        {
//            attachment -= GL_COLOR_ATTACHMENT0;
//
//            if(attachment >= R_MAX_COLOR_ATTACHMENTS)
//            {
//                return;
//            }
//
//            if(framebuffer->color_attachments[attachment])
//            {
//                r_DestroyTexture(framebuffer->color_attachments[attachment]);
//            }
//
////            framebuffer->color_attachments[attachment] = r_CreateTexture(NULL, framebuffer->width, framebuffer->height, format, min_filter, mag_filter, NULL);
//            framebuffer->color_attachments[attachment] = r_CreateTexture(NULL, &texture_desc, NULL);
//            texture = framebuffer->color_attachments[attachment];
//            target_attachment = GL_COLOR_ATTACHMENT0 + attachment;
//        }
//
//        if(target_attachment == GL_DEPTH_ATTACHMENT)
//        {
//            glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, texture->handle, 0);
//
//            if(format == R_FORMAT_DEPTH24_STENCIL8)
//            {
//                glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, texture->handle, 0);
//            }
//        }
//        else
//        {
//            glFramebufferTexture2D(GL_READ_FRAMEBUFFER, target_attachment, GL_TEXTURE_2D, texture->handle, 0);
//        }
//
//        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
//    }
//}

void r_SetAttachment(struct r_framebuffer_t *framebuffer, struct r_texture_t *texture, uint16_t attachment)
{
    if(framebuffer && framebuffer->index != 0xffff && texture && texture->index != 0xffff)
    {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer->handle);

        if(attachment == GL_DEPTH_ATTACHMENT || attachment == GL_STENCIL_ATTACHMENT)
        {
            framebuffer->depth_attachment = texture;
            glFramebufferTexture2D(GL_READ_FRAMEBUFFER, attachment, GL_TEXTURE_2D, texture->handle, 0);
        }
        else
        {
            uint16_t attachment_index = attachment - GL_COLOR_ATTACHMENT0;

            if(attachment_index < R_MAX_COLOR_ATTACHMENTS)
            {
                framebuffer->color_attachments[attachment_index] = texture;

//                if(attachment_index >= framebuffer->color_attachment_count)
//                {
//                    framebuffer->color_attachment_count = attachment_index + 1;
//                }

                glFramebufferTexture2D(GL_READ_FRAMEBUFFER, attachment, GL_TEXTURE_2D, texture->handle, 0);
            }
        }

        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    }
}

void r_ResizeFramebuffer(struct r_framebuffer_t *framebuffer, uint16_t width, uint16_t height)
{
    if(framebuffer && framebuffer->index != 0xffff)
    {
        if(framebuffer->width != width || framebuffer->height != height)
        {
//            for(uint32_t index = 0; index < framebuffer->color_attachment_count; index++)
//            {
//                r_ResizeTexture(framebuffer->color_attachments[index], width, height);
//            }

            if(framebuffer->depth_attachment)
            {
                r_ResizeTexture(framebuffer->depth_attachment, width, height);
            }

            framebuffer->width = width;
            framebuffer->height = height;
        }
    }
}

void r_BindFramebuffer(struct r_framebuffer_t *framebuffer)
{
    uint32_t handle = 0;
    uint32_t width = r_width;
    uint32_t height = r_height;

    if(framebuffer && framebuffer->index != 0xffff)
    {
        handle = framebuffer->handle;
        width = framebuffer->width;
        height = framebuffer->height;
    }

    glViewport(0, 0, width, height);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, handle);

    r_active_framebuffer = framebuffer;
}

void r_BlitFramebuffer(struct r_framebuffer_t *src_framebuffer, struct r_framebuffer_t *dst_framebuffer, uint32_t mask, uint32_t filter)
{
    uint32_t src_handle = 0;
    uint32_t src_width;
    uint32_t src_height;

    uint32_t dst_handle = 0;
    uint32_t dst_width;
    uint32_t dst_height;

    if(src_framebuffer && src_framebuffer->index != 0xffff)
    {
        src_handle = src_framebuffer->handle;
        src_width = src_framebuffer->width;
        src_height = src_framebuffer->height;
    }
    else
    {
        src_width = r_width;
        src_height = r_height;
    }

    if(dst_framebuffer && dst_framebuffer->index != 0xffff)
    {
        dst_handle = dst_framebuffer->handle;
        dst_width = dst_framebuffer->width;
        dst_height = dst_framebuffer->height;
    }
    else
    {
        dst_width = r_width;
        dst_height = r_height;
    }

    glViewport(0, 0, dst_width, dst_height);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, src_handle);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dst_handle);
    glBlitFramebuffer(0, 0, src_width, src_height, 0, 0, dst_width, dst_height, mask, filter);
}

void r_SampleFramebuffer(struct r_framebuffer_t *framebuffer, int32_t x, int32_t y, int32_t w, int32_t h, size_t size, void *data)
{
    struct r_texture_t *color_attachment = framebuffer->color_attachments[0];
    struct r_gl_format_info_t format_info = r_gl_format_info[color_attachment->desc->format];
    glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer->handle);
    glReadPixels(x, y, w, h, format_info.data_format, format_info.type, data);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
}

void r_PresentFramebuffer(struct r_framebuffer_t *framebuffer)
{
    if(framebuffer && framebuffer->index != 0xffff)
    {
        glDisable(GL_BLEND);
        glDisable(GL_STENCIL_TEST);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_SCISSOR_TEST);
        r_BlitFramebuffer(framebuffer, NULL, GL_COLOR_BUFFER_BIT, GL_LINEAR);
        SDL_GL_SwapWindow(r_window);
    }
}

/*
============================================================================
    material
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
//    material->name = strdup(name);
    strncpy(material->name, name, sizeof(material->name));
    material->diffuse_texture = diffuse_texture;
    material->normal_texture = normal_texture;
    material->roughness_texture = roughness_texture;
    material->height_texture = r_default_height_texture;

    return material;
}

struct r_material_t *r_GetMaterial(uint32_t index)
{
    struct r_material_t *material = NULL;

    material = ds_slist_get_element(&r_materials, index);

    if(material && material->index == 0xffffffff)
    {
        material = NULL;
    }

    return material;
}

struct r_material_t *r_FindMaterial(char *name)
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

void r_DestroyMaterial(struct r_material_t *material)
{
    if(material && material->index != 0xffffffff)
    {
        if(material != r_default_material)
        {
            ds_slist_remove_element(&r_materials, material->index);
            material->index = 0xffffffff;
        }
    }
}

void r_BindMaterial(struct r_material_t *material)
{
    if(material)
    {
        r_renderer_state.material_swaps++;

//        glActiveTexture(GL_TEXTURE0 + R_ALBEDO_TEX_UNIT);
//        glBindTexture(GL_TEXTURE_2D, material->diffuse_texture->handle);
        r_BindTexture(material->diffuse_texture, GL_TEXTURE0 + R_ALBEDO_TEX_UNIT);
        r_SetDefaultUniformI(R_UNIFORM_TEX_ALBEDO, R_ALBEDO_TEX_UNIT);

//        glActiveTexture(GL_TEXTURE0 + R_ROUGHNESS_TEX_UNIT);
//        glBindTexture(GL_TEXTURE_2D, material->roughness_texture->handle);
        r_BindTexture(material->roughness_texture, GL_TEXTURE0 + R_ROUGHNESS_TEX_UNIT);
        r_SetDefaultUniformI(R_UNIFORM_TEX_ROUGHNESS, R_ROUGHNESS_TEX_UNIT);

//        glActiveTexture(GL_TEXTURE0 + R_NORMAL_TEX_UNIT);
//        glBindTexture(GL_TEXTURE_2D, material->normal_texture->handle);
        r_BindTexture(material->normal_texture, GL_TEXTURE0 + R_NORMAL_TEX_UNIT);
        r_SetDefaultUniformI(R_UNIFORM_TEX_NORMAL, R_NORMAL_TEX_UNIT);
    }
}

/*
============================================================================
    model
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
    char full_path[PATH_MAX];

    strcpy(full_path, file_name);

    if(!strstr(full_path, "models"))
    {
        ds_path_append_end("models", full_path, full_path, PATH_MAX);
    }

    if(!strstr(full_path, ".mof"))
    {
        ds_path_set_ext(full_path, "mof", full_path, PATH_MAX);
    }

    if(ds_file_exists(full_path))
    {
        void *file_buffer;
        FILE *file = fopen(full_path, "rb");
        ds_file_read(file, &file_buffer, NULL);
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
            struct r_material_t *material = r_FindMaterial(batch_record->material);

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
                    diffuse_texture = r_FindTexture(material_record->diffuse_texture);

                    if(!diffuse_texture)
                    {
                        diffuse_texture = r_LoadTexture(material_record->diffuse_texture);
                    }
                }

                if(material_record->normal_texture[0])
                {
                    normal_texture = r_FindTexture(material_record->normal_texture);

                    if(!normal_texture)
                    {
                        normal_texture = r_LoadTexture(material_record->normal_texture);
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

        log_ScopedLogMessage(LOG_TYPE_NOTICE, "Model [%s] loaded!", full_path);

        ds_path_get_end(full_path, full_path, PATH_MAX);
        ds_path_drop_ext(full_path, full_path, PATH_MAX);

        if(skeleton)
        {
            skeleton_data.skeleton = skeleton;
            skeleton_data.weight_count = weight_count;
            skeleton_data.weights = weights;
            skeleton_data.weight_range_count = weight_range_count;
            skeleton_data.weight_ranges = weight_ranges;

            model = r_CreateModel(&geometry_data, &skeleton_data, full_path);
        }
        else
        {
            model = r_CreateModel(&geometry_data, NULL, full_path);
        }

        mem_Free(batches);
        mem_Free(file_buffer);
    }
    else
    {
        log_ScopedLogMessage(LOG_TYPE_ERROR, "Couldn't load model [%s]", full_path);
    }

    return model;
}

struct r_model_t *r_GetModel(uint32_t index)
{
    struct r_model_t *model = NULL;

    model = ds_slist_get_element(&r_models, index);

    if(model && model->index == 0xffffffff)
    {
        model = NULL;
    }

    return model;
}

struct r_model_t *r_CreateModel(struct r_model_geometry_t *geometry, struct r_model_skeleton_t *skeleton, char *name)
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

    size_t len = strlen(name) + 1;
    model->name = mem_Calloc(len, 1);
    strcpy(model->name, name);

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

struct r_model_t *r_FindModel(char *name)
{
    for(uint32_t model_index = 0; model_index < r_models.cursor; model_index++)
    {
        struct r_model_t *model = r_GetModel(model_index);

        if(model && !strcmp(model->name, name))
        {
            return model;
        }
    }

    return NULL;
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

    model->model_start = start;
    model->model_count = geometry->index_count;

    model->min = geometry->min;
    model->max = geometry->max;
}

struct r_model_t *r_ShallowCopyModel(struct r_model_t *base)
{
    struct r_model_geometry_t geometry = {};
    struct r_model_skeleton_t skeleton = {};

    struct ds_buffer_t batches = ds_buffer_create(sizeof(struct r_batch_t), base->batches.buffer_size);
    memcpy(batches.buffer, base->batches.buffer, sizeof(struct r_batch_t) * base->batches.buffer_size);

    for(uint32_t batch_index = 0; batch_index < base->batches.buffer_size; batch_index++)
    {
        struct r_batch_t *batch = ((struct r_batch_t *)batches.buffer) + batch_index;
        batch->start -= base->model_start;
    }

    geometry.batches = batches.buffer;
    geometry.batch_count = batches.buffer_size;
    geometry.indices = base->indices.buffer;
    geometry.index_count = base->indices.buffer_size;
    geometry.verts = base->verts.buffer;
    geometry.vert_count = base->verts.buffer_size;
    geometry.max = base->max;
    geometry.min = base->min;

    skeleton.skeleton = base->skeleton;
    skeleton.weights = base->weights.buffer;
    skeleton.weight_count = base->weights.buffer_size;
    skeleton.weight_ranges = base->weight_ranges.buffer;
    skeleton.weight_range_count = base->weight_ranges.buffer_size;

    struct r_model_t *copy = r_CreateModel(&geometry, &skeleton, "copy");
    copy->base = base;
//    struct r_model_t *copy;
//    uint32_t index = ds_slist_add_element(&r_models, NULL);
//    struct ds_chunk_t *chunk;
//    copy = ds_slist_get_element(&r_models, index);
//    memcpy(copy, base, sizeof(struct r_model_t));
//    copy->index = index;
//
//    copy->verts = ds_buffer_create(sizeof(struct r_vert_t), base->verts.buffer_size);
//    memcpy(copy->verts.buffer, base->verts.buffer, sizeof(struct r_vert_t) * copy->verts.buffer_size);

//    copy->vert_chunk = r_AllocateVertices(copy->verts.buffer_size);
//    r_FillVertices(copy->vert_chunk, copy->verts.buffer, copy->verts.buffer_size);
//    chunk = ds_get_chunk_pointer(&r_vertex_heap, copy->vert_chunk);
//    uint32_t new_start = chunk->start / sizeof(struct r_vert_t);
//
//    copy->index_chunk = r_AllocateIndices(copy->indices.buffer_size);
//    r_FillIndices(copy->index_chunk, copy->indices.buffer, copy->indices.buffer_size, new_start);
//    chunk = ds_get_chunk_pointer(&r_index_heap, copy->index_chunk);
//    new_start = chunk->start / sizeof(uint32_t);
//    chunk = ds_get_chunk_pointer(&r_index_heap, base->index_chunk);
//
//    copy->batches = ds_buffer_create(sizeof(struct r_batch_t), base->batches.buffer_size);
//    for(uint32_t batch_index = 0; batch_index < copy->batches.buffer_size; batch_index++)
//    {
//        ((struct r_batch_t *)copy->batches.buffer)[batch_index] = ((struct r_batch_t *)base->batches.buffer)[batch_index];
//        ((struct r_batch_t *)copy->batches.buffer)[batch_index].start += new_start;
//    }
//
//    copy->base = base;

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

        ds_slist_remove_element(&r_models, model->index);
        model->index = 0xffffffff;
    }
}

/*
============================================================================
    light
============================================================================
*/

struct r_light_t *r_CreateLight(uint32_t type, vec3_t *position, vec3_t *color, float radius, float energy)
{
    struct r_light_t *light;
    uint32_t index;

    index = ds_slist_add_element(&r_lights[type], NULL);
    light = ds_slist_get_element(&r_lights[type], index);

    light->index = index;
    light->type = type;
    light->position = *position;
    light->color = *color;
    light->range = radius;
    light->energy = energy;

//    switch(light->type)
//    {
//        case R_LIGHT_TYPE_POINT:
//        {
//            struct r_point_light_t *point_light = (struct r_point_light_t *)light;
//            point_light->shadow_maps[0] = R_INVALID_SHADOW_MAP_HANDLE;
//            point_light->shadow_maps[0] = R_INVALID_SHADOW_MAP_HANDLE;
//            point_light->shadow_maps[0] = R_INVALID_SHADOW_MAP_HANDLE;
//            point_light->shadow_maps[0] = R_INVALID_SHADOW_MAP_HANDLE;
//            point_light->shadow_maps[0] = R_INVALID_SHADOW_MAP_HANDLE;
//            point_light->shadow_maps[0] = R_INVALID_SHADOW_MAP_HANDLE;
//        }
//        break;
//    }

    r_AllocShadowMaps(light, 64);

    return light;
}

struct r_light_t *r_CopyLight(struct r_light_t *light)
{
    struct r_light_t *copy = NULL;
    if(light && light->index != 0xffffffff)
    {
        switch(light->type)
        {
            case R_LIGHT_TYPE_SPOT:
            {
                struct r_spot_light_t *spot_light = (struct r_spot_light_t *)light;
                copy = (struct r_light_t *)r_CreateSpotLight(&spot_light->position, &spot_light->color, &spot_light->orientation,
                                                             spot_light->range, spot_light->energy, spot_light->angle, spot_light->softness);
            }
            break;

            case R_LIGHT_TYPE_POINT:
            {
                struct r_point_light_t *point_light = (struct r_point_light_t *)light;
                copy = (struct r_light_t *)r_CreatePointLight(&point_light->position, &point_light->color, point_light->range, point_light->energy);
            }
            break;
        }
    }
    return copy;
}

struct r_point_light_t *r_CreatePointLight(vec3_t *position, vec3_t *color, float radius, float energy)
{
    struct r_point_light_t *light = (struct r_point_light_t *)r_CreateLight(R_LIGHT_TYPE_POINT, position, color, radius, energy);
    return light;
}

struct r_spot_light_t *r_CreateSpotLight(vec3_t *position, vec3_t *color, mat3_t *orientation, float radius, float energy, uint32_t angle, float softness)
{
    struct r_spot_light_t *light = (struct r_spot_light_t *)r_CreateLight(R_LIGHT_TYPE_SPOT, position, color, radius, energy);

    if(angle > R_SPOT_LIGHT_MAX_ANGLE)
    {
        angle = R_SPOT_LIGHT_MAX_ANGLE;
    }
    else if(angle < R_SPOT_LIGHT_MIN_ANGLE)
    {
        angle = R_SPOT_LIGHT_MIN_ANGLE;
    }

    if(orientation)
    {
        light->orientation = *orientation;
    }
    else
    {
        mat3_t_identity(&light->orientation);
    }

    light->angle = angle;
    light->softness = softness;
    return light;
}

void r_AllocShadowMaps(struct r_light_t *light, uint32_t resolution)
{
    if(light && light->index != 0xffffffff)
    {
        if(resolution < R_SHADOW_MAP_MIN_RESOLUTION)
        {
            resolution = R_SHADOW_MAP_MIN_RESOLUTION;
        }
        else if(resolution > R_SHADOW_MAP_MAX_RESOLUTION)
        {
            resolution = R_SHADOW_MAP_MAX_RESOLUTION;
        }

        uint32_t start_bucket_index = R_SHADOW_BUCKET0;
        uint32_t test_res = R_SHADOW_BUCKET0_RES;

        for(; start_bucket_index < R_SHADOW_BUCKET4; start_bucket_index++)
        {
            if(test_res >= resolution)
            {
                break;
            }

            test_res <<= 1;
        }

        uint32_t *shadow_maps;
        uint32_t shadow_map_count;

        switch(light->type)
        {
            case R_LIGHT_TYPE_POINT:
            {
                struct r_point_light_t *point_light = (struct r_point_light_t *)light;
                shadow_maps = point_light->shadow_maps;
                shadow_map_count = 6;
            }
            break;

            case R_LIGHT_TYPE_SPOT:
            {
                struct r_spot_light_t *spot_light = (struct r_spot_light_t *)light;
                shadow_maps = &spot_light->shadow_map;
                shadow_map_count = 1;
            }
            break;
        }

        light->shadow_map_res = resolution;

        for(uint32_t shadow_map_index = 0; shadow_map_index < shadow_map_count; shadow_map_index++)
        {
            for(uint32_t src_bucket_index = start_bucket_index; src_bucket_index <= R_SHADOW_BUCKET4; src_bucket_index++)
            {
                struct r_shadow_bucket_t *src_bucket = r_shadow_buckets + src_bucket_index;

                if(src_bucket->cur_src != 0xffff)
                {
                    /* found a bucket that contains available shadow maps */

                    uint32_t src_tile_index = src_bucket->cur_src;
                    struct r_shadow_tile_t *src_tile = src_bucket->tiles + src_tile_index;
                    uint32_t src_shadow_map_index = 0;
                    uint32_t used_mask = 1;

                    for(; src_shadow_map_index < 4; src_shadow_map_index++)
                    {
                        if(!(src_tile->used & used_mask))
                        {
                            src_tile->used |= used_mask;
                            break;
                        }

                        used_mask <<= 1;
                    }

                    struct r_shadow_map_t *src_shadow_map = src_tile->shadow_maps + src_shadow_map_index;

                    if(src_tile->used == 0xf)
                    {
                        /* tile exhausted, unlink it from the bucket */
                        src_bucket->cur_src = src_tile->next;

                        if(src_tile->next != 0xffff)
                        {
                            src_bucket->tiles[src_tile->next].prev = src_tile->prev;
                        }
                    }

                    while(src_bucket_index > start_bucket_index)
                    {
                        /* we're sourcing the shadow map from a bigger bucket, so go down the bucket
                        sizes, splitting the shadow map and filling them in the way */

                        src_bucket_index--;
                        /* child bucket will have a new tile created for it, filled with 4 shadow maps.
                        If this child bucket isn't the bucket for the size we requested, one of the just
                        created shadow maps will be used to create a new tile for a bucket smaller than it */
                        struct r_shadow_bucket_t *child_bucket = r_shadow_buckets + src_bucket_index;
                        struct r_shadow_tile_t *child_tile = child_bucket->tiles + child_bucket->cur_free;
                        child_tile->parent_tile = src_tile_index;
                        src_tile_index = child_bucket->cur_free;

                        child_bucket->cur_free = child_tile->next;
                        if(child_bucket->cur_free != 0xffff)
                        {
                            child_bucket->tiles[child_bucket->cur_free].prev = 0xffff;
                        }

                        child_tile->next = child_bucket->cur_src;
                        if(child_bucket->cur_src != 0xffff)
                        {
                            child_bucket->tiles[child_bucket->cur_src].prev = src_tile_index;
                        }
                        child_bucket->cur_src = src_tile_index;

                        /* lsb set, first shadow map used */
                        child_tile->used = 1;
                        src_shadow_map_index = 0;

//                        uint32_t x_coord = R_SHADOW_MAP_X_COORD(src_shadow_map->coords);
//                        uint32_t y_coord = R_SHADOW_MAP_Y_COORD(src_shadow_map->coords);
                        uint32_t x_coord = src_shadow_map->x_coord;
                        uint32_t y_coord = src_shadow_map->y_coord;
                        uint32_t resolution = R_SHADOW_BUCKET0_RES << src_bucket_index;

                        src_tile = child_tile;
                        src_shadow_map = src_tile->shadow_maps + 3;
                        src_shadow_map->x_coord = x_coord + resolution;
                        src_shadow_map->y_coord = y_coord + resolution;
//                        src_shadow_map->coords = R_SHADOW_MAP_COORDS(x_coord + resolution, y_coord + resolution);

                        src_shadow_map = src_tile->shadow_maps + 2;
//                        src_shadow_map->coords = R_SHADOW_MAP_COORDS(x_coord, y_coord + resolution);
                        src_shadow_map->x_coord = x_coord;
                        src_shadow_map->y_coord = y_coord + resolution;

                        src_shadow_map = src_tile->shadow_maps + 1;
//                        src_shadow_map->coords = R_SHADOW_MAP_COORDS(x_coord + resolution, y_coord);
                        src_shadow_map->x_coord = x_coord + resolution;
                        src_shadow_map->y_coord = y_coord;

                        src_shadow_map = src_tile->shadow_maps;
//                        src_shadow_map->coords = R_SHADOW_MAP_COORDS(x_coord, y_coord);
                        src_shadow_map->x_coord = x_coord;
                        src_shadow_map->y_coord = y_coord;
                    }

//                    uint32_t x_coord = R_SHADOW_MAP_X_COORD(src_shadow_map->coords);
//                    uint32_t y_coord = R_SHADOW_MAP_Y_COORD(src_shadow_map->coords);
//
//                    printf("allocated shadow map at %d %d\n", x_coord, y_coord);

                    shadow_maps[shadow_map_index] = R_SHADOW_MAP_HANDLE(src_bucket_index, src_tile_index, src_shadow_map_index);

                    break;
                }
            }
        }
    }
}

struct r_shadow_map_t *r_GetShadowMap(uint32_t shadow_map)
{
    uint32_t bucket_index = R_SHADOW_BUCKET_INDEX(shadow_map);
    uint32_t tile_index = R_SHADOW_TILE_INDEX(shadow_map);
    uint32_t map_index = R_SHADOW_MAP_INDEX(shadow_map);
    struct r_shadow_map_t *map = &r_shadow_buckets[bucket_index].tiles[tile_index].shadow_maps[map_index];
    return map;
}

void r_FreeShadowMaps(struct r_light_t *light)
{
    uint32_t *shadow_maps;
    uint32_t shadow_map_count;

    if(light && light->index != 0xffffffff)
    {
        switch(light->type)
        {
            case R_LIGHT_TYPE_POINT:
            {
                struct r_point_light_t *point_light = (struct r_point_light_t *)light;
                shadow_maps = point_light->shadow_maps;
                shadow_map_count = 6;
            }
            break;


            case R_LIGHT_TYPE_SPOT:
            {
                struct r_spot_light_t *spot_light = (struct r_spot_light_t *)light;
                shadow_maps = &spot_light->shadow_map;
                shadow_map_count = 1;
            }
            break;
        }

        for(uint32_t shadow_map_index = 0; shadow_map_index < shadow_map_count; shadow_map_index++)
        {
            uint32_t shadow_map_handle = shadow_maps[shadow_map_index];

            uint32_t bucket_index = R_SHADOW_BUCKET_INDEX(shadow_map_handle);
            uint32_t tile_index = R_SHADOW_TILE_INDEX(shadow_map_handle);
            uint32_t map_index = R_SHADOW_MAP_INDEX(shadow_map_handle);

            struct r_shadow_bucket_t *bucket = r_shadow_buckets + bucket_index;
            struct r_shadow_tile_t *tile = bucket->tiles + tile_index;
            uint32_t used_mask = 1 << map_index;

            while(tile->used & used_mask)
            {
                uint32_t next_used = tile->used & (~used_mask);

                if(tile->used == 0xf)
                {
                    /* tile isn't linked in the source linked list, so link it here */
                    tile->next = bucket->cur_src;

                    if(bucket->cur_src != 0xffff)
                    {
                        bucket->tiles[bucket->cur_src].prev = tile_index;
                    }

                    bucket->cur_src = tile_index;
                }
                else if(next_used == 0 && bucket_index != R_SHADOW_BUCKET4)
                {
                    /* this tile has all shadow maps unused, so return it to its parent
                    tile */

                    if(tile->prev == 0xffff)
                    {
                        bucket->cur_src = tile->next;
                    }
                    else
                    {
                        bucket->tiles[tile->prev].next = tile->next;
                    }

                    if(tile->next != 0xffff)
                    {
                        bucket->tiles[tile->next].prev = tile->prev;
                    }

                    tile->next = bucket->cur_free;
                    tile->prev = 0xffff;

                    if(bucket->cur_free != 0xffff)
                    {
                        bucket->tiles[bucket->cur_free].prev = tile_index;
                    }

                    bucket->cur_free = tile_index;

                    struct r_shadow_map_t *shadow_map = tile->shadow_maps;

                    bucket_index++;
                    bucket = r_shadow_buckets + bucket_index;
                    tile_index = tile->parent_tile;
                    tile = bucket->tiles + tile_index;

                    for(map_index = 0; map_index < 4; map_index++)
                    {
                        struct r_shadow_map_t *parent_shadow_map = tile->shadow_maps + map_index;
                        if(parent_shadow_map->x_coord == shadow_map->x_coord &&
                           parent_shadow_map->y_coord == shadow_map->y_coord)
                        {
                            break;
                        }
//                        if(parent_shadow_map->coords == shadow_map->coords)
//                        {
//                            break;
//                        }
                    }

                    used_mask = 1 << map_index;
                    continue;
                }

                tile->used = next_used;
            }
        }
    }
}

struct r_light_t *r_GetLight(uint32_t light_index)
{
    uint32_t type = (light_index >> R_LIGHT_TYPE_INDEX_SHIFT) & R_LIGHT_TYPE_INDEX_MASK;
    struct r_light_t *light = ds_slist_get_element(&r_lights[type], light_index & R_LIGHT_INDEX_MASK);

    if(light && light->index == 0xffff)
    {
        light = NULL;
    }

    return light;
}

void r_DestroyLight(struct r_light_t *light)
{
    if(light && light->index != 0xffff)
    {
        r_FreeShadowMaps(light);
        ds_slist_remove_element(&r_lights[light->type] , light->index);
        light->index = 0xffff;
    }
}

void r_DestroyAllLighs()
{
    for(uint32_t light_index = 0; light_index < r_lights[R_LIGHT_TYPE_POINT].cursor; light_index++)
    {
        struct r_light_t *light = r_GetLight(R_LIGHT_INDEX(R_LIGHT_TYPE_POINT, light_index));

        if(light)
        {
            r_DestroyLight(light);
        }
    }

    for(uint32_t light_index = 0; light_index < r_lights[R_LIGHT_TYPE_SPOT].cursor; light_index++)
    {
        struct r_light_t *light = r_GetLight(R_LIGHT_INDEX(R_LIGHT_TYPE_SPOT, light_index));

        if(light)
        {
            r_DestroyLight(light);
        }
    }
}

/*
============================================================================
    shader
============================================================================
*/

struct r_shader_t *r_CreateShader(struct r_shader_desc_t *desc)
{
    int compilation_status = 0;
    int info_log_length = 0;
    char *info_log = NULL;

    uint32_t vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, (const GLchar * const *)&desc->vertex_code, NULL);
    glCompileShader(vertex_shader);
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &compilation_status);

    if(!compilation_status)
    {
        glGetShaderiv(vertex_shader, GL_INFO_LOG_LENGTH, &info_log_length);
        info_log = mem_Calloc(1, info_log_length);
        glGetShaderInfoLog(vertex_shader, info_log_length, NULL, info_log);
        log_ScopedLogMessage(LOG_TYPE_ERROR, "Shader [%s] vertex stage compilation failed!\n Info log: %s", desc->name, info_log);
        mem_Free(info_log);
        glDeleteShader(vertex_shader);
        return NULL;
    }

    uint32_t fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, (const GLchar * const *)&desc->fragment_code, NULL);
    glCompileShader(fragment_shader);
    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &compilation_status);
    if(!compilation_status)
    {
        glGetShaderiv(fragment_shader, GL_INFO_LOG_LENGTH, &info_log_length);
        info_log = mem_Calloc(1, info_log_length);
        glGetShaderInfoLog(fragment_shader, info_log_length, NULL, info_log);
        log_ScopedLogMessage(LOG_TYPE_ERROR, "Shader [%s] fragment stage compilation failed!\n Info log: %s", desc->name, info_log);
        mem_Free(info_log);
        glDeleteShader(vertex_shader);
        glDeleteShader(fragment_shader);
        return NULL;
    }

    uint32_t shader_program = glCreateProgram();
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
        log_ScopedLogMessage(LOG_TYPE_ERROR, "Program linking for shader [%s] failed!\n Info log: %s", desc->name, info_log);
        mem_Free(info_log);
        glDeleteProgram(shader_program);
        return NULL;
    }

    uint32_t index = ds_slist_add_element(&r_shaders, NULL);
    struct r_shader_t *shader = ds_slist_get_element(&r_shaders, index);
    shader->index = index;
    shader->handle = shader_program;

//    for(uint32_t attrib_index = 0; attrib_index < desc->attrib_count; attrib_index++)
//    {
//        struct r_vertex_attrib_t *attrib = desc->vertex_attribs + attrib_index;
//        glBindAttribLocation(shader_program, attrib->location, attrib->name);
//    }

    struct r_vertex_layout_t *vertex_layout = desc->vertex_layout;
//    shader->vertex_layout.attrib_count = vertex_layout->attrib_count;
    shader->vertex_layout.stride = vertex_layout->stride;
    shader->vertex_layout.attribs = mem_Calloc(vertex_layout->attrib_count, sizeof(struct r_vertex_attrib_t));

    memcpy(shader->vertex_layout.attribs, vertex_layout->attribs, sizeof(struct r_vertex_attrib_t) * vertex_layout->attrib_count);

    for(uint32_t attrib_index = 0; attrib_index < vertex_layout->attrib_count; attrib_index++)
    {
        struct r_vertex_attrib_t *desc_attrib = vertex_layout->attribs + attrib_index;
        struct r_vertex_attrib_t *shader_attrib = shader->vertex_layout.attribs + shader->vertex_layout.attrib_count;
        shader_attrib->location = glGetAttribLocation(shader->handle, desc_attrib->name);
        if(shader_attrib->location != GL_INVALID_INDEX)
        {
            shader_attrib->format = desc_attrib->format;
            shader_attrib->normalized = desc_attrib->normalized;
            shader_attrib->offset = desc_attrib->offset;
            shader->vertex_layout.attrib_count++;
        }
    }

    shader->uniforms = mem_Calloc(desc->uniform_count, sizeof(struct r_uniform_t));
    shader->uniform_count = desc->uniform_count;
    memcpy(shader->uniforms, desc->uniforms, sizeof(struct r_uniform_t) * desc->uniform_count);

    for(uint32_t uniform_index = 0; uniform_index < desc->uniform_count; uniform_index++)
    {
        struct r_uniform_t *desc_uniform = desc->uniforms + uniform_index;
        struct r_uniform_t *shader_uniform = shader->uniforms + uniform_index;
        shader_uniform->location = glGetUniformLocation(shader->handle, desc_uniform->name);
    }

    /* TODO: refactor this into a better API. It should be possible to pass
    this kind of stuff in a shader description. */
    uint32_t point_light_uniform_block = glGetProgramResourceIndex(shader->handle, GL_SHADER_STORAGE_BLOCK, "r_point_lights");
    uint32_t spot_light_uniform_block = glGetProgramResourceIndex(shader->handle, GL_SHADER_STORAGE_BLOCK, "r_spot_lights");
    uint32_t light_indices_uniform_block = glGetProgramResourceIndex(shader->handle, GL_SHADER_STORAGE_BLOCK, "r_light_indices");
    uint32_t shadow_indices_uniform_block = glGetProgramResourceIndex(shader->handle, GL_SHADER_STORAGE_BLOCK, "r_shadow_maps");

    if(point_light_uniform_block != 0xffffffff)
    {
        glShaderStorageBlockBinding(shader->handle, point_light_uniform_block, R_POINT_LIGHT_UNIFORM_BUFFER_BINDING);
    }

    if(spot_light_uniform_block != 0xffffffff)
    {
        glShaderStorageBlockBinding(shader->handle, spot_light_uniform_block, R_SPOT_LIGHT_UNIFORM_BUFFER_BINDING);
    }

    if(light_indices_uniform_block != 0xffffffff)
    {
        glShaderStorageBlockBinding(shader->handle, light_indices_uniform_block, R_LIGHT_INDICES_UNIFORM_BUFFER_BINDING);
    }

    if(shadow_indices_uniform_block != 0xffffffff)
    {
        glShaderStorageBlockBinding(shader->handle, shadow_indices_uniform_block, R_SHADOW_INDICES_BUFFER_BINDING);
    }

    return shader;
}

struct r_shader_t *r_LoadShader(struct r_shader_desc_t *desc)
{
    FILE *shader_file;
//    uint32_t vertex_shader;
//    uint32_t fragment_shader;
//    uint32_t shader_program;
    char *shader_source;
    struct r_shader_t *shader = NULL;
//    int32_t compilation_status;
//    int32_t info_log_length;
//    char *info_log;
    char include_error[256];
    char vertex_full_path[PATH_MAX];
    char fragment_full_path[PATH_MAX];
    struct r_shader_desc_t shader_desc = {
        .uniform_count = desc->uniform_count,
        .uniforms = desc->uniforms,
        .vertex_layout = desc->vertex_layout
    };

    strcpy(vertex_full_path, desc->vertex_code);
    strcpy(fragment_full_path, desc->fragment_code);

    if(!strstr(vertex_full_path, "shaders"))
    {
        ds_path_append_end("shaders", vertex_full_path, vertex_full_path, PATH_MAX);
    }

    if(!strstr(vertex_full_path, ".vert"))
    {
        ds_path_set_ext(vertex_full_path, "vert", vertex_full_path, PATH_MAX);
    }

    if(ds_file_exists(vertex_full_path))
    {
        if(!strstr(fragment_full_path, "shaders"))
        {
            ds_path_append_end("shaders", fragment_full_path, fragment_full_path, PATH_MAX);
        }

        if(!strstr(fragment_full_path, ".frag"))
        {
            ds_path_set_ext(fragment_full_path, "frag", fragment_full_path, PATH_MAX);
        }

        if(ds_file_exists(fragment_full_path))
        {
            shader_file = fopen(vertex_full_path, "rb");
            ds_file_read(shader_file, (void **)&shader_source, NULL);
            fclose(shader_file);

            shader_desc.vertex_code = stb_include_string(shader_source, NULL, "shaders", NULL, include_error);
            mem_Free(shader_source);

            if(shader_desc.vertex_code)
            {
                shader_file = fopen(fragment_full_path, "rb");
                ds_file_read(shader_file, (void **)&shader_source, NULL);
                fclose(shader_file);
                shader_desc.fragment_code = stb_include_string(shader_source, NULL, "shaders", NULL, include_error);
                mem_Free(shader_source);

                if(shader_desc.fragment_code)
                {
                    shader = r_CreateShader(&shader_desc);

                    if(shader)
                    {
                        log_ScopedLogMessage(LOG_TYPE_NOTICE, "Shaders [%s] and [%s] loaded!", vertex_full_path, fragment_full_path);
                    }
                }
                else
                {
                    log_ScopedLogMessage(LOG_TYPE_ERROR, "Preprocessor error: %s", include_error);
                }

                mem_Free(shader_desc.fragment_code);
            }
            else
            {
                log_ScopedLogMessage(LOG_TYPE_ERROR, "Preprocessor error: %s", include_error);
            }

            mem_Free(shader_desc.vertex_code);
        }
        else
        {
            log_ScopedLogMessage(LOG_TYPE_ERROR, "Couldn't load fragment shader %s", fragment_full_path);
        }
    }
    else
    {
        log_ScopedLogMessage(LOG_TYPE_ERROR, "Couldn't load vertex shader %s", vertex_full_path);
    }


//    vertex_shader = glCreateShader(GL_VERTEX_SHADER);
//    glShaderSource(vertex_shader, 1, (const GLchar * const *)&preprocessed_shader_source, NULL);
//    free(preprocessed_shader_source);
//    mem_Free(shader_source);
//    glCompileShader(vertex_shader);
//    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &compilation_status);

//    if(!compilation_status)
//    {
//        glGetShaderiv(vertex_shader, GL_INFO_LOG_LENGTH, &info_log_length);
//        info_log = mem_Calloc(1, info_log_length);
//        glGetShaderInfoLog(vertex_shader, info_log_length, NULL, info_log);
//        log_ScopedLogMessage(LOG_TYPE_ERROR, "Shader [%s] vertex stage compilation failed!\n Info log: %s", vertex_full_path, info_log);
//        mem_Free(info_log);
//        glDeleteShader(vertex_shader);
//        return NULL;
//    }




//    fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
//    glShaderSource(fragment_shader, 1, (const GLchar * const *)&preprocessed_shader_source, NULL);
//    free(preprocessed_shader_source);
//    mem_Free(shader_source);
//    glCompileShader(fragment_shader);
//    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &compilation_status);
//    if(!compilation_status)
//    {
//        glGetShaderiv(fragment_shader, GL_INFO_LOG_LENGTH, &info_log_length);
//        info_log = mem_Calloc(1, info_log_length);
//        glGetShaderInfoLog(fragment_shader, info_log_length, NULL, info_log);
//        log_ScopedLogMessage(LOG_TYPE_ERROR, "Shader [%s] fragment stage compilation failed!\n Info log: %s", fragment_full_path, info_log);
//        mem_Free(info_log);
//        glDeleteShader(vertex_shader);
//        glDeleteShader(fragment_shader);
//        return NULL;
//    }


//    shader_program = glCreateProgram();
//    glAttachShader(shader_program, vertex_shader);
//    glAttachShader(shader_program, fragment_shader);
//    glLinkProgram(shader_program);
//    glDeleteShader(vertex_shader);
//    glDeleteShader(fragment_shader);
//    glGetProgramiv(shader_program, GL_LINK_STATUS, &compilation_status);
//    if(!compilation_status)
//    {
//        glGetProgramiv(shader_program, GL_INFO_LOG_LENGTH, &info_log_length);
//        info_log = mem_Calloc(1, info_log_length);
//        glGetProgramInfoLog(shader_program, info_log_length, NULL, info_log);
//        log_ScopedLogMessage(LOG_TYPE_ERROR, "Program linking for shaders [%s] and [%s] failed!\n Info log: %s", vertex_full_path, fragment_full_path, info_log);
////        printf("r_LoadShader: program linking failed for shaders %s and %s!\n", vertex_full_path, fragment_full_path);
////        printf("info log:\n %s\n", info_log);
//        mem_Free(info_log);
//        glDeleteProgram(shader_program);
//        return NULL;
//    }

//    uint32_t shader_index = ds_slist_add_element(&r_shaders, NULL);
//    shader = ds_slist_get_element(&r_shaders, shader_index);
//    shader->index = shader_index;
//    shader->handle = shader_program;
//
//    for(uint32_t uniform_index = 0; uniform_index < R_UNIFORM_LAST; uniform_index++)
//    {
//        shader->uniforms[uniform_index].location = glGetUniformLocation(shader_program, r_default_uniforms[uniform_index].name);
//    }

//    uint32_t point_light_uniform_block = glGetProgramResourceIndex(shader_program, GL_SHADER_STORAGE_BLOCK, "r_point_lights");
//    uint32_t spot_light_uniform_block = glGetProgramResourceIndex(shader_program, GL_SHADER_STORAGE_BLOCK, "r_spot_lights");
//    uint32_t light_indices_uniform_block = glGetProgramResourceIndex(shader_program, GL_SHADER_STORAGE_BLOCK, "r_light_indices");
//    uint32_t shadow_indices_uniform_block = glGetProgramResourceIndex(shader_program, GL_SHADER_STORAGE_BLOCK, "r_shadow_maps");
//
//    if(point_light_uniform_block != 0xffffffff)
//    {
//        glShaderStorageBlockBinding(shader_program, point_light_uniform_block, R_POINT_LIGHT_UNIFORM_BUFFER_BINDING);
//    }
//
//    if(spot_light_uniform_block != 0xffffffff)
//    {
//        glShaderStorageBlockBinding(shader_program, spot_light_uniform_block, R_SPOT_LIGHT_UNIFORM_BUFFER_BINDING);
//    }
//
//    if(light_indices_uniform_block != 0xffffffff)
//    {
//        glShaderStorageBlockBinding(shader_program, light_indices_uniform_block, R_LIGHT_INDICES_UNIFORM_BUFFER_BINDING);
//    }
//
//    if(shadow_indices_uniform_block != 0xffffffff)
//    {
//        glShaderStorageBlockBinding(shader_program, shadow_indices_uniform_block, R_SHADOW_INDICES_BUFFER_BINDING);
//    }

//    shader->attribs = 0;
//
//    if(glGetAttribLocation(shader_program, "r_position") != -1)
//    {
//        shader->attribs |= R_ATTRIB_POSITION;
//        glBindAttribLocation(shader_program, R_POSITION_LOCATION, "r_position");
//    }
//
//    if(glGetAttribLocation(shader_program, "r_tex_coords") != -1)
//    {
//        shader->attribs |= R_ATTRIB_TEX_COORDS;
//        glBindAttribLocation(shader_program, R_TEX_COORDS_LOCATION, "r_tex_coords");
//    }
//
//    if(glGetAttribLocation(shader_program, "r_normal") != -1)
//    {
//        shader->attribs |= R_ATTRIB_NORMAL;
//        glBindAttribLocation(shader_program, R_NORMAL_LOCATION, "r_normal");
//    }
//
//    if(glGetAttribLocation(shader_program, "r_tangent") != -1)
//    {
//        shader->attribs |= R_ATTRIB_TANGENT;
//        glBindAttribLocation(shader_program, R_TANGENT_LOCATION, "r_tangent");
//    }
//
//    if(glGetAttribLocation(shader_program, "r_color") != -1)
//    {
//        shader->attribs |= R_ATTRIB_COLOR;
//        glBindAttribLocation(shader_program, R_COLOR_LOCATION, "r_color");
//    }

//    shader->vertex_layout = &r_default_vertex_layout;

    return shader;
}

struct r_shader_t *r_GetDefaultShader(uint32_t shader)
{
    return NULL;
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
        r_renderer_state.shader_swaps++;

        r_current_shader = shader;
        glUseProgram(shader->handle);

        if(r_active_framebuffer)
        {
            r_SetDefaultUniformI(R_UNIFORM_WIDTH, r_active_framebuffer->width);
            r_SetDefaultUniformI(R_UNIFORM_HEIGHT, r_active_framebuffer->height);
        }
        else
        {
            r_SetDefaultUniformI(R_UNIFORM_WIDTH, r_width);
            r_SetDefaultUniformI(R_UNIFORM_HEIGHT, r_height);
        }
        r_SetDefaultUniformF(R_UNIFORM_Z_NEAR, r_z_near);
        r_SetDefaultUniformF(R_UNIFORM_CLUSTER_DENOM, r_denom);

        r_BindVertexLayout(&shader->vertex_layout);
    }
}

void r_BindVertexLayout(struct r_vertex_layout_t *layout)
{
    if(layout && layout->attrib_count)
    {
        for(uint32_t attrib_index = 0; attrib_index < layout->attrib_count; attrib_index++)
        {
            struct r_vertex_attrib_t *attrib = layout->attribs + attrib_index;
            struct r_gl_format_info_t *format_info = r_gl_format_info + attrib->format;

            glEnableVertexArrayAttrib(r_vao, attrib->location);
            glVertexAttribPointer(attrib->location, format_info->size, format_info->type, attrib->normalized, layout->stride, (void *)((uintptr_t)attrib->offset));
        }
    }
}

//struct r_named_uniform_t *r_GetNamedUniform(struct r_shader_t *shader, char *name)
//{
//    if(shader)
//    {
//        if(!shader->named_uniforms.elem_size)
//        {
//            shader->named_uniforms = ds_list_create(sizeof(struct r_named_uniform_t), 8);
//        }
//    }
//
//    for(uint32_t uniform_index = 0; uniform_index < shader->named_uniforms.cursor; uniform_index++)
//    {
//        struct r_named_uniform_t *uniform = ds_list_get_element(&shader->named_uniforms, uniform_index);
//
//        if(!strcmp(uniform->name, name))
//        {
//            return uniform;
//        }
//    }
//
//    struct r_named_uniform_t *uniform = NULL;
//    uint32_t uniform_location = glGetUniformLocation(shader->handle, name);
//    if(uniform_location != GL_INVALID_INDEX)
//    {
//        GLenum type;
//        GLint size;
//        uint32_t uniform_type;
//        glGetActiveUniform(shader->handle, uniform_location, 0, NULL, &size, &type, NULL);
//
//        switch(type)
//        {
//            case GL_FLOAT:
//                uniform_type = R_UNIFORM_TYPE_FLOAT;
//            break;
//
//            case GL_INT:
//            case GL_SAMPLER_1D:
//            case GL_SAMPLER_2D:
//            case GL_SAMPLER_3D:
//            case GL_SAMPLER_CUBE:
//            case GL_SAMPLER_1D_SHADOW:
//            case GL_SAMPLER_2D_SHADOW:
//            case GL_SAMPLER_1D_ARRAY:
//            case GL_SAMPLER_2D_ARRAY:
//            case GL_UNSIGNED_INT_SAMPLER_1D:
//            case GL_UNSIGNED_INT_SAMPLER_2D:
//            case GL_UNSIGNED_INT_SAMPLER_3D:
//            case GL_UNSIGNED_INT_SAMPLER_CUBE:
//                uniform_type = R_UNIFORM_TYPE_INT;
//            break;
//
//            case GL_UNSIGNED_INT:
//                uniform_type = R_UNIFORM_TYPE_UINT;
//            break;
//
//            case GL_FLOAT_VEC2:
//                uniform_type = R_UNIFORM_TYPE_VEC2;
//            break;
//
//            case GL_FLOAT_VEC3:
//                uniform_type = R_UNIFORM_TYPE_VEC3;
//            break;
//
//            case GL_FLOAT_VEC4:
//                uniform_type = R_UNIFORM_TYPE_VEC4;
//            break;
//
//            case GL_FLOAT_MAT2:
//                uniform_type = R_UNIFORM_TYPE_MAT2;
//            break;
//
//            case GL_FLOAT_MAT3:
//                uniform_type = R_UNIFORM_TYPE_MAT3;
//            break;
//
//            case GL_FLOAT_MAT4:
//                uniform_type = R_UNIFORM_TYPE_MAT4;
//            break;
//
//            default:
//                uniform_type = R_UNIFORM_TYPE_UNKNOWN;
//            break;
//        }
//
//        if(uniform_type != R_UNIFORM_TYPE_UNKNOWN)
//        {
//            uint32_t index = ds_list_add_element(&shader->named_uniforms, NULL);
//            uniform = ds_list_get_element(&shader->named_uniforms, index);
//            uniform->uniform.location = uniform_location;
//            uniform->type = uniform_type;
//            uniform->name = mem_Calloc(1, strlen(name) + 1);
//            strcpy(uniform->name, name);
//        }
//    }
//
//    return uniform;
//}

//uint32_t r_GetUniformIndex(struct r_shader_t *shader, char *name)
//{
//    return glGetUniformLocation(shader->handle, name);
//}

void r_SetUniform(uint32_t uniform, void *value)
{
    struct r_uniform_t *uniform_ptr = r_current_shader->uniforms + uniform;

    if(value)
    {
        switch(uniform_ptr->type)
        {
            case R_UNIFORM_TYPE_MAT4:
                r_SetUniformMat4(uniform, value);
            break;

            case R_UNIFORM_TYPE_VEC2:
                r_SetUniformVec2(uniform, value);
            break;

            case R_UNIFORM_TYPE_VEC4:
                r_SetUniformVec4(uniform, value);
            break;

            case R_UNIFORM_TYPE_FLOAT:
                r_SetUniformF(uniform, *(float *)value);
            break;

            case R_UNIFORM_TYPE_INT:
                r_SetUniformI(uniform, *(int32_t *)value);
            break;

            case R_UNIFORM_TYPE_UINT:
                r_SetUniformUI(uniform, *(uint32_t *)value);
            break;
        }
    }
}

void r_SetUniformI(uint32_t uniform, int32_t value)
{
    if(r_current_shader->uniforms[uniform].location != GL_INVALID_INDEX)
    {
        glUniform1i(r_current_shader->uniforms[uniform].location, value);
    }
}

void r_SetDefaultUniformI(uint32_t uniform, int32_t value)
{
    r_SetUniformI(uniform, value);
}

void r_SetUniformUI(uint32_t uniform, uint32_t value)
{
    if(r_current_shader->uniforms[uniform].location != GL_INVALID_INDEX)
    {
        glUniform1ui(r_current_shader->uniforms[uniform].location, value);
    }
}

void r_SetDefaultUniformUI(uint32_t uniform, uint32_t value)
{
    r_SetUniformUI(uniform, value);
}

void r_SetUniformF(uint32_t uniform, float value)
{
    if(r_current_shader->uniforms[uniform].location != GL_INVALID_INDEX)
    {
        glUniform1f(r_current_shader->uniforms[uniform].location, value);
    }
}

void r_SetDefaultUniformF(uint32_t uniform, float value)
{
    r_SetUniformF(uniform, value);
}

void r_SetUniformVec2(uint32_t uniform, vec2_t *value)
{
    if(r_current_shader->uniforms[uniform].location != GL_INVALID_INDEX)
    {
        glUniform2fv(r_current_shader->uniforms[uniform].location, 1, value->comps);
    }
}

void r_SetUniformVec4(uint32_t uniform, vec4_t *value)
{
    if(r_current_shader->uniforms[uniform].location != GL_INVALID_INDEX)
    {
        glUniform4fv(r_current_shader->uniforms[uniform].location, 1, value->comps);
    }
}

void r_SetDefaultUniformVec2(uint32_t uniform, vec2_t *value)
{
    r_SetUniformVec2(uniform, value);
}

void r_SetUniformMat4(uint32_t uniform, mat4_t *matrix)
{
    if(r_current_shader->uniforms[uniform].location != GL_INVALID_INDEX)
    {
        glUniformMatrix4fv(r_current_shader->uniforms[uniform].location, 1, GL_FALSE, (const float *)matrix->comps);
    }
}

void r_SetDefaultUniformMat4(uint32_t uniform, mat4_t *matrix)
{
    r_SetUniformMat4(uniform, matrix);
}

//void r_SetNamedUniformByName(char *uniform, void *value)
//{
//    struct r_named_uniform_t *named_uniform = r_GetNamedUniform(r_current_shader, uniform);
//    r_SetNamedUniform(named_uniform, value);
//}
//
//void r_SetNamedUniform(struct r_named_uniform_t *uniform, void *value)
//{
//    if(uniform)
//    {
//        switch(uniform->type)
//        {
//            case R_UNIFORM_TYPE_VEC4:
//                r_SetNamedUniformVec4(uniform, value);
//            break;
//        }
//    }
//}
//
//void r_SetNamedUniformI(struct r_named_uniform_t *uniform, int32_t value)
//{
//    if(uniform && uniform->uniform.location != GL_INVALID_INDEX)
//    {
//        glUniform1i(uniform->uniform.location, value);
//    }
//}
//
//void r_SetNamedUniformVec4(struct r_named_uniform_t *uniform, vec4_t *value)
//{
//    if(uniform && uniform->uniform.location != GL_INVALID_INDEX)
//    {
//        glUniform4fv(uniform->uniform.location, 1, (const float *)value->comps);
//    }
//}

/*
============================================================================
============================================================================
============================================================================
*/

#ifdef __cplusplus
}
#endif
