#ifndef R_MAIN_H
#define R_MAIN_H

#include "r_com.h"

#include "GL/glew.h"

void r_Init();

void r_Shutdown();

/*
============================================================================
============================================================================
============================================================================
*/

struct r_texture_t *r_LoadTexture(char *file_name, char *name);

struct r_texture_t *r_CreateTexture(char *name, uint32_t width, uint32_t height, uint32_t internal_format, void *data);

struct r_texture_t *r_GetTexture(char *name);

void r_FreeTexture(struct r_texture_t *texture);

/*
============================================================================
============================================================================
============================================================================
*/

struct r_material_t *r_CreateMaterial(char *name, struct r_texture_t *diffuse_texture, struct r_texture_t *normal_texture, struct r_texture_t *roughness_texture);

struct r_material_t *r_GetMaterial(char *name);

void r_BindMaterial(struct r_material_t *material);

/*
============================================================================
============================================================================
============================================================================
*/

struct ds_chunk_h r_AllocateVertices(uint32_t count);

void r_FreeVertices(struct ds_chunk_h chunk);

void r_FillVertices(struct ds_chunk_h chunk, struct r_vert_t *vertices, uint32_t count);

struct ds_chunk_h r_AllocateIndices(uint32_t count);

void r_FreeIndices(struct ds_chunk_h chunk);

void r_FillIndices(struct ds_chunk_h chunk, uint32_t *indices, uint32_t count, uint32_t offset);

struct r_model_t *r_LoadModel(char *file_name);

struct r_model_t *r_CreateModel(struct r_model_create_info_t *create_info);

struct r_model_t *r_ShallowCopyModel(struct r_model_t *model);

/*
============================================================================
============================================================================
============================================================================
*/

struct r_light_t *r_CreateLight(uint32_t type, vec3_t *position, vec3_t *color, float radius, float energy);

struct r_light_t *r_GetLight(uint32_t light_index);

void r_DestroyLight(struct r_light_t *light);

/*
============================================================================
============================================================================
============================================================================
*/

struct r_shader_t *r_LoadShader(char *vertex_file_name, char *fragment_file_name);

void r_FreeShader(struct r_shader_t *shader);

void r_BindShader(struct r_shader_t *shader);

void r_SetUniformMatrix4(uint32_t uniform, mat4_t *matrix);

void r_SetUniform1f(uint32_t uniform, float value);

void r_SetUniform1i(uint32_t uniform, uint32_t value);

/*
============================================================================
============================================================================
============================================================================
*/


#endif // R_MAIN_H
