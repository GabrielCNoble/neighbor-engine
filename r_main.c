#include "r_main.h"
#include "SDL2/SDL.h"
#include "GL/glew.h"
#include "stb/stb_image.h"
#include "dstuff/ds_stack_list.h"
#include "dstuff/ds_file.h"
#include "dstuff/ds_mem.h"
#include "dstuff/ds_obj.h"
#include "dstuff/ds_alloc.h"
#include "r_draw.h"
#include "anim.h"

//extern struct stack_list_t d_shaders;
//extern struct stack_list_t d_textures;
//extern struct stack_list_t d_materials;
//extern struct stack_list_t d_models;
//extern struct d_texture_t *d_default_texture;
//
//extern struct ds_heap_t d_vertex_heap;
//extern struct ds_heap_t d_index_heap;


struct list_t r_draw_batches;
struct list_t r_immediate_batches;
struct stack_list_t r_shaders;
struct stack_list_t r_textures;
struct stack_list_t r_materials;
struct stack_list_t r_models;

uint32_t r_vertex_buffer;
struct ds_heap_t r_vertex_heap;
uint32_t r_index_buffer;
struct ds_heap_t r_index_heap;
uint32_t r_immediate_cursor;
uint32_t r_immediate_buffer;
//struct ds_heap_t r_immediate_heap;
    
uint32_t r_vao;
struct r_shader_t *r_lit_shader;
struct r_shader_t *r_immediate_shader;
struct r_shader_t *r_current_shader;

struct r_model_t *test_model;
struct r_texture_t *r_default_texture;
struct r_material_t *r_default_material;

extern mat4_t r_projection_matrix;
extern mat4_t r_view_matrix;
extern mat4_t r_inv_view_matrix;
extern mat4_t r_view_projection_matrix;

SDL_Window *r_window;
SDL_GLContext *r_context;
int32_t r_width = 800;
int32_t r_height = 600;


char *d_uniform_names[] = 
{
    [R_UNIFORM_MVP] = "d_mvp",
    [R_UNIFORM_TEX0] = "d_tex0",
    [R_UNIFORM_TEX1] = "d_tex1",
};

void r_Init()
{    
    r_draw_batches = create_list(sizeof(struct r_draw_batch_t), 4096);
    r_immediate_batches = create_list(sizeof(struct r_immediate_batch_t), 4096);
    r_shaders = create_stack_list(sizeof(struct r_shader_t), 16);
    r_materials = create_stack_list(sizeof(struct r_material_t), 32);
    r_textures = create_stack_list(sizeof(struct r_texture_t), 128);
    r_models = create_stack_list(sizeof(struct r_model_t), 512);
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
    
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClearDepth(1.0);

    glGenBuffers(1, &r_vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, r_vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, R_VERTEX_BUFFER_SIZE, NULL, GL_DYNAMIC_DRAW);
    glGenBuffers(1, &r_immediate_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, r_immediate_buffer);
    glBufferData(GL_ARRAY_BUFFER, R_IMMEDIATE_BUFFER_SIZE, NULL, GL_DYNAMIC_DRAW);
    glGenBuffers(1, &r_index_buffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, r_index_buffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, R_INDEX_BUFFER_SIZE, NULL, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    
    glGenVertexArrays(1, &r_vao);
    glBindVertexArray(r_vao);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    
    r_lit_shader = r_LoadShader("shaders/lit.vert", "shaders/lit.frag");
    r_immediate_shader = r_LoadShader("shaders/immediate.vert", "shaders/immediate.frag");
    mat4_t_persp(&r_projection_matrix, 0.68, (float)r_width / (float)r_height, 0.1, 100.0);
    mat4_t_identity(&r_view_matrix);
    
    uint32_t pixels[] = 
    {
        0xff777777, 0xff444444, 0xff777777, 0xff444444,
        0xff444444, 0xff777777, 0xff444444, 0xff777777,
        0xff777777, 0xff444444, 0xff777777, 0xff444444,
        0xff444444, 0xff777777, 0xff444444, 0xff777777,
    };
    r_default_texture = r_CreateTexture("default", 4, 4, GL_RGBA8, pixels);
    r_default_material = r_CreateMaterial("deafult", r_default_texture, NULL);
}

void r_Shutdown()
{
    
}
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
    }
    
    texture_index = add_stack_list_element(&r_textures, NULL);
    texture = get_stack_list_element(&r_textures, texture_index);
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
        struct r_texture_t *texture = get_stack_list_element(&r_textures, texture_index);
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

