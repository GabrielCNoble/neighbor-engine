#ifndef DS_PATH_H
#define DS_PATH_H

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
    ds_path - simple path manipulation routines

    INTEGRATION:

        Integrating this library works similarly to the stb libraries. To create the
        implementations you do the following in ONE C or C++ file:

            #define DS_PATH_IMPLEMENTATION
            #include "ds_path.h"



    ABOUT:

        This file provides a simple set of functions for path manipulation.

        All functions take as arguments an output buffer and the size of this buffer.
        If the output buffer is not big enough to contain the result, it's left
        untouched, and the function returns the number of extra characters the caller
        would need to provide for the call to succedd. It returns zero on success.

        Also, no function will check if it has received a non-null output buffer pointer,
        since using it without an output buffer makes no sense. That means, if you
        pass a NULL pointer in the output buffer, it'll crash.

        All functions are guaranteed to return up to MAX_PATH characters in the
        output buffer, INCLUDING THE NULL TERMINATOR. So, if an output of this
        size gets returned, the output will consist of 259 non-null characters,
        and one null character at the end. This also means that no function will
        deal with paths longer than PATH_MAX (including null terminator). If such
        a path is passed, the function will return SIZE_MAX.

        It's safe to pass the same input buffer as the output buffer. All functions
        format the provided paths, which are copied it to an internal buffer before
        copying it to the output buffer.

        The kinds of path handled are the most common type (either '\' or '/'
        as separator). This means both Windows and unix-like paths should work
        well.

        Paths are normalized to unix-like paths, since the standard C library can
        make do with those paths in all operating systems. Trailing directory
        separators are stripped away. Directory separators at the start of the
        path are just converted. Repeated separators are converted to a single
        separator.

        Keep in mind those functions mostly just modify text. They don't employ any
        sort of regular expression matching, and don't validate paths. If they recieve
        gargabe, they'll likely return garbage.


    BASIC USAGE:

        char output[<some size>];

        if(!ds_path_append_end("this\\is\\a\\relative\\path", "this\\is\\a\\folder\\or\\file" output, <some size>))
        {
            // everything is fine, use path here
        }

        // not enough space in the output buffer. If all you need is to quickly concatenate the path to use in some function call,
        // just declare the output buffer with a size of PATH_MAX
*/

#ifndef DS_STRING_H_INCLUDED
    #include <string.h>
    #define DS_STRING_H_INCLUDED
#endif

#ifndef DS_LIMITS_H_INCLUDED
    #include <limits.h>
    #define DS_LIMITS_H_INCLUDED
#endif

#ifndef DS_STDDEF_H_INCLUDED
    #include <stddef.h>
    #define DS_STDDEF_H_INCLUDED
#endif

#ifndef PATH_MAX
    #define PATH_MAX 260
#endif

#define DS_PATH_NO_ERROR 0
#define DS_PATH_NO_SPACE 1
#define DS_PATH_BAD_PATH 2
#define DS_PATH_BAD_EXT 3

#define DS_PATH_VALIDATE_PATHS
#define DS_PATH_CONVENTION_UNIX

#ifdef __cplusplus
extern "C"
{
#endif

/*
    ds_path_get_end - returns the last part of a path.

    If there's a path "foo/bar/baz", the return will be "baz".

    An empty path gives an empty string.
*/
size_t ds_path_get_end(char *path, char *out, size_t out_size);

/*
    ds_path_drop_end - removes the last part of a path, returns the rest.

    If there's a path "foo/bar/baz", the return will be "foo/bar".

    An empty path gives an empty string.
*/
size_t ds_path_drop_end(char *path, char *out, size_t out_size);

/*
    ds_path_append_end - appends a path to the end of another.

    If there's a path "foo/bar/baz", and another "fiz/buzz", the return will be
    "foo/bar/baz/fiz/buzz".

    Empty paths gives an empty string.
*/
size_t ds_path_append_end(char *path, char *append, char *out, size_t out_size);

/*
    ds_path_get_ext - returns the extension at the end of path, if any.

    If there's a path "foo/bar/baz/bleh.ext", the return will be "ext"

    An empty path or no extension gives an empty string.
*/
size_t ds_path_get_ext(char *path, char *out, size_t out_size);

/*
    ds_path_drop_ext - removes the extension in the last part of the path, if any.

    If there's a path "foo/bar/baz/bleh.ext", the return will be "foo/bar/baz/bleh"

    An empty path gives an empty string.
*/
size_t ds_path_drop_ext(char *path, char *out, size_t out_size);

/*
    ds_path_set_ext - either adds or substitutes an already present extension at the
    end of a path.

    If there's a path "foo/bar/baz/bleh", and an extension "ext" the return will be
    "foo/bar/baz/bleh.ext". If then there's a second extension "ext2", the return
    will be "foo/bar/baz/bleh.ext2"

    An empty path gives an empty string. An empty extension returns gives the original
    path.
*/
size_t ds_path_set_ext(char *path, char *ext, char *out, size_t out_size);

/*
    ds_path_append_ext - adds an extension at the end of a path.

    If there's a path "foo/bar/baz/bleh", and an extension "ext" the return will be
    "foo/bar/baz/bleh.ext". If then there's a second extension "ext2", the return
    will be "foo/bar/baz/bleh.ext.ext2"

    An empty path gives an empty string. An empty extension returns gives the original
    path.
*/
size_t ds_path_append_ext(char *path, char *ext, char *out, size_t out_size);

/*
    ds_path_format_path - formats a path to a unix-like format

    If there's a path "foo\\bar\\baz\\bleh", the return will be "foo/bar/baz/bleh".

    An empty path gives an empty string.
*/
size_t ds_path_format_path(char *path, char *out, size_t out_size);



size_t ds_path_validate_path(char *path);


int ds_path_is_absolute(char *path);

#ifdef __cplusplus
}
#endif

