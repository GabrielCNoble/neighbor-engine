#ifndef MEM_H
#define MEM_H

#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#define MEM_GUARD_POINTERS 24

//#define INSTRUMENT_MEMORY
//#define CHECK_DOUBLE_FREE
//#define RECORD_REALLOCS

struct mem_realloc_t
{
    struct mem_realloc_t *next;
    uint32_t line;
    uint32_t prev_size;
    uint32_t new_size;
    char file[256];
};

struct mem_header_t
{
    void *guard[MEM_GUARD_POINTERS];
    size_t size;
    uint32_t freed;
    uint32_t line;
    char file[256];
    struct mem_header_t *next;
    struct mem_header_t *prev;
    
    #ifdef RECORD_REALLOCS
    uint32_t realloc_count;
    struct mem_realloc_t *reallocs;
    struct mem_realloc_t *last_realloc;
    #endif
};

struct mem_tail_t
{
    void *guard[MEM_GUARD_POINTERS];
};


#ifdef __cplusplus
extern "C"
{
#endif

struct mem_header_t *mem_GetAllocHeader(void *memory);

struct mem_tail_t *mem_GetAllocTail(void *memory);

void *mem_InitHeaderAndTail(void *memory, uint32_t size, uint32_t line, char *file);

void mem_CheckGuardImp(void *memory);

void mem_CheckGuardsImp();

void *mem_MallocImp(size_t size, uint32_t line, char *file);

void *mem_CallocImp(size_t num, size_t size, uint32_t line, char *file);

void *mem_ReallocImp(void *memory, uint32_t new_size, uint32_t line, char *file);

void mem_FreeImp(void *memory, uint32_t line, char *file);

void mem_CheckCommitmentImp();

#ifdef __cplusplus
}
#endif

#ifdef INSTRUMENT_MEMORY

#define mem_CheckGuard(memory) mem_CheckGuardImp(memory)

#define mem_CheckGuards() mem_CheckGuardsImp()

#define mem_Malloc(size) mem_MallocImp(size, __LINE__, __FILE__)

#define mem_Calloc(num, size) mem_CallocImp(num, size, __LINE__, __FILE__)

#define mem_Realloc(memory, new_size) mem_ReallocImp(memory, new_size, __LINE__, __FILE__)

#define mem_Free(memory) mem_FreeImp(memory, __LINE__, __FILE__)

#define mem_CheckCommitment mem_CheckCommitmentImp

#else

#define mem_CheckGuard(memory)

#define mem_CheckGuards()

#define mem_Malloc(size) malloc(size)

#define mem_Calloc(num, size) calloc(num, size)

#define mem_Realloc(memory, size) realloc(memory, size)

#define mem_Free(memory) free(memory)

#define mem_CheckCommitment()

#endif

#ifdef DS_MEMORY_IMPLEMENTATION

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct mem_header_t *mem_headers = NULL;
struct mem_header_t *mem_last_header = NULL;

