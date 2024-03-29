#include "r_draw_i.h"

extern struct r_i_cmd_buffer_t r_immediate_cmd_buffer;
extern struct r_shader_t *r_immediate_shader;
extern struct r_shader_t *r_current_shader;
extern uint32_t r_uniform_type_sizes[];

struct r_i_draw_state_t *r_i_DrawState(struct r_i_cmd_buffer_t *cmd_buffer, struct r_i_draw_range_t *range)
{
    if(!cmd_buffer)
    {
        cmd_buffer = &r_immediate_cmd_buffer;
    }

    if(range)
    {
        if(!range->draw_state)
        {
            range->draw_state = r_AllocImmediateCmdData(cmd_buffer, sizeof(struct r_i_draw_state_t));
            memset(range->draw_state, 0, sizeof(struct r_i_draw_state_t));
        }

        return range->draw_state;
    }

    if(!cmd_buffer->draw_state)
    {
        cmd_buffer->draw_state = r_AllocImmediateCmdData(cmd_buffer, sizeof(struct r_i_draw_state_t));
        memset(cmd_buffer->draw_state, 0, sizeof(struct r_i_draw_state_t));
    }

    return cmd_buffer->draw_state;
}

void r_i_ApplyDrawState(struct r_i_draw_state_t *draw_state)
{
    if(draw_state->framebuffer != NULL)
    {
        struct r_i_framebuffer_t *framebuffer = draw_state->framebuffer;
        r_BindFramebuffer(framebuffer->framebuffer);
    }

    if(draw_state->draw_mask != NULL)
    {
        struct r_i_draw_mask_t *draw_mask = draw_state->draw_mask;

        glColorMask(draw_mask->red, draw_mask->green, draw_mask->blue, draw_mask->alpha);
        glDepthMask(draw_mask->depth);
        glStencilMask(draw_mask->stencil);
    }

    if(draw_state->rasterizer != NULL)
    {
        struct r_i_raster_t *rasterizer = draw_state->rasterizer;

        if(rasterizer->point_size > 0.0)
        {
            glPointSize(rasterizer->point_size);
        }

        if(rasterizer->line_width > 0.0)
        {
            glLineWidth(rasterizer->line_width);
        }

        if(rasterizer->polygon_mode != R_I_DONT_CARE)
        {
            glPolygonMode(GL_FRONT_AND_BACK, rasterizer->polygon_mode);
        }

        if(rasterizer->cull_enable == R_I_ENABLE)
        {
            glEnable(GL_CULL_FACE);
        }
        else if(rasterizer->cull_enable == R_I_DISABLE)
        {
            glDisable(GL_CULL_FACE);
        }

        if(rasterizer->cull_face != R_I_DONT_CARE)
        {
            glCullFace(rasterizer->cull_face);
        }

        if(rasterizer->polygon_offset_enable == R_I_ENABLE)
        {
            glEnable(rasterizer->offset_type);
            glPolygonOffset(rasterizer->factor, rasterizer->units);
        }
        else if(rasterizer->polygon_offset_enable == R_I_DISABLE)
        {
            glDisable(rasterizer->offset_type);
        }
    }

    if(draw_state->scissor != NULL)
    {
        struct r_i_scissor_t *scissor = draw_state->scissor;

        if(scissor->enable == R_I_ENABLE)
        {
            glEnable(GL_SCISSOR_TEST);
        }
        else if(scissor->enable == R_I_DISABLE)
        {
            glDisable(GL_SCISSOR_TEST);
        }

        glScissor(scissor->x, scissor->y, scissor->width, scissor->height);

//        if(scissor->x != GL_DONT_CARE && scissor->y != GL_DONT_CARE &&
//           scissor->width != GL_DONT_CARE && scissor->height != GL_DONT_CARE)
//        {
//            glScissor(scissor->x, scissor->y, scissor->width, scissor->height);
//        }
    }

    if(draw_state->blending != NULL)
    {
        struct r_i_blending_t *blending = draw_state->blending;

        if(blending->enable == R_I_ENABLE)
        {
            glEnable(GL_BLEND);
        }
        else if(blending->enable == R_I_DISABLE)
        {
            glDisable(GL_BLEND);
        }

        if(blending->src_factor != R_I_DONT_CARE && blending->dst_factor != R_I_DONT_CARE)
        {
            glBlendFunc(blending->src_factor, blending->dst_factor);
        }
    }

    if(draw_state->depth != NULL)
    {
        struct r_i_depth_t *depth = draw_state->depth;

        if(depth->enable == R_I_ENABLE)
        {
            glEnable(GL_DEPTH_TEST);
        }
        else if(depth->enable == R_I_DISABLE)
        {
            glDisable(GL_DEPTH_TEST);
        }

        if(depth->func != R_I_DONT_CARE)
        {
            glDepthFunc(depth->func);
        }
    }

    if(draw_state->stencil != NULL)
    {
        struct r_i_stencil_t *stencil = draw_state->stencil;

        if(stencil->enable == R_I_ENABLE)
        {
            glEnable(GL_STENCIL_TEST);
        }
        else if(stencil->enable == R_I_DISABLE)
        {
            glDisable(GL_STENCIL_TEST);
        }

        if(stencil->depth_fail != R_I_DONT_CARE &&
           stencil->depth_pass != R_I_DONT_CARE &&
           stencil->stencil_fail != R_I_DONT_CARE)
        {
            glStencilOp(stencil->stencil_fail, stencil->depth_fail, stencil->depth_pass);
        }

        if(stencil->func != R_I_DONT_CARE)
        {
            glStencilFunc(stencil->func, stencil->ref, stencil->mask);
        }
    }
}

