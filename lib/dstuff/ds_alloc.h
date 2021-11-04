#ifndef DS_ALLOC_H
#define DS_ALLOC_H

#include "ds_slist.h"
#include "ds_list.h"
#include <stdint.h>

/*
    TODO:

        -   allow the heap to grow by having multiple buffers (similar to
            stack_list_t or list_t
*/

struct ds_heap_t
{
    struct ds_slist_t alloc_chunks;
    struct ds_list_t free_chunks;
    uint32_t frees_since_defrag;
    uint32_t size;
};

struct ds_chunk_t
{
    uint32_t size;
    uint32_t start;
    uint32_t align;
    uint32_t index;
};

struct ds_chunk_h
{
    uint32_t index;
};
#define DS_INVALID_CHUNK_INDEX 0xffffffff
#define DS_CHUNK_HANDLE(index) (struct ds_chunk_h){index}
#define DS_INVALID_CHUNK_HANDLE DS_CHUNK_HANDLE(DS_INVALID_CHUNK_INDEX)

enum DS_ALLOC_HINT
{
    DS_ALLOC_HINT_FIT = 0,
    DS_ALLOC_HINT_CLOSE_ENOUGH,
};

struct ds_heap_t ds_create_heap(uint32_t size);

void ds_destroy_heap(struct ds_heap_t *heap);

void ds_reset_heap(struct ds_heap_t *heap);

struct ds_chunk_h ds_alloc_chunk(struct ds_heap_t *heap, uint32_t size, uint32_t align);

struct ds_chunk_h ds_alloc_chunk_min(struct ds_heap_t *heap, uint32_t size, uint32_t min_size, uint32_t align);

void ds_free_chunk(struct ds_heap_t *heap, struct ds_chunk_h handle);

struct ds_chunk_t *ds_get_chunk_pointer(struct ds_heap_t *heap, struct ds_chunk_h handle);

void ds_defrag_heap(struct ds_heap_t *heap);

#ifdef DS_ALLOC_IMPLEMENTATION

struct ds_heap_t ds_create_heap(uint32_t size)
{
    struct ds_heap_t heap = {};

    if(size)
    {
        heap.alloc_chunks = ds_slist_create(sizeof(struct ds_chunk_t), 128);
        heap.free_chunks = ds_list_create(sizeof(struct ds_chunk_t), 128);
        heap.size = size;
        heap.frees_since_defrag = 0;

        struct ds_chunk_t chunk;

        chunk.size = size;
        chunk.align = 0;
        chunk.start = 0;

        ds_list_add_element(&heap.free_chunks, &chunk);
    }

    return heap;
}

void ds_destroy_heap(struct ds_heap_t *heap)
{
    if(heap && heap->size)
    {
        ds_slist_destroy(&heap->alloc_chunks);
        ds_list_destroy(&heap->free_chunks);
        heap->size = 0;
    }
}

void ds_reset_heap(struct ds_heap_t *heap)
{
    if(heap && heap->size)
    {
        heap->alloc_chunks.cursor = 0;
        heap->alloc_chunks.free_stack_top = 0xffffffff;
        heap->free_chunks.cursor = 1;

        struct ds_chunk_t *chunk = ds_list_get_element(&heap->free_chunks, 0);
        chunk->start = 0;
        chunk->align = 0;
        chunk->size = heap->size;
    }
}

struct ds_chunk_h ds_alloc_chunk(struct ds_heap_t *heap, uint32_t size, uint32_t align)
{
    return ds_alloc_chunk_min(heap, size, size, align);
}

struct ds_chunk_h ds_alloc_chunk_min(struct ds_heap_t *heap, uint32_t size, uint32_t min_size, uint32_t align)
{
    struct ds_chunk_t *chunk;
    struct ds_chunk_t new_chunk;
    struct ds_chunk_h chunk_handle = DS_INVALID_CHUNK_HANDLE;
    uint32_t chunk_align;
    uint32_t chunk_size;
    uint32_t defragd = 0;
    uint32_t chunk_index = 0;
    uint32_t best_size_index;
    uint32_t best_size;

    _try_again:

    best_size = 0;
    best_size_index = 0xffffffff;

    for(chunk_index = 0; chunk_index < heap->free_chunks.cursor; chunk_index++)
    {
        chunk = ds_list_get_element(&heap->free_chunks, chunk_index);
        chunk_align = 0;

        if(align && (chunk->start % align))
        {
            chunk_align = align - chunk->start % align;
        }

        /* usable size after taking alignment into consideration */
        chunk_size = chunk->size - chunk_align;

        if(chunk_size >= size)
        {
            best_size = chunk_size;
            best_size_index = chunk_index;
            break;
        }

        if(chunk_size > best_size)
        {
            best_size = chunk_size;
            best_size_index = chunk_index;
        }
    }

