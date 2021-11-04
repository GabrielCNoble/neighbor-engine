#ifndef DS_LIST_H
#define DS_LIST_H

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
    ds_list - simple growable list

    INTEGRATION:

        Integrating this library works similarly to the stb libraries. To create the
        implementations you do the following in ONE C or C++ file:

            #define DS_LIST_IMPLEMENTATION
            #include "ds_list.h"


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

struct ds_list_t
{
    void **buffers;
    uint32_t buffer_size;
    uint32_t elem_size;
    uint32_t cursor;
    uint32_t size;
};

#ifdef __cplusplus
extern "C"
{
#endif

struct ds_list_t ds_list_create(uint32_t elem_size, uint32_t buffer_size);

struct ds_list_t ds_list_copy(struct ds_list_t *source);

void ds_list_destroy(struct ds_list_t *list);

void ds_list_expand(struct ds_list_t *list, uint32_t elem_count);

void *ds_list_get_element(struct ds_list_t *list, uint32_t index);

void *ds_list_get_last_element(struct ds_list_t *list);

uint32_t ds_list_add_element(struct ds_list_t *list, void *element);

void ds_list_remove_element(struct ds_list_t *list, uint32_t index);

uint32_t ds_list_find_element(struct ds_list_t *list, void *element);

void ds_list_qsort_rec(struct ds_list_t *list, uint32_t left, uint32_t right, int32_t (*compare)(void *a, void *b));

void ds_list_qsort(struct ds_list_t *list, int32_t (*compare)(void *a, void *b));

#ifdef __cplusplus
}
#endif

#ifdef DS_LIST_IMPLEMENTATION