void r_i_ApplyUniforms(struct r_i_uniform_list_t *uniform_list)
{
    struct r_shader_t *shader = uniform_list->shader;

    for(uint32_t index = 0; index < shader->uniform_count; index++)
    {
        struct r_i_uniform_t *uniform = uniform_list->uniforms + index;

        if(uniform->value)
        {
            struct r_uniform_t *shader_uniform = shader->uniforms + uniform->uniform;

            if(shader_uniform->type == R_UNIFORM_TYPE_TEXTURE)
            {
                struct r_i_texture_t *texture = uniform->value;
                r_BindTexture(texture->texture, GL_TEXTURE0 + texture->tex_unit);
                r_SetUniform(uniform->uniform, &texture->tex_unit);
            }
            else
            {
                r_SetUniform(uniform->uniform, uniform->value);
            }
        }
    }
}

void r_i_SetShader(struct r_i_cmd_buffer_t *cmd_buffer, struct r_shader_t *shader)
{
    if(!shader)
    {
        shader = r_immediate_shader;
    }

    if(!cmd_buffer)
    {
        cmd_buffer = &r_immediate_cmd_buffer;
    }

    cmd_buffer->shader = shader;
    cmd_buffer->dirty_shader = 1;
}

void r_i_SetUniforms(struct r_i_cmd_buffer_t *cmd_buffer, struct r_i_draw_range_t *range, struct r_i_uniform_t *uniforms, uint32_t count)
{
    if(!cmd_buffer)
    {
        cmd_buffer = &r_immediate_cmd_buffer;
    }

    struct r_i_uniform_list_t *uniform_list = NULL;
    struct r_i_uniform_list_t **uniform_src = NULL;
    struct r_shader_t *shader = NULL;

    if(range)
    {
        shader = range->shader;
        uniform_src = &range->uniforms;
    }
    else
    {
        shader = cmd_buffer->shader;
        uniform_src = &cmd_buffer->uniforms;
    }

    if(!shader)
    {
        /* no shader has been set for this command buffer yet */
        return;
    }

    if(!(*uniform_src))
    {
        *uniform_src = r_AllocImmediateCmdData(cmd_buffer, sizeof(struct r_i_uniform_list_t ));
        (*uniform_src)->uniforms = r_AllocImmediateCmdData(cmd_buffer, sizeof(struct r_i_uniform_t) * shader->uniform_count);
        (*uniform_src)->shader = shader;
        memset((*uniform_src)->uniforms, 0, sizeof(struct r_i_uniform_t) * shader->uniform_count);
    }

    uniform_list = *uniform_src;

    for(uint32_t uniform_index = 0; uniform_index < count; uniform_index++)
    {
        struct r_i_uniform_t *src = uniforms + uniform_index;
        struct r_i_uniform_t *dst = uniform_list->uniforms + src->uniform;

        dst->count = src->count;
        dst->uniform = src->uniform;

        struct r_uniform_t *uniform = shader->uniforms + dst->uniform;
        uint32_t uniform_size = 0;

        if(uniform->type == R_UNIFORM_TYPE_TEXTURE)
        {
            uniform_size = sizeof(struct r_i_texture_t);
        }
        else
        {
            uniform_size = r_uniform_type_sizes[uniform->type] * src->count;
        }

        if(!dst->value)
        {
            dst->value = r_AllocImmediateCmdData(cmd_buffer, uniform_size);
        }

        memcpy(dst->value, src->value, uniform_size);
    }

//    if(range)
//    {
//        range->uniforms = uniform_list;
//    }
//    else
//    {
//        cmd_buffer->uniforms = uniform_list;
//    }
}

