#ifndef R_MAIN_H
#define R_MAIN_H

#include "r_com.h"
#include "r_draw.h"

#include "GL/glew.h"

void r_Init();

void r_Shutdown();

/*
============================================================================
============================================================================
============================================================================
*/

struct r_vis_item_t *r_AllocateVisItem(mat4_t *transform, struct r_model_t *model);

void r_FreeVisItem(struct r_vis_item_t *item);

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

struct r_material_t *r_GetDefaultMaterial();

void r_BindMaterial(struct r_material_t *material);

/*
============================================================================
============================================================================
============================================================================
*/

struct ds_chunk_h r_AllocateVertices(uint32_t count);

void r_FreeVertices(struct ds_chunk_h chunk);

void r_FillVertices(struct ds_chunk_h chunk, struct r_vert_t *vertices, uint32_t count);

struct ds_chunk_t *r_GetVerticesChunk(struct ds_chunk_h chunk);

struct ds_chunk_h r_AllocateIndices(uint32_t count);

void r_FreeIndices(struct ds_chunk_h chunk);

void r_FillIndices(struct ds_chunk_h chunk, uint32_t *indices, uint32_t count, uint32_t offset);

struct ds_chunk_t *r_GetIndicesChunk(struct ds_chunk_h chunk);

struct r_model_t *r_LoadModel(char *file_name);

struct r_model_t *r_CreateModel(struct r_model_geometry_t *geometry, struct r_model_skeleton_t *skeleton);

void r_UpdateModelGeometry(struct r_model_t *model, struct r_model_geometry_t *geometry);

struct r_model_t *r_ShallowCopyModel(struct r_model_t *model);

void r_DestroyModel(struct r_model_t *model);

/*
============================================================================
============================================================================
============================================================================
*/

struct r_light_t *r_CreateLight(uint32_t type, vec3_t *position, vec3_t *color, float radius, float energy);

struct r_light_t *r_GetLight(uint32_t light_index);

void r_AllocShadowMaps(struct r_light_t *light, uint32_t resolution);

void r_DestroyLight(struct r_light_t *light);

/*
============================================================================
============================================================================
============================================================================
*/
//
//char *r_PreprocessShaderSource(char *shader_source);

struct r_shader_t *r_LoadShader(char *vertex_file_name, char *fragment_file_name);

void r_FreeShader(struct r_shader_t *shader);

void r_BindShader(struct r_shader_t *shader);

struct r_named_uniform_t *r_GetNamedUniform(struct r_shader_t *shader, char *name);

uint32_t r_GetUniformIndex(struct r_shader_t *shader, char *name);


void r_SetDefaultUniformI(uint32_t uniform, int32_t value);

void r_SetDefaultUniformF(uint32_t uniform, float value);

void r_SetDefaultUniformVec2(uint32_t uniform, vec2_t *value);

void r_SetDefaultUniformMat4(uint32_t uniform, mat4_t *matrix);

void r_SetNamedUniformByName(char *uniform, void *value);

void r_SetNamedUniform(struct r_named_uniform_t *uniform, void *value);

void r_SetNamedUniformI(struct r_named_uniform_t *uniform, int32_t value);

void r_SetNamedUniformVec4(struct r_named_uniform_t *uniform, vec4_t *value);




/*
============================================================================
============================================================================
============================================================================
*/


#endif // R_MAIN_H