#ifdef __cplusplus
extern "C"
{
#endif

struct ds_list_t ds_list_create(uint32_t elem_size, uint32_t buffer_size)
{
    struct ds_list_t list;
    memset(&list, 0, sizeof(struct ds_list_t));

    /* add two extra elements at the start of the first buffer, to serve
    as storage for the pivot and for the temporary element when sorting
    the list */
    list.buffer_size = buffer_size + 2;
    list.elem_size = elem_size;

    ds_list_expand(&list, 1);
    list.buffer_size -= 2;
    list.size -= 2;

    /* the first buffer will get its pointer advanced forward by
    two elements, so the storage for the pivot stays at -2 position, and
    the temporary element stays at -1 */
    list.buffers[0] = (char *)list.buffers[0] + list.elem_size * 2;
    return list;
}

struct ds_list_t ds_list_copy(struct ds_list_t *source)
{
    struct ds_list_t list;

    list = ds_list_create(source->elem_size, source->buffer_size);

    if(source->size > list.size)
    {
        ds_list_expand(&list, source->size - list.size);
    }

    list.cursor = source->cursor;

    if(source->cursor)
    {
        uint32_t buffer_index = 0;
        uint32_t buffer_copy_size = source->elem_size * source->buffer_size;

        uint32_t full_buffer_count = source->cursor / source->buffer_size;

        for(; buffer_index < full_buffer_count; buffer_index++)
        {
            memcpy(list.buffers[buffer_index], source->buffers[buffer_index], buffer_copy_size);
        }

        uint32_t rest_size = source->elem_size * (source->cursor % source->buffer_size);

        if(rest_size)
        {
            memcpy(list.buffers[buffer_index], source->buffers[buffer_index], rest_size);
        }
    }

    return list;
}

void ds_list_destroy(struct ds_list_t *list)
{
    if(list->buffers)
    {
        list->buffers[0] = (char *)list->buffers[0] - list->elem_size * 2;
        for(uint32_t i = 0; i < list->size / list->buffer_size; i++)
        {
            DS_FREE(list->buffers[i]);
        }

        DS_FREE(list->buffers);

        list->buffers = NULL;
        list->cursor = 0;
        list->elem_size = 0;
        list->size = 0;
    }
}

void ds_list_expand(struct ds_list_t *list, uint32_t elem_count)
{
    void **buffers;
    uint32_t buffer_count;
    uint32_t list_buffer_count;

    elem_count = (elem_count + list->buffer_size - 1) & (~(list->buffer_size - 1));

    if(elem_count % list->buffer_size)
    {
        elem_count += list->buffer_size - elem_count % list->buffer_size;
    }

    buffer_count = elem_count / list->buffer_size;
    list_buffer_count = list->size / list->buffer_size;
    list->size += elem_count;
    buffers = (void**)DS_CALLOC(list->size / list->buffer_size, sizeof(void *));
    if(list->buffers)
    {
        memcpy(buffers, list->buffers, sizeof(void *) * list_buffer_count);
        DS_FREE(list->buffers);
    }

    for(uint32_t i = 0; i < buffer_count; i++)
    {
        buffers[i + list_buffer_count] = DS_CALLOC(list->buffer_size, list->elem_size);
    }

    list->buffers = buffers;
}

void *ds_list_get_element(struct ds_list_t *list, uint32_t index)
{
    char *buffer;
    void *element = NULL;

    if(index < list->cursor)
    {
        buffer = (char*)list->buffers[index / list->buffer_size];
        element = buffer + (index % list->buffer_size) * list->elem_size;
    }

    return element;
}

void *ds_list_get_last_element(struct ds_list_t *list)
{
    if(list->cursor)
    {
        return ds_list_get_element(list, list->cursor - 1);
    }

    return NULL;
}

uint32_t ds_list_add_element(struct ds_list_t *list, void *element)
{
    uint32_t index = 0xffffffff;
    char *buffer;

    index = list->cursor++;
    if(index >= list->size)
    {
        /* this will add an extra buffer... */
        ds_list_expand(list, 1);
    }

    if(element)
    {
        buffer = (char*)list->buffers[index / list->buffer_size];
        memcpy(buffer + (index % list->buffer_size) * list->elem_size, element, list->elem_size);
    }

    return index;
}

void ds_list_remove_element(struct ds_list_t *list, uint32_t index)
{
    char *buffer0;
    char *buffer1;
    uint32_t last_index;

    if(index < list->cursor)
    {
        if(index < list->cursor - 1)
        {
            last_index = list->cursor - 1;
            buffer0 = (char*)list->buffers[index / list->buffer_size];
            buffer0 += list->elem_size * (index % list->buffer_size);

            buffer1 = (char*)list->buffers[last_index / list->buffer_size];
            buffer1 += list->elem_size * (last_index % list->buffer_size);

            memcpy(buffer0, buffer1, list->elem_size);
        }

        list->cursor--;
    }
}

uint32_t ds_list_find_element(struct ds_list_t *list, void *element)
{
    for(uint32_t element_index = 0; element_index < list->cursor; element_index++)
    {
        if(!memcmp(element, ds_list_get_element(list, element_index), list->elem_size))
        {
            return element_index;
        }
    }

    return 0xffffffff;
}

void ds_list_qsort_rec(struct ds_list_t *list, uint32_t left, uint32_t right, int32_t (*compare)(void *a, void *b))
{
    uint32_t cur_left = left;
    uint32_t cur_right = right;
    void *middle = (char *)list->buffers[0] - list->elem_size * 2;
    void *temp = (char *)list->buffers[0] - list->elem_size;
    void *left_elem;
    void *right_elem;
    memcpy(middle, ds_list_get_element(list, (right + left) / 2), list->elem_size);

    while(1)
    {
        while(compare(ds_list_get_element(list, cur_left), middle) < 0) cur_left++;
        while(compare(middle, ds_list_get_element(list, cur_right)) < 0) cur_right--;
        if(cur_left >= cur_right) break;

        left_elem = ds_list_get_element(list, cur_left);
        right_elem = ds_list_get_element(list, cur_right);

        memcpy(temp, left_elem, list->elem_size);
        memcpy(left_elem, right_elem, list->elem_size);
        memcpy(right_elem, temp, list->elem_size);

        cur_left++;
        cur_right--;
    }

    if(left < cur_right) ds_list_qsort_rec(list, left, cur_right, compare);
    if(cur_left < right) ds_list_qsort_rec(list, cur_right + 1, right, compare);
}

void ds_list_qsort(struct ds_list_t *list, int32_t (*compare)(void *a, void *b))
{
    if(list->cursor)
    {
        ds_list_qsort_rec(list, 0, list->cursor - 1, compare);
    }
}

#ifdef __cplusplus
}
#endif

#endif /* DS_LIST_IMPLEMENTATION */


#endif /* DS_LIST_H */