struct r_material_t *r_CreateMaterial(char *name, struct r_texture_t *diffuse_texture, struct r_texture_t *normal_texture)
{
    uint32_t material_index;
    struct r_material_t *material;
    
    material_index = add_stack_list_element(&r_materials, NULL);
    material = get_stack_list_element(&r_materials, material_index);
    
    material->index = material_index;
    material->name = strdup(name);
    material->diffuse_texture = diffuse_texture;
    material->normal_texture = normal_texture;
    
    return material;
}

struct r_material_t *r_GetMaterial(char *name)
{
    for(uint32_t material_index = 0; material_index < r_materials.cursor; material_index++)
    {
        struct r_material_t *material = get_stack_list_element(&r_materials, material_index);
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

void r_BindMaterial(struct r_material_t *material)
{
    if(material)
    {
        if(material->diffuse_texture)
        {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, material->diffuse_texture->handle);
            r_SetUniform1i(R_UNIFORM_TEX0, 0);
        }   
        
        if(material->normal_texture)
        {
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, material->normal_texture->handle);
            r_SetUniform1i(R_UNIFORM_TEX1, 1);
        }
    }
}

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

struct r_model_t *r_LoadModel(char *file_name)
{
    struct r_model_t *model = NULL;
    if(file_exists(file_name))
    {
        void *file_buffer;
        FILE *file = fopen(file_name, "rb");
        read_file(file, &file_buffer, NULL);
        fclose(file);
        
        struct a_skeleton_section_t *skeleton_section;
        struct a_weight_section_t *weight_section;
        struct r_vert_section_t *verts;
        struct r_index_section_t *indexes;
        struct r_batch_section_t *batch_section;
        struct r_material_section_t *materials;
        
        ds_get_section_data(file_buffer, "[skeleton]", (void **)&skeleton_section, NULL);
        ds_get_section_data(file_buffer, "[weights]", (void **)&weight_section, NULL);
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
                    
                struct r_texture_t *diffuse_texture;
                
                if(material_record->diffuse_texture[0]) 
                {
                    diffuse_texture = r_GetTexture(material_record->diffuse_texture);
                    if(!diffuse_texture)
                    {
                        diffuse_texture = r_LoadTexture(material_record->diffuse_texture, material_record->diffuse_texture);
                    }
                }
                else
                {
                    diffuse_texture = r_default_texture;
                }
                
                struct r_texture_t *normal_texture = r_GetTexture(material_record->normal_texture);
                if(!normal_texture)
                {
                    normal_texture = r_LoadTexture(material_record->normal_texture, material_record->normal_texture);
                }
                
                material = r_CreateMaterial(batch_record->material, diffuse_texture, normal_texture);
            }
            
