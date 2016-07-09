#ifndef _MALLOC_H_
#define _MALLOC_H_

#include "wm_os.h"

#define malloc  os_mem_alloc
#define realloc os_mem_realloc
#define free    os_mem_free


static inline void *calloc_xiaomi (size_t __nmemb, size_t __size)
{
    size_t size = __nmemb * __size;
    void *ptr = os_mem_alloc(size);
    if (ptr)
        memset(ptr, 0x00, size);
    return ptr;
}

#define calloc calloc_xiaomi


#endif //_MALLOC_H_
