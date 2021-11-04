#ifndef DS_SLIST_H
#define DS_SLIST_H

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
    ds_slist - simple growable list

    INTEGRATION:

        Integrating this library works similarly to the stb libraries. To create the
        implementations you do the following in ONE C or C++ file:

            #define DS_SLIST_IMPLEMENTATION
            #include "ds_slist.h"


    ABOUT:

        This file provides a simple and generic list-like container. 
    

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

#ifndef DS_FREE
    #define DS_FREE free
#endif

struct ds_slist_t
{
    void **buffers;
    uint32_t buffer_size;
    uint32_t elem_size;
    uint32_t cursor;
    uint32_t size;
    uint32_t used;

    uint32_t *free_stack;
    uint32_t free_stack_top;
};

#ifdef __cplusplus
extern "C"
{
#endif

struct ds_slist_t ds_slist_create(uint32_t elem_size, uint32_t buffer_size);

void ds_slist_destroy(struct ds_slist_t *slist);

void ds_slist_expand(struct ds_slist_t *slist, uint32_t elem_count);

void *ds_slist_get_element(struct ds_slist_t *slist, uint32_t index);

uint32_t ds_slist_add_element(struct ds_slist_t *slist, void *element);

void ds_slist_remove_element(struct ds_slist_t *slist, uint32_t index);

#ifdef __cplusplus
}
#endif

#ifdef DS_SLIST_IMPLEMENTATION

#ifdef __cplusplus
extern "C"
{
#endif

struct ds_slist_t ds_slist_create(uint32_t elem_size, uint32_t buffer_size)
{
    struct ds_slist_t slist;
    memset(&slist, 0, sizeof(struct ds_slist_t));

    slist.buffer_size = buffer_size;
    slist.elem_size = elem_size;
    slist.free_stack_top = 0xffffffff;
    slist.used = 0;

    ds_slist_expand(&slist, 1);

    return slist;
}

void ds_slist_destroy(struct ds_slist_t *slist)
{
    uint32_t buffer_count = slist->size / slist->buffer_size;

    for(uint32_t i = 0; i < buffer_count; i++)
    {
        DS_FREE(slist->buffers[i]);
    }

    DS_FREE(slist->free_stack);
}

void ds_slist_expand(struct ds_slist_t *slist, uint32_t elem_count)
{
    void **buffers;
    uint32_t *free_stack;
    uint32_t buffer_count;
    uint32_t list_buffer_count;

    elem_count = (elem_count + slist->buffer_size - 1) & (~(slist->buffer_size - 1));

    if(elem_count % slist->buffer_size)
    {
        elem_count += slist->buffer_size - elem_count % slist->buffer_size;
    }

    buffer_count = elem_count / slist->buffer_size;
    list_buffer_count = slist->size / slist->buffer_size;

    slist->size += elem_count;

    buffers = (void**)DS_CALLOC(slist->size, sizeof(void *));
    free_stack = (uint32_t*)DS_CALLOC(slist->size, sizeof(uint32_t));

    if(slist->buffers)
    {
        memcpy(buffers, slist->buffers, sizeof(void *) * list_buffer_count);
        DS_FREE(slist->buffers);
        DS_FREE(slist->free_stack);
    }

    for(uint32_t i = 0; i < buffer_count; i++)
    {
        buffers[i + list_buffer_count] = DS_CALLOC(slist->buffer_size, slist->elem_size);
    }

    slist->buffers = buffers;
    slist->free_stack = free_stack;
}

void *ds_slist_get_element(struct ds_slist_t *slist, uint32_t index)
{
    char *buffer;
    void *element = NULL;

    if(index < slist->size)
    {
        buffer = (char*)slist->buffers[index / slist->buffer_size];
        element = buffer + (index % slist->buffer_size) * slist->elem_size;
    }

    return element;
}

uint32_t ds_slist_add_element(struct ds_slist_t *slist, void *element)
{
    uint32_t index = 0xffffffff;
    char *buffer;

    if(slist->free_stack_top < 0xffffffff)
    {
        index = slist->free_stack[slist->free_stack_top];
        slist->free_stack_top--;
    }
    else
    {
        index = slist->cursor++;

        if(index >= slist->size)
        {
            /* this will add an extra buffer... */
            ds_slist_expand(slist, 1);
        }
    }

    if(element && index < 0xffffffff)
    {
        buffer = (char*)slist->buffers[index / slist->buffer_size];
        memcpy(buffer + (index % slist->buffer_size) * slist->elem_size, element, slist->elem_size);
    }
    
    slist->used++;

    return index;
}

void ds_slist_remove_element(struct ds_slist_t *slist, uint32_t index)
{
    if(index < slist->cursor)
    {
        slist->free_stack_top++;
        slist->free_stack[slist->free_stack_top] = index;
        slist->used--;
    }
}

#ifdef __cplusplus
}
#endif

#endif

#endif