#ifdef DS_PATH_IMPLEMENTATION

#ifdef __cplusplus
extern "C"
{
#endif


size_t ds_path_get_end(char *path, char *out, size_t out_size)
{
    char stripped_path[PATH_MAX] = "";

    if(ds_path_format_path(path, stripped_path, PATH_MAX) == SIZE_MAX)
    {
        return DS_PATH_NO_SPACE;
    }

    size_t length = strlen(stripped_path);
    size_t index = length;

    while(index && stripped_path[index - 1] != '/')
    {
        index--;
    }

    /* substring chars + null terminator */
    size_t copy_size = (length - index) + 1;

    if(copy_size > out_size)
    {
        return DS_PATH_NO_SPACE;
        // return copy_size - out_size;
    }

    out[0] = '\0';
    /* strncat nicely writes a null terminator for us */
    strncat(out, stripped_path + index, copy_size - 1);

    return DS_PATH_NO_ERROR;
}

size_t ds_path_drop_end(char *path, char *out, size_t out_size)
{
    char stripped_path[PATH_MAX] = "";

    if(ds_path_format_path(path, stripped_path, PATH_MAX) == DS_PATH_NO_SPACE)
    {
        return DS_PATH_NO_SPACE;
    }

    size_t length = strlen(stripped_path);
    size_t index = length;

    while(index && stripped_path[index - 1] != '/')
    {
        index--;
    }

    if(index > out_size)
    {
        return DS_PATH_NO_SPACE;
        // return index - out_size;
    }

    out[0] = '\0';
    /* strncat nicely writes a null terminator for us */
    strncat(out, stripped_path, index - 1);

    return DS_PATH_NO_ERROR;
}

size_t ds_path_append_end(char *path, char *append, char *out, size_t out_size)
{
    char formatted_path[PATH_MAX] = "";
    char formatted_append[PATH_MAX] = "";
    size_t append_offset = 0;

    if(ds_path_format_path(path, formatted_path, PATH_MAX) == DS_PATH_NO_SPACE ||
       ds_path_format_path(append, formatted_append, PATH_MAX) == DS_PATH_NO_SPACE)
    {
        return DS_PATH_NO_SPACE;
    }

    size_t formatted_len = strlen(formatted_path);
    size_t append_len = strlen(formatted_append);

    if(formatted_append[0] == '/')
    {
        append_offset++;
        append_len--;
    }

    /* length of formatted_len + append_len + a '/' char + null terminator */
    size_t copy_size = formatted_len + append_len + 2;

    if(copy_size > out_size)
    {
        return DS_PATH_NO_SPACE;
        // return copy_size - out_size;
    }

    out[0] = '\0';

    strncat(out, formatted_path, formatted_len);

    if(formatted_len && append_len)
    {
        strncat(out, "/", 2);
    }

    strncat(out, formatted_append + append_offset, append_len);

    return DS_PATH_NO_ERROR;
}

size_t ds_path_get_ext(char *path, char *out, size_t out_size)
{
    char stripped_ext[PATH_MAX];

    if(ds_path_format_path(path, stripped_ext, PATH_MAX) == DS_PATH_NO_SPACE)
    {
        return DS_PATH_NO_SPACE;
    }

    size_t length = strlen(stripped_ext);
    size_t index = length;

    while(index && path[index - 1] != '.')
    {
        index--;
    }

    size_t copy_size = (length - index) + 1;

    if(copy_size > out_size)
    {
        return DS_PATH_NO_SPACE;
        // return copy_size - out_size;
    }

    out[0] = '\0';

    if(index)
    {
        strncat(out, stripped_ext + index, copy_size - 1);
    }

    return DS_PATH_NO_ERROR;
}

size_t ds_path_drop_ext(char *path, char *out, size_t out_size)
{
     char new_path[PATH_MAX];
     uint32_t length = strlen(path);
     uint32_t index = length;

     ds_path_format_path(path, new_path, PATH_MAX);

     while(index && new_path[index] != '.')index--;

     if(index)
     {
         new_path[index] = '\0';
     }

     strncpy(out, new_path, out_size);

     return DS_PATH_NO_ERROR;
}

size_t ds_path_set_ext(char *path, char *ext, char *out, size_t out_size)
{
    char new_path[PATH_MAX];
    char new_ext[PATH_MAX];
    char *new_ext_start = new_ext;

    if(ds_path_format_path(path, new_path, PATH_MAX) == DS_PATH_NO_SPACE ||
       ds_path_format_path(ext, new_ext, PATH_MAX) == DS_PATH_NO_SPACE)
    {
        return DS_PATH_NO_SPACE;
    }

    size_t path_length = strlen(new_path) + 1;
    size_t ext_length = strlen(new_ext);

    for(size_t index = ext_length; index >= 0; index--)
    {
        if(new_ext[index] == '.')
        {
            new_ext_start += index + 1;
            break;
        }
    }

    for(size_t index = 0; index < path_length; index++)
    {
        if(new_path[index] == '.')
        {
            index++;

            if(!strncmp(new_path + index, ext, PATH_MAX))
            {
                return DS_PATH_NO_ERROR;
            }

            new_path[index] = '\0';
            path_length -= index;
            break;
        }
    }

    if(path_length + ext_length > out_size)
    {
        return DS_PATH_NO_SPACE;
    }

    strncpy(out, new_path, path_length);

    if(ext_length)
    {
        strncat(out, ".", 2);
        strncat(out, new_ext, ext_length);
    }

    return DS_PATH_NO_ERROR;
}

size_t ds_path_append_ext(char *path, char *ext, char *out, size_t out_size)
{
    // static char file_name_ext[PATH_MAX];
    // int32_t index;
    // if(strcmp(ds_path_GetExt(path), ext))
    // {
    //     strcpy(file_name_ext, path);
    //     index = strlen(file_name_ext);

    //     while(index >= 0 && file_name_ext[index] != '.')
    //     {
    //         index--;
    //     }

    //     if(index >= 0)
    //     {
    //         file_name_ext[index] = '\0';
    //     }

    //     if(ext[0] != '.')
    //     {
    //         strcat(file_name_ext, ".");
    //     }

    //     strcat(file_name_ext, ext);
    //     return file_name_ext;
    // }

    // return file_name_ext;
}

size_t ds_path_format_path(char *path, char *out, size_t out_size)
{
    char formatted_path[PATH_MAX] = "";
    size_t formatted_index = 0;

    if(path)
    {
        size_t index;

        for(index = 0; path[index] && formatted_index < PATH_MAX; index++)
        {
            if(path[index] == '\\' || path[index] == '/')
            {
                formatted_path[formatted_index] = '/';

                while(path[index + 1] == '\\' || path[index + 1] == '/')
                {
                    index++;
                }
            }
            else
            {
                formatted_path[formatted_index] = path[index];
            }

            formatted_index++;
        }

        if(path[index])
        {
            return DS_PATH_NO_SPACE;
        }

        if(formatted_index > 1 && formatted_path[formatted_index - 1] == '/')
        {
            if(formatted_index > 2 && formatted_path[formatted_index - 2] == ':')
            {
                /* do nothing */
            }
            else
            {
                /* only remove a trailing '/' if it's at the end of the path, and it's not
                after a volume separator */
                formatted_index--;
            }
        }
    }

    formatted_path[formatted_index] = '\0';
    formatted_index++;

    if(formatted_index > out_size)
    {
        return formatted_index - out_size;
    }

    out[0] = '\0';
    strncat(out, formatted_path, out_size);

    return 0;
}

int ds_path_is_absolute(char *path)
{
    char formatted_path[PATH_MAX];
    ds_path_format_path(path, formatted_path, PATH_MAX);
    size_t len = strlen(formatted_path) + 1;

    size_t index;
    for(index = 0; index < len; index++)
    {
        if(formatted_path[index] == ':')
        {
            index++;

            if(formatted_path[index] == '/' || formatted_path[index] == '\0')
            {
                return 1;
            }

            break;
        }
    }

    return 0;
}

size_t ds_path_validate_path(char *path)
{
    #ifdef DS_PATH_VALIDATE_PATHS
        if(!path)
        {
            return 0;
        }

        size_t length = strlen(path) + 1;
    #else
        return 1;
    #endif
}

#ifdef __cplusplus
}
#endif


#endif /* DS_PATH_IMPLEMENTATION */


#endif /* DS_PATH_H */