void r_i_SetBlending(struct r_i_cmd_buffer_t *cmd_buffer, struct r_i_draw_range_t *range, struct r_i_blending_t *blending)
{
    if(!cmd_buffer)
    {
        cmd_buffer = &r_immediate_cmd_buffer;
    }

    struct r_i_draw_state_t *draw_state = r_i_DrawState(cmd_buffer, range);

    if(!draw_state->blending)
    {
        draw_state->blending = r_AllocImmediateCmdData(cmd_buffer, sizeof(struct r_i_blending_t));
    }

    *draw_state->blending = *blending;
}

void r_i_SetDepth(struct r_i_cmd_buffer_t *cmd_buffer, struct r_i_draw_range_t *range, struct r_i_depth_t *depth)
{
    if(!cmd_buffer)
    {
        cmd_buffer = &r_immediate_cmd_buffer;
    }

    struct r_i_draw_state_t *draw_state = r_i_DrawState(cmd_buffer, range);

    if(!draw_state->depth)
    {
        draw_state->depth = r_AllocImmediateCmdData(cmd_buffer, sizeof(struct r_i_depth_t));
    }

    *draw_state->depth = *depth;
}

void r_i_SetStencil(struct r_i_cmd_buffer_t *cmd_buffer, struct r_i_draw_range_t *range, struct r_i_stencil_t *stencil)
{
    if(!cmd_buffer)
    {
        cmd_buffer = &r_immediate_cmd_buffer;
    }

    struct r_i_draw_state_t *draw_state = r_i_DrawState(cmd_buffer, range);

    if(!draw_state->stencil)
    {
        draw_state->stencil = r_AllocImmediateCmdData(cmd_buffer, sizeof(struct r_i_stencil_t));
    }

    *draw_state->stencil = *stencil;
}

void r_i_SetRasterizer(struct r_i_cmd_buffer_t *cmd_buffer, struct r_i_draw_range_t *range, struct r_i_raster_t *rasterizer)
{
    if(!cmd_buffer)
    {
        cmd_buffer = &r_immediate_cmd_buffer;
    }

    struct r_i_draw_state_t *draw_state = r_i_DrawState(cmd_buffer, range);

    if(!draw_state->rasterizer)
    {
        draw_state->rasterizer = r_AllocImmediateCmdData(cmd_buffer, sizeof(struct r_i_raster_t));
    }

    *draw_state->rasterizer = *rasterizer;
}

void r_i_SetDrawMask(struct r_i_cmd_buffer_t *cmd_buffer, struct r_i_draw_range_t *range, struct r_i_draw_mask_t *draw_mask)
{
    if(!cmd_buffer)
    {
        cmd_buffer = &r_immediate_cmd_buffer;
    }

    struct r_i_draw_state_t *draw_state = r_i_DrawState(cmd_buffer, range);

    if(!draw_state->draw_mask)
    {
        draw_state->draw_mask = r_AllocImmediateCmdData(cmd_buffer, sizeof(struct r_i_draw_mask_t));
    }

    *draw_state->draw_mask = *draw_mask;
}

void r_i_SetScissor(struct r_i_cmd_buffer_t *cmd_buffer, struct r_i_draw_range_t *range, struct r_i_scissor_t *scissor)
{
    if(!cmd_buffer)
    {
        cmd_buffer = &r_immediate_cmd_buffer;
    }

    struct r_i_draw_state_t *draw_state = r_i_DrawState(cmd_buffer, range);

    if(!draw_state->scissor)
    {
        draw_state->scissor = r_AllocImmediateCmdData(cmd_buffer, sizeof(struct r_i_scissor_t));
    }

    *draw_state->scissor = *scissor;
}

void r_i_SetFramebuffer(struct r_i_cmd_buffer_t *cmd_buffer, struct r_i_draw_range_t *range, struct r_i_framebuffer_t *framebuffer)
{
    if(!cmd_buffer)
    {
        cmd_buffer = &r_immediate_cmd_buffer;
    }

    struct r_i_draw_state_t *draw_state = r_i_DrawState(cmd_buffer, range);

    if(!draw_state->framebuffer)
    {
        draw_state->framebuffer = r_AllocImmediateCmdData(cmd_buffer, sizeof(struct r_i_framebuffer_t));
    }

    *draw_state->framebuffer = *framebuffer;
}

