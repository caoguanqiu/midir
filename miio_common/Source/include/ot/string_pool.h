
#ifndef __STRING_POOL_H__
#define __STRING_POOL_H__

#include "lib_generic.h"

typedef struct {
	char *pool;
	u32_t size;
}string_pool_t;

string_pool_t* str_pool_alloc(int size);
void str_pool_reset(string_pool_t* sp);
u32_t str_pool_get(string_pool_t* sp , char* s);
u32_t str_pool_put(string_pool_t* sp , char* s);
u32_t str_pool_size(string_pool_t* sp);
void str_pool_list(string_pool_t* sp);


#endif /* __STRING_POOL_H__ */

