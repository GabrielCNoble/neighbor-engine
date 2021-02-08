#define DS_VECTOR_IMPLEMENTATION
#include "dstuff/ds_vector.h"

#define DS_MEMORY_IMPLEMENTATION
#include "dstuff/ds_mem.h"

#define DS_STACK_LIST_IMPLEMENTATION
#include "dstuff/ds_stack_list.h"

#define DS_LIST_IMPLEMENTATION
#include "dstuff/ds_list.h"

#define DS_RINGBUFFER_IMPLEMENTATION
#include "dstuff/ds_ringbuffer.h"

#define DS_BUFFER_IMPLEMENTATION
#include "dstuff/ds_buffer.h"

#define DS_DBVH_IMPLEMENTATION
#include "dstuff/ds_dbvh.h"

#define DS_FILE_IMPLEMENTATION
#include "dstuff/ds_file.h"

#define DS_MATRIX_IMPLEMENTATION
#include "dstuff/ds_matrix.h"

#define DS_ALLOC_IMPLEMENTATION
#include "dstuff/ds_alloc.h"

#define DS_PATH_IMPLEMENTATION
#include "dstuff/ds_path.h"

#define DS_WAVEFRONT_IMPLEMENTATION
#include "dstuff/ds_obj.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_MALLOC mem_Malloc
#define STBI_CALLOC mem_Calloc
#define STBI_REALLOC mem_Realloc
#define STBI_FREE mem_Free
#include "stb/stb_image.h"

//#define TINYOBJ_LOADER_C_IMPLEMENTATION
//#define TINYOBJ_MALLOC mem_Malloc
//#define TINYOBJ_REALLOC mem_Realloc
//#define TINYOBJ_CALLOC mem_Calloc
//#define TINYOBJ_FREE mem_Free
//#include "tinyobj_loader_c.h"
