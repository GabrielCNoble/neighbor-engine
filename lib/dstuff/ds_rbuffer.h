#ifndef DS_RBUFFER_H
#define DS_RBUFFER_H

/*
    This is free and unencumbered software released into the public domain.

    Anyone is free to copy, modify, publish, use, compile, sell, or
    distribute this software, either in source code form or as a compiled
    binary, for any purpose, commercial or non-commercial, and by any
    means.

    In jurisdictions that recognize copyright laws, the author or authors
    of this software dedicate any and all copyright interest in the
    software to the public domain. We make this dedication for the benefit
    of the public at large and to the detriment of our heirs and
    successors. We intend this dedication to be an overt act of
    relinquishment in perpetuity of all present and future rights to this
    software under copyright law.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
    OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
    ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
    OTHER DEALINGS IN THE SOFTWARE.

    For more information, please refer to <http://unlicense.org/>
*/

/*
    ds_rbuffer - simple ring buffer

    INTEGRATION:

        Integrating this library works similarly to the stb libraries. To create the
        implementations you do the following in ONE C or C++ file:

            #define DS_RBUFFER_IMPLEMENTATION
            #include "ds_rbuffer.h"


    ABOUT:

        TODO
    

    BASIC USAGE:

        TODO
*/

#ifndef DS_STDINT_INCLUDED
    #include <stdint.h>
    #define DS_STDINT_INCLUDED
#endif

#ifndef DS_STRING_H_INCLUDED
    #include <string.h>
    #define DS_STRING_H_INCLUDED
#endif

#ifndef DS_LIMITS_H_INCLUDED
    #include <limits.h>
    #define DS_LIMITS_H_INCLUDED
#endif

#ifdef DS_STDDEF_H_INCLUDED
    #include <stddef.h>
    #define DS_STDDEF_H_INCLUDED
#endif

#ifndef DS_STDLIB_H_INCLUDED
    #include <stdlib.h>
    #define DS_STDLIB_H_INCLUDED
#endif

#ifndef DS_MALLOC
    #define DS_MALLOC malloc
#endif

#ifndef DS_CALLOC
    #define DS_CALLOC calloc
#endif

#ifndef DS_REALLOC
    #define DS_REALLOC realloc
#endif

#ifndef DS_FREE
    #define DS_FREE free
#endif

struct ds_rbuffer_t
{
    void *buffer;
    uint32_t elem_size;
    uint32_t buffer_size;
    uint32_t free_slots;
    uint32_t next_in;
    uint32_t next_out;
};


struct ds_rbuffer_t ds_rbuffer_create(uint32_t elem_size, uint32_t buffer_size);

void ds_rbuffer_destroy(struct ds_rbuffer_t *ringbuffer);

void ds_rbuffer_reset(struct ds_rbuffer_t *ringbuffer);

void ds_rbuffer_resize(struct ds_rbuffer_t *ringbuffer, uint32_t new_elem_count);

uint32_t ds_rbuffer_add_element(struct ds_rbuffer_t *ringbuffer, void *element);

void *ds_rbuffer_peek_element(struct ds_rbuffer_t *ringbuffer);

void *ds_rbuffer_get_element(struct ds_rbuffer_t *ringbuffer);


#ifdef DS_RBUFFER_IMPLEMENTATION

struct ds_rbuffer_t ds_rbuffer_create(uint32_t elem_size, uint32_t buffer_size)
{
    struct ds_rbuffer_t ringbuffer;
    memset(&ringbuffer, 0, sizeof(struct ds_rbuffer_t));

    ringbuffer.buffer_size = buffer_size;
    ringbuffer.elem_size = elem_size;
    ringbuffer.free_slots = buffer_size;
    ringbuffer.buffer = DS_CALLOC(elem_size, buffer_size);

    return ringbuffer;
}

void ds_rbuffer_destroy(struct ds_rbuffer_t *ringbuffer)
{
    if(ringbuffer)
    {
        DS_FREE(ringbuffer->buffer);
        ringbuffer->buffer = NULL;
        ringbuffer->buffer_size = 0;
        ringbuffer->elem_size = 0;
    }
}

void ds_rbuffer_reset(struct ds_rbuffer_t *ringbuffer)
{
    if(ringbuffer)
    {
        ringbuffer->next_in = 0;
        ringbuffer->next_out = 0;
    }
}

void ds_rbuffer_resize(struct ds_rbuffer_t *ringbuffer, uint32_t new_elem_count)
{
    if(ringbuffer)
    {
        uint32_t elem_count = ringbuffer->buffer_size / ringbuffer->elem_size;
        if(new_elem_count > elem_count)
        {
            uint32_t new_size = ringbuffer->elem_size * new_elem_count;
            ringbuffer->buffer = DS_REALLOC(ringbuffer->buffer, new_size);
            ringbuffer->free_slots += new_elem_count - elem_count;
            ringbuffer->buffer_size = new_size;
        }
    }
}

uint32_t ds_rbuffer_add_element(struct ds_rbuffer_t *ringbuffer, void *element)
{
    uint32_t index = 0xffffffff;

    if(ringbuffer->free_slots)
    {
        index = ringbuffer->next_in;

        if(element)
        {
            memcpy((char *)ringbuffer->buffer + index * ringbuffer->elem_size, element, ringbuffer->elem_size);
        }

        ringbuffer->next_in = (ringbuffer->next_in + 1) % ringbuffer->buffer_size;
        ringbuffer->free_slots--;
    }

    return index;
}

void *ds_rbuffer_peek_element(struct ds_rbuffer_t *ringbuffer)
{
    return (char *)ringbuffer->buffer + ringbuffer->next_out * ringbuffer->elem_size;
}

void *ds_rbuffer_get_element(struct ds_rbuffer_t *ringbuffer)
{
    void *element = NULL;

    if(ringbuffer->free_slots < ringbuffer->buffer_size)
    {
        element = (char *)ringbuffer->buffer + ringbuffer->next_out * ringbuffer->elem_size;
        ringbuffer->next_out = (ringbuffer->next_out + 1) % ringbuffer->buffer_size;
        ringbuffer->free_slots++;
    }

    return element;
}


#endif


#endif