            batches[batch_index].start = batch_record->start;
            batches[batch_index].count = batch_record->count;
            batches[batch_index].material = material;
        }
        
        struct a_skeleton_t *skeleton = NULL;
        struct a_weight_t *weights = NULL;
        uint32_t weight_count = 0;
        
        if(skeleton_section && skeleton_section->bone_count)
        {
            skeleton = a_CreateSkeleton(skeleton_section->bone_count, skeleton_section->bones);
            weights = mem_Calloc(verts->vert_count * R_MAX_VERTEX_WEIGHTS, sizeof(struct a_weight_t));
            uint32_t cur_index = 0xffffffff;
            uint32_t cur_index_count = 0;
            
            for(uint32_t weight_index = 0; weight_index < weight_section->weight_count; weight_index++)
            {
                struct a_weight_record_t *weight = weight_section->weights + weight_index;
                if(cur_index != weight->vert_index)
                {
                    if(cur_index_count < R_MAX_VERTEX_WEIGHTS && cur_index != 0xffffffff)
                    {
                        while(cur_index_count < R_MAX_VERTEX_WEIGHTS)
                        {
                            weights[weight_count].bone_index = 0xffffffff;
                            weights[weight_count].weight = 0.0;
                            weight_count++;
                            cur_index_count++;
                        }
                    }
                    cur_index = weight->vert_index;
                    cur_index_count = 0;
                }
                
                if(cur_index_count < R_MAX_VERTEX_WEIGHTS)
                {
                    weights[weight_count] = weight->weight;
                    weight_count++;
                    cur_index_count++;
                }
            }
            weight_count = verts->vert_count * R_MAX_VERTEX_WEIGHTS;
        }
        struct r_model_create_info_t create_info = {};
        create_info.vert_count = verts->vert_count;
        create_info.verts = verts->verts;
        create_info.index_count = indexes->index_count;
        create_info.indices = indexes->indexes;
        create_info.batch_count = batch_section->batch_count;
        create_info.batches = batches;
        create_info.skeleton = skeleton;
        create_info.weight_count = weight_count;
        create_info.weights = weights;
        
        
        model = r_CreateModel(&create_info);
        mem_Free(batches);
        mem_Free(file_buffer);
        if(weights)
        {
            mem_Free(weights);
        }
    }
    
    return model;
}

struct r_model_t *r_CreateModel(struct r_model_create_info_t *create_info)
{
    struct r_model_t *model;
    uint32_t index;
    
    index = add_stack_list_element(&r_models, NULL);
    model = get_stack_list_element(&r_models, index);
    model->index = index;
    model->base = NULL;
    model->vert_chunk = r_AllocateVertices(create_info->vert_count);
    r_FillVertices(model->vert_chunk, create_info->verts, create_info->vert_count);
    
    struct ds_chunk_t *chunk = ds_get_chunk_pointer(&r_vertex_heap, model->vert_chunk);
    uint32_t start = chunk->start / sizeof(struct r_vert_t);
    model->index_chunk = r_AllocateIndices(create_info->index_count);
    r_FillIndices(model->index_chunk, create_info->indices, create_info->index_count, start);
    
    chunk = ds_get_chunk_pointer(&r_index_heap, model->index_chunk);
    start = chunk->start / sizeof(uint32_t);
    
    model->batch_count = create_info->batch_count;
    model->batches = mem_Calloc(create_info->batch_count, sizeof(struct r_batch_t));
    memcpy(model->batches, create_info->batches, sizeof(struct r_batch_t) * model->batch_count);
    for(uint32_t batch_index = 0; batch_index < create_info->batch_count; batch_index++)
    {
        model->batches[batch_index].start += start;
    }
    model->skeleton = create_info->skeleton;
    model->weight_count = create_info->weight_count;
    model->weights = mem_Calloc(create_info->weight_count, sizeof(struct a_weight_t));
    memcpy(model->weights, create_info->weights, sizeof(struct a_weight_t) * create_info->weight_count);
    
    model->vert_count = create_info->vert_count;
    model->verts = mem_Calloc(create_info->vert_count, sizeof(struct r_vert_t));
    memcpy(model->verts, create_info->verts, sizeof(struct r_vert_t) * create_info->vert_count);
    
    model->indice_count = create_info->index_count;
    model->indices = mem_Calloc(create_info->index_count, sizeof(uint32_t));
    memcpy(model->indices, create_info->indices, sizeof(uint32_t) * create_info->index_count);
    
    return model;
}

struct r_model_t *r_ShallowCopyModel(struct r_model_t *base)
{
    struct r_model_t *copy;
    uint32_t index = add_stack_list_element(&r_models, NULL);
    struct ds_chunk_t *chunk;
    copy = get_stack_list_element(&r_models, index);
    memcpy(copy, base, sizeof(struct r_model_t));
    copy->index = index;
    
    copy->verts = mem_Calloc(copy->vert_count, sizeof(struct r_vert_t));
    memcpy(copy->verts, base->verts, sizeof(struct r_vert_t) * copy->vert_count);
    