void r_i_Clear(struct r_i_cmd_buffer_t *cmd_buffer, struct r_i_draw_range_t *range, struct r_i_clear_t *clear)
{
    if(!cmd_buffer)
    {
        cmd_buffer = &r_immediate_cmd_buffer;
    }

    if(cmd_buffer->draw_state)
    {
        r_IssueImmediateCmd(cmd_buffer, R_I_CMD_SET_DRAW_STATE, cmd_buffer->draw_state);
        cmd_buffer->draw_state = NULL;
    }

    struct r_i_clear_t *clear_state = r_AllocImmediateCmdData(cmd_buffer, sizeof(struct r_i_clear_t));
    *clear_state = *clear;
    r_IssueImmediateCmd(cmd_buffer, R_I_CMD_CLEAR, clear_state);
}

void r_i_DrawList(struct r_i_cmd_buffer_t *cmd_buffer, struct r_i_draw_list_t *draw_list)
{
    if(!cmd_buffer)
    {
        cmd_buffer = &r_immediate_cmd_buffer;
    }

    if(cmd_buffer->dirty_shader)
    {
        struct r_i_shader_t *shader = r_AllocImmediateCmdData(cmd_buffer, sizeof(struct r_i_shader_t));
        shader->shader = cmd_buffer->shader;
        r_IssueImmediateCmd(cmd_buffer, R_I_CMD_SET_SHADER, shader);
        cmd_buffer->dirty_shader = 0;
    }

    if(cmd_buffer->uniforms)
    {
        r_IssueImmediateCmd(cmd_buffer, R_I_CMD_SET_UNIFORM, cmd_buffer->uniforms);
        cmd_buffer->uniforms = NULL;
    }

    if(cmd_buffer->draw_state)
    {
        r_IssueImmediateCmd(cmd_buffer, R_I_CMD_SET_DRAW_STATE, cmd_buffer->draw_state);
        cmd_buffer->draw_state = NULL;
    }

    r_IssueImmediateCmd(cmd_buffer, R_I_CMD_DRAW, draw_list);
}

struct r_i_draw_list_t *r_i_AllocDrawList(struct r_i_cmd_buffer_t *cmd_buffer, uint32_t range_count)
{
    if(!cmd_buffer)
    {
        cmd_buffer = &r_immediate_cmd_buffer;
    }

    struct r_i_draw_list_t *draw_list = r_AllocImmediateCmdData(cmd_buffer, sizeof(struct r_i_draw_list_t));
    draw_list->ranges =  r_AllocImmediateCmdData(cmd_buffer, sizeof(struct r_i_draw_range_t) * range_count);
    draw_list->range_count = range_count;
    draw_list->indexed = 0;
    draw_list->mesh = NULL;
//    draw_list->shader = cmd_buffer->shader;
//    draw_list->size = 1.0;
    draw_list->cmd_buffer = cmd_buffer;

    for(uint32_t index = 0; index < range_count; index++)
    {
        draw_list->ranges[index].shader = cmd_buffer->shader;
    }

    return draw_list;
}

struct r_i_mesh_t *r_i_AllocMesh(struct r_i_cmd_buffer_t *cmd_buffer, uint32_t vert_size, uint32_t vert_count, uint32_t index_count)
{
    struct r_i_mesh_t *mesh = NULL;

    if(!cmd_buffer)
    {
        cmd_buffer = &r_immediate_cmd_buffer;
    }

    mesh = r_AllocImmediateCmdData(cmd_buffer, sizeof(struct r_i_mesh_t));
    mesh->verts.count = vert_count;
    mesh->verts.stride = vert_size;
    mesh->verts.verts = r_AllocImmediateCmdData(cmd_buffer, vert_size * vert_count);
    mesh->indices.count = index_count;

    if(index_count)
    {
        mesh->indices.indices = r_AllocImmediateCmdData(cmd_buffer, sizeof(uint32_t) * index_count);
    }

    return mesh;
}

struct r_i_mesh_t *r_i_AllocMeshForModel(struct r_i_cmd_buffer_t *cmd_buffer, struct r_model_t *model)
{
    struct r_i_mesh_t *mesh = r_i_AllocMesh(cmd_buffer, sizeof(struct r_vert_t), model->verts.buffer_size, model->indices.buffer_size);
    memcpy(mesh->verts.verts, model->verts.buffer, sizeof(struct r_vert_t) * model->verts.buffer_size);

    if(model->indices.buffer_size)
    {
        memcpy(mesh->indices.indices, model->indices.buffer, sizeof(uint32_t) * model->indices.buffer_size);
    }

    return mesh;
}