#ifdef __cplusplus
extern "C"
{
#endif

struct mem_header_t *mem_GetAllocHeader(void *memory)
{
    return (struct mem_header_t *)((char *)memory - sizeof(struct mem_header_t));
}

struct mem_tail_t *mem_GetAllocTail(void *memory)
{
    struct mem_header_t *header;
    header = mem_GetAllocHeader(memory);
    return (struct mem_tail_t *)((char *)memory + header->size);
}

void *mem_InitHeaderAndTail(void *memory, uint32_t size, uint32_t line, char *file)
{
    struct mem_header_t *header;
    struct mem_tail_t *tail;

    header = memory;
    header->size = size;
    header->line = line;
    strcpy(header->file, file);

    for(uint32_t i = 0; i < MEM_GUARD_POINTERS; i++)
    {
        header->guard[i] = memory;
    }

    memory = (char *)memory + sizeof(struct mem_header_t);

    tail = (struct mem_tail_t *)((char *)memory + size);

    for(uint32_t i = 0; i < MEM_GUARD_POINTERS; i++)
    {
        tail->guard[i] = header->guard[i];
    }

    if(!mem_headers)
    {
        mem_headers = header;
    }
    else
    {
        mem_last_header->next = header;
        header->prev = mem_last_header;
    }
    mem_last_header = header;

    return memory;
}

void mem_CheckGuardImp(void *memory)
{
    struct mem_header_t *header;
    struct mem_tail_t *tail;

    header = mem_GetAllocHeader(memory);
    tail = mem_GetAllocTail(memory);

    for(uint32_t i = 0; i < MEM_GUARD_POINTERS; i++)
    {
        if(header->guard[i] != header)
        {
            printf("mem_CheckGuard: allocation at line %d, file %s has corrupt header guard bytes!\n", header->line, header->file);
            break;
        }
    }

    for(uint32_t i = 0; i < MEM_GUARD_POINTERS; i++)
    {
        if(tail->guard[i] != header)
        {
            printf("mem_CheckGuard: allocation at line %d, file %s has corrupt tail guard bytes!\n", header->line, header->file);
            break;
        }
    }
}

void mem_CheckGuardsImp()
{
    struct mem_header_t *header;
    struct mem_header_t *prev_header;
    header = mem_headers;
    
    while(header)
    {
        mem_CheckGuard(header + 1);
        prev_header = header;
        header = header->next;
    }
}

void *mem_MallocImp(size_t size, uint32_t line, char *file)
{
    void *memory;
    memory = malloc(sizeof(struct mem_header_t) + sizeof(struct mem_tail_t) + size);
    return mem_InitHeaderAndTail(memory, size, line, file);
}

void *mem_CallocImp(size_t num, size_t size, uint32_t line, char *file)
{
    void *memory;
    size_t total_size = sizeof(struct mem_header_t) + sizeof(struct mem_tail_t) + size * num;
    memory = calloc(1, total_size);
    return mem_InitHeaderAndTail(memory, num * size, line, file);
}

void *mem_ReallocImp(void *memory, uint32_t new_size, uint32_t line, char *file)
{
    struct mem_header_t *header;
    struct mem_tail_t *new_tail;
    struct mem_tail_t *old_tail;
    if(memory)
    {        
        header = mem_GetAllocHeader(memory);
        header = realloc(header, sizeof(struct mem_header_t) + sizeof(struct mem_tail_t) + new_size);
        
        /* this may be a new allocation altogether, so update
        the prev and next pointes to point to this header */
        if(header->prev)
        {
            header->prev->next = header;
        }
        if(header->next)
        {
            header->next->prev = header;
        }
        
        /* size and allocation address may have changed, so the tail has to be updated */
        new_tail = (struct mem_tail_t *)((char *)header + sizeof(struct mem_header_t) + new_size);
        
        for(uint32_t guard_index = 0; guard_index < MEM_GUARD_POINTERS; guard_index++)
        {
            header->guard[guard_index] = header;
            new_tail->guard[guard_index] = header;
        }
    
        #ifdef RECORD_REALLOCS
        
        struct mem_realloc_t *realloc;
        realloc = calloc(1, sizeof(struct mem_realloc_t));
        realloc->line = line;
        realloc->prev_size = header->size;
        realloc->new_size = new_size;
        strcpy(realloc->file, file);
        
        if(!header->reallocs)
        {
            header->reallocs = realloc;
        }
        else
        {
            header->last_realloc->next = realloc;
        }
        
        header->last_realloc = realloc;
        
        #endif
        
        header->size = new_size;
        memory = header + 1;
    }
    else
    {
        memory = mem_CallocImp(1, new_size, line, file);
    }
    
    return memory;
}

void mem_FreeImp(void *memory, uint32_t line, char *file)
{
    struct mem_header_t *header = mem_GetAllocHeader(memory);
    printf("freeing allocation from (line %d, file %s) at (line %d, file %s)\n", header->line, header->file, line, file);
    mem_CheckGuardImp(memory);

    #ifdef CHECK_DOUBLE_FREE

    if(header->freed)
    {
        printf("double free of allocation from (line %d, file %s) at (line %d, file %s)\n", header->line, header->file, line, file);
        return;
    }
    else
    {
        header->freed = 1;
    }

    #else

    if(header->prev)
    {
        header->prev->next = header->next;
    }
    else
    {
        mem_headers = header->next;
    }

    if(header->next)
    {
        header->next->prev = header->prev;
    }
    else
    {
        mem_last_header = header->prev;
    }
    free(header);

    #endif
}

void mem_CheckCommitmentImp()
{
    struct mem_header_t *header = mem_headers;
    uint32_t total_committed = 0;
    while(header)
    {
        total_committed += header->size;
        header = header->next;
    }
    
    printf("total memory commited: %u bytes\n", total_committed);
}

#ifdef __cplusplus
}
#endif

#endif

#endif

