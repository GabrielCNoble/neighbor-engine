#ifndef R_MAIN_H
#define R_MAIN_H

#include "r_defs.h"
#include "r_draw.h"

#include "../lib/GLEW/include/GL/glew.h"

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
    texture
============================================================================
*/

struct r_texture_t     *r_LoadTexture(char *file_name);

struct r_texture_t     *r_CreateTexture(char *name, uint32_t width, uint32_t height, uint32_t format, uint32_t min_filter, uint32_t mag_filter, void *data);

void                    r_ResizeTexture(struct r_texture_t *texture, uint32_t width, uint32_t height);

struct r_texture_t     *r_GetTexture(uint32_t index);

struct r_texture_t     *r_FindTexture(char *name);

void                    r_BindTexture(struct r_texture_t *texture, uint32_t tex_unit);

void                    r_DestroyTexture(struct r_texture_t *texture);

/*
============================================================================
    framebuffer
============================================================================
*/

struct r_framebuffer_t     *r_CreateFramebuffer(uint16_t width, uint16_t height);

struct r_framebuffer_t     *r_GetFramebuffer(uint32_t index);

void                        r_DestroyFramebuffer(struct r_framebuffer_t *framebuffer);

void                        r_AddAttachment(struct r_framebuffer_t *framebuffer, uint16_t attachment, uint16_t format, uint16_t min_filter, uint16_t mag_filter);

void                        r_ResizeFramebuffer(struct r_framebuffer_t *framebuffer, uint16_t width, uint16_t height);

void                        r_BindFramebuffer(struct r_framebuffer_t *framebuffer);

void                        r_BlitFramebuffer(struct r_framebuffer_t *src_framebuffer, struct r_framebuffer_t *dst_framebuffer, uint32_t mask, uint32_t filter);

void                        r_PresentFramebuffer(struct r_framebuffer_t *framebuffer);



/*
============================================================================
    material
============================================================================
*/

struct r_material_t    *r_CreateMaterial(char *name, struct r_texture_t *diffuse_texture, struct r_texture_t *normal_texture, struct r_texture_t *roughness_texture);

struct r_material_t    *r_GetMaterial(uint32_t index);

struct r_material_t    *r_FindMaterial(char *name);

struct r_material_t    *r_GetDefaultMaterial();

void                    r_DestroyMaterial(struct r_material_t *material);

void                    r_BindMaterial(struct r_material_t *material);

/*
============================================================================
    model
============================================================================
*/

struct ds_chunk_h       r_AllocateVertices(uint32_t count);

void                    r_FreeVertices(struct ds_chunk_h chunk);

void                    r_FillVertices(struct ds_chunk_h chunk, struct r_vert_t *vertices, uint32_t count);

struct ds_chunk_t      *r_GetVerticesChunk(struct ds_chunk_h chunk);

struct ds_chunk_h       r_AllocateIndices(uint32_t count);

void                    r_FreeIndices(struct ds_chunk_h chunk);

void                    r_FillIndices(struct ds_chunk_h chunk, uint32_t *indices, uint32_t count, uint32_t offset);

struct ds_chunk_t      *r_GetIndicesChunk(struct ds_chunk_h chunk);

struct r_model_t       *r_LoadModel(char *file_name);

struct r_model_t       *r_GetModel(uint32_t index);

struct r_model_t       *r_CreateModel(struct r_model_geometry_t *geometry, struct r_model_skeleton_t *skeleton, char *name);

struct r_model_t       *r_FindModel(char *name);

void                    r_UpdateModelGeometry(struct r_model_t *model, struct r_model_geometry_t *geometry);

struct r_model_t       *r_ShallowCopyModel(struct r_model_t *model);

void                    r_DestroyModel(struct r_model_t *model);

/*
============================================================================
    light
============================================================================
*/

struct r_light_t           *r_CreateLight(uint32_t type, vec3_t *position, vec3_t *color, float radius, float energy);

struct r_light_t           *r_CopyLight(struct r_light_t *light);

struct r_point_light_t     *r_CreatePointLight(vec3_t *position, vec3_t *color, float radius, float energy);

struct r_spot_light_t      *r_CreateSpotLight(vec3_t *position, vec3_t *color, mat3_t *orientation, float radius, float energy, uint32_t angle, float softness);

struct r_light_t           *r_GetLight(uint32_t light_index);

void                        r_AllocShadowMaps(struct r_light_t *light, uint32_t resolution);

struct r_shadow_map_t      *r_GetShadowMap(uint32_t shadow_map);

void                        r_FreeShadowMaps(struct r_light_t *light);

void                        r_DestroyLight(struct r_light_t *light);

void                        r_DestroyAllLighs();

/*
============================================================================
    shader
============================================================================
*/

struct r_shader_t          *r_LoadShader(char *vertex_file_name, char *fragment_file_name);

void                        r_FreeShader(struct r_shader_t *shader);

void                        r_BindShader(struct r_shader_t *shader);

struct r_named_uniform_t   *r_GetNamedUniform(struct r_shader_t *shader, char *name);

uint32_t                    r_GetUniformIndex(struct r_shader_t *shader, char *name);

void                        r_SetDefaultUniformI(uint32_t uniform, int32_t value);

void                        r_SetDefaultUniformUI(uint32_t uniform, uint32_t value);

void                        r_SetDefaultUniformF(uint32_t uniform, float value);

void                        r_SetDefaultUniformVec2(uint32_t uniform, vec2_t *value);

void                        r_SetDefaultUniformMat4(uint32_t uniform, mat4_t *matrix);

void                        r_SetNamedUniformByName(char *uniform, void *value);

void                        r_SetNamedUniform(struct r_named_uniform_t *uniform, void *value);

void                        r_SetNamedUniformI(struct r_named_uniform_t *uniform, int32_t value);

void                        r_SetNamedUniformVec4(struct r_named_uniform_t *uniform, vec4_t *value);




/*
============================================================================
============================================================================
============================================================================
*/


#endif // R_MAIN_H
