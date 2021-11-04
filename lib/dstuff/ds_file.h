#ifndef FILE_H
#define FILE_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

//struct ds_record_data_t
//{
//    char name[64];
//    uint32_t size;
//    char data[];
//};
//
//struct ds_record_t
//{
//    struct ds_record_t *next;
//    struct ds_record_t *prev;
//    struct ds_record_data_t data;
//};

struct ds_data_buffer_t
{
    struct ds_data_buffer_t *next;
    uint32_t size;
    uint8_t data[];
};

struct ds_info_t
{
    char name[64];
    uint32_t size;
    uint32_t offset;
};

struct ds_section_t
{
    struct ds_info_t info;
    struct ds_data_buffer_t *buffers;
    struct ds_data_buffer_t *last;
};

static char ds_registry_tag[] = "[registry]";

struct ds_registry_t 
{
    char tag[(sizeof(ds_registry_tag) + 3)&(~3)]; 
    uint32_t section_count;
    struct ds_info_t sections[];
};


long file_size(FILE *file);

void read_file(FILE *file, void **buffer, uint32_t *buffer_size);

void write_file(void **buffer, long *buffer_size);

int file_exists(char *file_name);


//struct ds_record_t *ds_append_record(struct ds_section_t *section, char *record_name, uint32_t size, void *data);

//void ds_drop_record(struct ds_section_t *section, char *record_name);

//struct ds_record_t *ds_find_record(struct ds_section_t *section, char *record_name);

void *ds_append_data(struct ds_section_t *section, uint32_t size, void *data);

void ds_free_section(struct ds_section_t *section);

void ds_serialize_sections(void **buffer, uint32_t *buffer_size, uint32_t section_count, struct ds_section_t **sections);

//void ds_serialize_section(struct ds_section_t *section, void **buffer, uint32_t *buffer_size);

void ds_get_section_data(void *buffer, char *section_name, void **data, uint32_t *size);

//struct ds_section_t ds_unserialize_section(void **buffer);


#ifdef DS_FILE_IMPLEMENTATION

#include "ds_mem.h"
#include <stdarg.h>

long file_size(FILE *file)
{
    long size;
    long offset;

    offset = ftell(file);
    fseek(file, 0, SEEK_END);
    size = ftell(file);
    fseek(file, offset, SEEK_SET);

    return size;
}

void read_file(FILE *file, void **buffer, uint32_t *buffer_size)
{
    char *file_buffer = NULL;
    long size = 0;

    if(file)
    {
        size = file_size(file);
        file_buffer = (char *)mem_Calloc(size + 1, 1);
        fread(file_buffer, size, 1, file);
        file_buffer[size] = '\0';
    }

    *buffer = (void *)file_buffer;
    if(buffer_size)
    {
        *buffer_size = (uint32_t)size;
    }
}

void write_file(void **buffer, long *buffer_size)
{

}

int file_exists(char *file_name)
{
    FILE *file;

    file = fopen(file_name, "r");

    if(file)
    {
        fclose(file);
        return 1;
    }

    return 0;
}

void *ds_append_data(struct ds_section_t *section, uint32_t size, void *data) 
{
    if(section && size)
    {
        struct ds_data_buffer_t *buffer = mem_Calloc(1, sizeof(struct ds_data_buffer_t) + size);
        
        if(data)
        {
            memcpy(&buffer->data, data, size);
        }
        
        buffer->size = size;
        if(!section->buffers)
        {
            section->buffers = buffer;
        }
        else
        {
            section->last->next = buffer;
        }
        section->last = buffer;
        section->info.size += size;
        
        return &buffer->data;
    }
    
    return NULL;
}

void ds_free_section(struct ds_section_t *section)
{
    struct ds_data_buffer_t *buffer = section->buffers;
    
    while(buffer)
    {
        struct ds_data_buffer_t *next = buffer->next;
        mem_Free(buffer);
        buffer = next;
    }
    
    section->buffers = NULL;
    section->last = NULL;
    section->info.size = 0;
}

void ds_serialize_sections(void **buffer, uint32_t *buffer_size, uint32_t section_count, struct ds_section_t **sections)
{
    uint32_t output_size;
    char *output;
    uint32_t current_offset = 0;
    output_size = sizeof(struct ds_registry_t) + sizeof(struct ds_info_t) * section_count;
    for(uint32_t section_index = 0; section_index < section_count; section_index++)
    {
        struct ds_section_t *section = sections[section_index];
        output_size += section->info.size;
    }
    
    output = mem_Calloc(1, output_size);
    *buffer = output;
    *buffer_size = output_size;
    struct ds_registry_t *registry = (struct ds_registry_t *)output;
    output += sizeof(struct ds_registry_t) + sizeof(struct ds_info_t) * section_count;
    registry->section_count = section_count;
    strcpy(registry->tag, ds_registry_tag);
    
    for(uint32_t section_index = 0; section_index < section_count; section_index++)
    {
        struct ds_section_t *section = sections[section_index];
        struct ds_info_t *info = registry->sections + section_index;
        memcpy(info, &section->info, sizeof(struct ds_info_t));
        info->offset = output - (char *)registry;
        struct ds_data_buffer_t *buffer = section->buffers;
        
        while(buffer) 
        {
            memcpy(output, buffer->data, buffer->size);
            output += buffer->size;
            buffer = buffer->next;
        }
    }
}

void ds_get_section_data(void *buffer, char *section_name, void **data, uint32_t *size)
{
    char *input = (char *)buffer;
    
    if(!strcmp(input, ds_registry_tag))
    {
        if(size)
        {
            *size = 0;
        }
        
        *data = NULL;
        struct ds_registry_t *registry = (struct ds_registry_t *)buffer;
        
        for(uint32_t section_index = 0; section_index < registry->section_count; section_index++)
        {
            struct ds_info_t *info = registry->sections + section_index;
            if(!strcmp(info->name, section_name))
            {
                if(size)
                {
                    *size = info->size;
                }
                *data = input + info->offset;
                return;
            }
        }
    }
}

//void ds_serialize_section(struct ds_section_t *section, void **buffer, uint32_t *buffer_size)
//{
//    uint32_t size = 0;
//    char *output;
//    size += sizeof(struct ds_section_data_t) + section->data.size;
//    
//    output = mem_Calloc(1, size);
//    *buffer = output;
//    *buffer_size = size;
//    memcpy(output, &section->data, sizeof(struct ds_section_data_t));
//    output += sizeof(struct ds_section_data_t);
//    
//    struct ds_section_buffer_t *section_buffer = section->buffers;
//    while(section_buffer)
//    {
//        memcpy(output, &section_buffer->data, section_buffer->size);
//        output += section_buffer->size;
//        section_buffer = section_buffer->next;
//    }
//}
//
//struct ds_section_t ds_unserialize_section(void **buffer)
//{
//    char *input = *buffer;
//    struct ds_section_t section = {};
//        
//    memcpy(&section.data, input, sizeof(struct ds_section_data_t));
//    input += sizeof(struct ds_section_data_t);
//    
//    section.buffers = mem_Calloc(1, sizeof(struct ds_section_buffer_t) + section.data.size);
//    section.buffers->size = section.data.size;
//    section.last_buffer = section.buffers;
//    memcpy(&section.buffers->data, input, section.data.size);
//    input += section.data.size;
//    
//    *buffer = input;
//    
//    return section;
//}

#endif

#endif // FILE_H