    copy->vert_chunk = r_AllocateVertices(copy->vert_count);
    r_FillVertices(copy->vert_chunk, copy->verts, copy->vert_count);
    chunk = ds_get_chunk_pointer(&r_vertex_heap, copy->vert_chunk);
    uint32_t new_start = chunk->start / sizeof(struct r_vert_t);
    
    copy->index_chunk = r_AllocateIndices(copy->indice_count);
    r_FillIndices(copy->index_chunk, copy->indices, copy->indice_count, new_start);
    chunk = ds_get_chunk_pointer(&r_index_heap, copy->index_chunk);
    new_start = chunk->start / sizeof(uint32_t);
    chunk = ds_get_chunk_pointer(&r_index_heap, base->index_chunk);
    uint32_t old_start = chunk->start / sizeof(uint32_t);
    
    copy->batches = mem_Calloc(copy->batch_count, sizeof(struct r_batch_t));
    for(uint32_t batch_index = 0; batch_index < copy->batch_count; batch_index++)
    {
        copy->batches[batch_index] = base->batches[batch_index];
        copy->batches[batch_index].start -= old_start;
        copy->batches[batch_index].start += new_start;
    }
    
    copy->base = base;
    
    return copy;
}

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
    vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, (const GLchar * const *)&shader_source, NULL);
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
    fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, (const GLchar * const *)&shader_source, NULL);
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
    
    uint32_t shader_index = add_stack_list_element(&r_shaders, NULL);
    shader = get_stack_list_element(&r_shaders, shader_index);
    shader->index = shader_index;
    shader->handle = shader_program;
    
    for(uint32_t uniform_index = 0; uniform_index < R_UNIFORM_LAST; uniform_index++)
    {
        shader->uniforms[uniform_index] = glGetUniformLocation(shader_program, d_uniform_names[uniform_index]);
    }
    shader->attribs = 0;
    
    if(glGetAttribLocation(shader_program, "d_position") != -1)
    {
        shader->attribs |= R_ATTRIB_POSITION;
        glBindAttribLocation(shader_program, R_POSITION_LOCATION, "d_position");
    }
    
    if(glGetAttribLocation(shader_program, "d_tex_coords") != -1)
    {
        shader->attribs |= R_ATTRIB_TEX_COORDS;
        glBindAttribLocation(shader_program, R_TEX_COORDS_LOCATION, "d_tex_coords");
    }
    
    if(glGetAttribLocation(shader_program, "d_normal") != -1)
    {
        shader->attribs |= R_ATTRIB_NORMAL;
        glBindAttribLocation(shader_program, R_NORMAL_LOCATION, "d_normal");
    }
    
    if(glGetAttribLocation(shader_program, "d_color") != -1)
    {
        shader->attribs |= R_ATTRIB_COLOR;
        glBindAttribLocation(shader_program, R_COLOR_LOCATION, "d_color");
    }
    
    return shader;
}

void r_FreeShader(struct r_shader_t *shader)
{
    if(shader && shader->handle < 0xffffffff)
    {
        glDeleteProgram(shader->handle);
        shader->handle = 0xffffffff;
        remove_stack_list_element(&r_shaders, shader->index);
    }
}

void r_BindShader(struct r_shader_t *shader)
{
    if(shader)
    {
        r_current_shader = shader;
        glUseProgram(shader->handle);
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
            glVertexAttribPointer(R_NORMAL_LOCATION, 3, GL_FLOAT, GL_FALSE, sizeof(struct r_vert_t), (void *)offsetof(struct r_vert_t, normal));
        }
        else
        {
            glDisableVertexArrayAttrib(r_vao, R_NORMAL_LOCATION);
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

void r_SetUniformMatrix4(uint32_t uniform, mat4_t *matrix)
{
    glUniformMatrix4fv(r_current_shader->uniforms[uniform], 1, GL_FALSE, (const float *)matrix->comps);
}

void r_SetUniform1i(uint32_t uniform, uint32_t value)
{
    glUniform1i(uniform, value);
}