void r_i_DrawBox(vec3_t *half_extents, vec4_t *color)
{
    vec3_t min = vec3_t_c(-half_extents->x, -half_extents->y, -half_extents->z);
    vec3_t max = vec3_t_c(half_extents->x, half_extents->y, half_extents->z);
//
    struct r_vert_t verts[] = {
        {.pos = vec3_t_c(min.x, max.y, min.z), .color = *color},
        {.pos = vec3_t_c(min.x, max.y, max.z), .color = *color},
        {.pos = vec3_t_c(max.x, max.y, max.z), .color = *color},
        {.pos = vec3_t_c(max.x, max.y, min.z), .color = *color},

        {.pos = vec3_t_c(min.x, min.y, min.z), .color = *color},
        {.pos = vec3_t_c(min.x, min.y, max.z), .color = *color},
        {.pos = vec3_t_c(max.x, min.y, max.z), .color = *color},
        {.pos = vec3_t_c(max.x, min.y, min.z), .color = *color},
    };

    uint32_t indices[] = {
        0, 1,
        1, 2,
        2, 3,
        3, 0,

        4, 5,
        5, 6,
        6, 7,
        7, 4,

        0, 4,
        1, 5,
        2, 6,
        3, 7
    };

    struct r_i_draw_list_t *draw_list = r_i_AllocDrawList(NULL, 1);
    draw_list->mesh = r_i_AllocMesh(NULL, sizeof(struct r_vert_t), 8, 24);
    memcpy(draw_list->mesh->verts.verts, verts, sizeof(verts));
    memcpy(draw_list->mesh->indices.indices, indices, sizeof(indices));
    draw_list->ranges[0].count = 24;
    draw_list->ranges[0].start = 0;
    draw_list->indexed = 1;
    draw_list->mode = GL_LINES;

    r_i_DrawList(NULL, draw_list);
}

void r_i_DrawVerts(struct r_i_cmd_buffer_t *cmd_buffer, struct r_vert_t *verts, uint32_t vert_count, uint32_t *indices, uint32_t index_count, uint32_t mode)
{
    struct r_i_draw_list_t *draw_list = r_i_AllocDrawList(cmd_buffer, 1);
    struct r_i_mesh_t *mesh = r_i_AllocMesh(cmd_buffer, sizeof(struct r_vert_t), vert_count, index_count);
    memcpy(mesh->verts.verts, verts, sizeof(struct r_vert_t) * vert_count);

    if(index_count)
    {
        memcpy(mesh->indices.indices, indices, sizeof(uint32_t) * index_count);
    }
    else
    {
        index_count = vert_count;
    }

    draw_list->ranges[0].start = 0;
    draw_list->ranges[0].count = index_count;
    draw_list->mode = mode;
    draw_list->indexed = index_count;
    draw_list->mesh = mesh;

    r_i_DrawList(cmd_buffer, draw_list);
}

void r_i_DrawLine(struct r_i_cmd_buffer_t *cmd_buffer, vec3_t *start, vec3_t *end, vec4_t *color)
{
    struct r_vert_t verts[2] = {
        [0] = {
            .pos = *start,
            .color = *color
        },
        [1] = {
            .pos = *end,
            .color = *color
        }
    };

    r_i_DrawVerts(cmd_buffer, verts, 2, NULL, 0, GL_LINES);
}

void r_i_DrawPoint(struct r_i_cmd_buffer_t *cmd_buffer, vec3_t *pos, vec4_t *color)
{

}

//struct r_i_indices_t *r_i_AllocIndices(struct r_i_cmd_buffer_t *cmd_buffer, uint32_t index_count)
//{
//    if(!cmd_buffer)
//    {
//        cmd_buffer = &r_immediate_cmd_buffer;
//    }
//
//    struct r_i_indices_t *indices = r_AllocImmediateCmdData(cmd_buffer, sizeof(struct r_i_indices_t));
//    indices->indices = r_AllocImmediateCmdData(cmd_buffer, sizeof(uint32_t) * index_count);
//    indices->count = index_count;
//
//    return indices;
//}
//
//struct r_i_verts_t *r_i_AllocVerts(struct r_i_cmd_buffer_t *cmd_buffer, uint32_t vert_count, uint32_t vert_size)
//{
//    if(!cmd_buffer)
//    {
//        cmd_buffer = &r_immediate_cmd_buffer;
//    }
//
//    struct r_i_verts_t *verts = r_AllocImmediateCmdData(cmd_buffer, sizeof(struct r_i_verts_t));
//    verts->verts = r_AllocImmediateCmdData(cmd_buffer, vert_count * vert_size);
//    verts->count = vert_count;
//    verts->stride = vert_size;
//    return verts;
//}



