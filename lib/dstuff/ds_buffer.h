#ifndef DS_BUFFER_H
#define DS_BUFFER_H

#include "ds_mem.h"

struct ds_buffer_t
{
    uint32_t elem_size;
    uint32_t buffer_size;
    void *buffer;
};

struct ds_buffer_t ds_buffer_create(uint32_t elem_size, uint32_t elem_count);

struct ds_buffer_t ds_buffer_copy(struct ds_buffer_t *source);

void ds_buffer_destroy(struct ds_buffer_t *buffer);

void ds_buffer_resize(struct ds_buffer_t *buffer, uint32_t new_size);

void ds_buffer_fill(struct ds_buffer_t *buffer, uint32_t offset, void *data, uint32_t count);

void ds_buffer_shift(struct ds_buffer_t *buffer, int32_t offset, int32_t shift);

#ifdef DS_BUFFER_IMPLEMENTATION

struct ds_buffer_t ds_buffer_create(uint32_t elem_size, uint32_t elem_count)
{
    struct ds_buffer_t buffer = {};
    buffer.elem_size = elem_size;
    ds_buffer_resize(&buffer, elem_count);
    return buffer;
}

struct ds_buffer_t ds_buffer_copy(struct ds_buffer_t *source)
{
    struct ds_buffer_t buffer;
    buffer = ds_buffer_create(source->elem_size, source->buffer_size);
    memcpy(buffer.buffer, source->buffer, source->buffer_size * source->elem_size);
    return buffer;
}

void ds_buffer_destroy(struct ds_buffer_t *buffer)
{
    if(buffer && buffer->buffer)
    {
        mem_Free(buffer->buffer);
        buffer->buffer_size = 0;
        buffer->elem_size = 0;
    }
}

void ds_buffer_resize(struct ds_buffer_t *buffer, uint32_t new_size)
{
    if(buffer && new_size)
    {
        buffer->buffer = mem_Realloc(buffer->buffer, buffer->elem_size * new_size);
        buffer->buffer_size = new_size;
    }
}

void ds_buffer_fill(struct ds_buffer_t *buffer, uint32_t offset, void *data, uint32_t count)
{
    if(buffer && data && count)
    {
        if(buffer->buffer_size < offset + count)
        {
            ds_buffer_resize(buffer, offset + count);
        }

        memcpy((char *)buffer->buffer + buffer->elem_size * offset, data, buffer->elem_size * count);
    }
}

void ds_buffer_shift(struct ds_buffer_t *buffer, int32_t offset, int32_t shift)
{
    if(buffer && shift && offset >= 0)
    {
        if(offset + shift > 0)
        {
            uint32_t src_offset = offset * buffer->elem_size;
            uint32_t dst_offset = (offset + shift) * buffer->elem_size;
            uint32_t copy_size = (buffer->buffer_size - offset) * buffer->elem_size;
            ds_buffer_resize(buffer, buffer->buffer_size + shift);

            if(copy_size)
            {
                memcpy((char *)buffer->buffer + dst_offset, (char *)buffer->buffer + src_offset, copy_size);
            }
        }
    }
}

void ds_buffer_append(struct ds_buffer_t *buffer, int32_t count, void *data)
{
    if(buffer && count && data)
    {
        ds_buffer_resize(buffer, buffer->buffer_size + count);
        memcpy((char *)buffer->buffer + buffer->buffer_size * buffer->elem_size, data, buffer->buffer_size * count);
    }
}

#endif

#endif // DS_BUFFER_H






