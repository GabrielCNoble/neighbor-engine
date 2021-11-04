#ifndef DS_DIR_H
#define DS_DIR_H


#include <dirent.h>
#include <limits.h>
#include <stdint.h>

#include "ds_path.h"

enum DS_DIR_ENTRY_TYPE
{
    DS_DIR_ENTRY_TYPE_DIR = 0,
    DS_DIR_ENTRY_TYPE_FILE,
};

// struct ds_dir_entry_t
// {
//     uint32_t type;
//     char name[PATH_MAX];
// };

// struct ds_dir_list_t
// {
//     DIR *dir;
//     char path[PATH_MAX];
//     uint32_t max_entry_count;
//     uint32_t entry_count;
//     struct ds_dir_entry_t *entries;
// };

struct ds_dir_t
{
    DIR *dir;
    char name[PATH_MAX];
};

struct ds_dir_entry_t
{
    uint32_t type;
    char name[PATH_MAX];
};

uint32_t ds_dir_open_dir(char *path, struct ds_dir_t *dir);

void ds_dir_close_dir(struct ds_dir_t *dir);

uint32_t ds_dir_next_entry(struct ds_dir_t *dir, struct ds_dir_entry_t *entry);

// void ds_dir_GoUp(struct ds_dir_list_t *dir);

// void ds_dir_GoDown(struct ds_dir_list_t *dir, char *name);

// void ds_dir_EnumerateEntries(struct ds_dir_list_t *dir);

// uint32_t ds_dir_IsDir(char *path);



#ifdef DS_DIR_IMPLEMENTATION


uint32_t ds_dir_open_dir(char *path, struct ds_dir_t *dir)
{
    DIR *probe_dir;
    probe_dir = opendir(path);

    if(probe_dir)
    {
        dir->dir = probe_dir;
        strcpy(dir->name, probe_dir->dd_name);
        uint32_t length = strlen(dir->name);

        if(length && dir->name[length - 1] == '*')
        {
            dir->name[length - 1] = '\0';
        }

        ds_path_format_path(dir->name, dir->name, PATH_MAX);

        return 1;
    }

    return 0;
}

void ds_dir_close_dir(struct ds_dir_t *dir)
{
    if(dir && dir->dir)
    {
        closedir(dir->dir);
        dir->dir = NULL;
    }
}

uint32_t ds_dir_next_entry(struct ds_dir_t *dir, struct ds_dir_entry_t *entry)
{
    char probe_path[PATH_MAX];

    if(dir && dir->dir)
    {
        struct dirent *dir_entry = readdir(dir->dir);

        if(dir_entry)
        {
            strcpy(entry->name, dir_entry->d_name);
            uint32_t length = strlen(entry->name);

            if(length && entry->name[length - 1] == '*')
            {
                entry->name[length - 1] = '\0';
            }

            ds_path_format_path(entry->name, entry->name, PATH_MAX);
            ds_path_append_end(dir->name, entry->name, probe_path, PATH_MAX);
            DIR *probe_dir = opendir(probe_path);

            if(probe_dir)
            {
                closedir(probe_dir);
                entry->type = DS_DIR_ENTRY_TYPE_DIR;
            }
            else
            {
                entry->type = DS_DIR_ENTRY_TYPE_FILE;
            }

            return 1;
        }
    }

    return 0;
}

#endif // DS_DIR_IMPLEMENTATION

#endif // DS_DIR_H