    if(size != min_size && chunk_index >= heap->free_chunks.cursor)
    {
        size = best_size;
        chunk_size = best_size;
        chunk_index = best_size_index;
    }

    if(chunk_index < heap->free_chunks.cursor)
    {
        if(chunk_size == size)
        {
            /* taking alignment into consideration this chunk has the exact size
            of the required allocation */

            chunk->align = chunk_align;
            /* the start of the chunk will move chunk_align bytes forward */
            chunk->start += chunk_align;
            /* the chunk will 'shrink', since its start point moved forward by
            chunk_align bytes, the usable space will decrease by chunk_align bytes */
            chunk->size = chunk_size;

            /* this chunk fits the request perfectly, so move it from the free list
            to the alloc list */
            chunk_handle.index = ds_slist_add_element(&heap->alloc_chunks, chunk);
            ds_list_remove_element(&heap->free_chunks, chunk_index);
        }
        else
        {
            /* this chunk is bigger even after accounting for alignment, so chop off the
            beginning part of this chunk, and adjust its size accordingly */
            new_chunk.align = chunk_align;
            new_chunk.start = chunk->start + chunk_align;
            /* since this chunk's size is bigger even after adjusting the start point,
            the allocated chunk size remains the same */
            new_chunk.size = size;

            chunk_handle.index = ds_slist_add_element(&heap->alloc_chunks, &new_chunk);
            /* new_chunk start moved forward chunk_align bytes, which means its
            end will move forward chunk_align bytes */
            chunk->start += size + chunk_align;
            /* also, since its end moved forward chunk_align bytes, the sliced chunk's
            usable size also shrinks by chunk_align bytes */
            chunk->size -= size + chunk_align;
        }
    }

    if(chunk_handle.index == DS_INVALID_CHUNK_INDEX && !defragd)
    {
        ds_defrag_heap(heap);
        defragd = 1;
        goto _try_again;
    }

    return chunk_handle;
}

void ds_free_chunk(struct ds_heap_t *heap, struct ds_chunk_h handle)
{
    struct ds_chunk_t *chunk = NULL;

    chunk = ds_get_chunk_pointer(heap, handle);

    if(heap && chunk)
    {
        ds_list_add_element(&heap->free_chunks, chunk);
        ds_slist_remove_element(&heap->alloc_chunks, handle.index);
        chunk->size = 0;
        heap->frees_since_defrag++;

        if(heap->frees_since_defrag > 64)
        {
            ds_defrag_heap(heap);
        }
    }
}

struct ds_chunk_t *ds_get_chunk_pointer(struct ds_heap_t *heap, struct ds_chunk_h handle)
{
    struct ds_chunk_t *chunk = NULL;

    if(heap)
    {
        chunk = ds_slist_get_element(&heap->alloc_chunks, handle.index);

        if(chunk && !chunk->size)
        {
            chunk = NULL;
        }
    }

    return chunk;
}

int32_t ds_cmp_chunks(void *a, void *b)
{
    struct ds_chunk_t *chunk_a = a;
    struct ds_chunk_t *chunk_b = b;
    return (int32_t)(chunk_a->start - chunk_b->start);
}

void ds_defrag_heap(struct ds_heap_t *heap)
{
    struct ds_chunk_t *first_chunk;
    struct ds_chunk_t *second_chunk;

    if(heap)
    {
        uint32_t total_defragd = 0;
        uint32_t prev_free_chunks = heap->free_chunks.cursor;

        ds_list_qsort(&heap->free_chunks, ds_cmp_chunks);

        for(uint32_t first_index = 0; first_index < heap->free_chunks.cursor; first_index++)
        {
            first_chunk = ds_list_get_element(&heap->free_chunks, first_index);

            for(uint32_t second_index = first_index + 1; second_index < heap->free_chunks.cursor; second_index++)
            {
                second_chunk = ds_list_get_element(&heap->free_chunks, second_index);

                if(first_chunk->start + first_chunk->size != second_chunk->start - second_chunk->align)
                {
                    first_index = second_index - 1;
                    break;
                }
                total_defragd += second_chunk->size + second_chunk->align;
                first_chunk->size += second_chunk->size + second_chunk->align;
                second_chunk->size = 0;
            }
        }

        for(uint32_t chunk_index = 0; chunk_index < heap->free_chunks.cursor; chunk_index++)
        {
            first_chunk = ds_list_get_element(&heap->free_chunks, chunk_index);

            if(!first_chunk->size)
            {
                ds_list_remove_element(&heap->free_chunks, chunk_index);
                chunk_index--;
            }
        }

        heap->frees_since_defrag = 0;
    }
}

#endif

#endif // DS_ALLOC_H
