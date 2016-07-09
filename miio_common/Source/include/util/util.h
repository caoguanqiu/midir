
#ifndef __UTL_H__
#define __UTL_H__
#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>

#include <lib/lib_generic.h>

typedef struct id_str_pair{
	const int id;
	const char *str;
}id_str_pair_t ;


const int id_to_idx(const id_str_pair_t* array, size_t array_len, int id);
const int str_to_idx(const id_str_pair_t* array, size_t array_len, const char*str);
int str_to_id(const id_str_pair_t* array, size_t array_len, const char*str);
const char* id_to_str(const id_str_pair_t* array, size_t array_len, int id);



s32_t arch_vsprintf(char *buf, const char *format, va_list arg_ptr);
s32_t arch_vsnprintf(char *buf, int size, const char *format, va_list arg_ptr);
s32_t arch_sprintf(char *buf ,const char *format,...);
s32_t arch_snprintf(char *buf, int size, const char *fmt, ...);
int arch_rsscanf(const char* str, const char* format, ...);
int snprintf_hex(char *buf, size_t buf_size, const u8_t *data, size_t len, char style);
int snprint_time_stamp(u32_t tick, char* buf, int size);

int str_escape_from_buf(char* src, size_t src_len, char* dest, size_t dest_size);
int str_unescape_to_buf(const char* src, size_t src_len, char* dest, size_t dest_size);

u32_t memchcmp(const void* s, u8_t c, size_t n);
 int arch_strstart(const char *str, const char *start);


u32_t arch_atoun(const char* c, size_t n);
u32_t arch_atou_len(const char* c, size_t* len);
u32_t arch_axtou(const char* c);
u32_t arch_axtou_len(const char* c, size_t* len);
u32_t arch_axtoun(const char* c, size_t n);
size_t arch_axtobuf_detail(const char* in, size_t in_size, u8_t* out, size_t out_size, size_t *in_len);
u64_t arch_atou64n(const char* c, size_t n);
s32_t arch_atoi(const char * c, size_t n);
s64_t arch_atos64n(const char* c, size_t n);

#define arch_axtobuf(in, out) arch_axtobuf_detail((in), 0xFFFFFFFF, (out), 0xFFFFFFFF, NULL)

int memstrcmp(const char *str_mem, size_t str_mem_len, const char *str);


#endif /* __UTL_H__ */

