#ifndef __ADDON_H__
#define  __ADDON_H__

#include "lib_generic.h"
#include "lib_xobj.h"
#include "string_pool.h"


#ifndef ADDON_METHOD_SIZE_MAX
#define ADDON_METHOD_SIZE_MAX 64	//addon中函数的最长名称长度
#endif

//addx根名称
#ifndef ADDX_BASE_NAME
#define ADDX_BASE_NAME "miIO"
#endif

#define USER_ADDLIB_NAME "user"



#if defined(__GNUC__)     /* GNU GCC Compiler */

// for miio general purpose functions and variables
#define _VSYM_EXPORT(symbol)								\
const char __vsym_##symbol##_name[] = #symbol;			\
const sym_fw_t __vsym_##symbol __attribute__((used, section("VSymTab"))) = 	\
{														\
	(void *)&symbol,									\
	__vsym_##symbol##_name								\
};
#define VSYM_EXPORT(symbol)	_VSYM_EXPORT(symbol)



#define _VSYM_EXPORT_DETAIL(symbol, addr)				\
const char __vsym_##symbol##_name[] = #symbol;			\
const sym_fw_t __vsym_##symbol __attribute__((used, section("VSymTab")))= 	\
{														\
	(void *)addr,										\
	__vsym_##symbol##_name								\
};
#define VSYM_EXPORT_DETAIL(symbol, addr)	_VSYM_EXPORT_DETAIL(symbol, addr)



#define _FSYM_EXPORT(symbol)					 			\
const char __fsym_##symbol##_name[] = #symbol;			\
const sym_fw_t __fsym_##symbol __attribute__((used, section("FSymTab"))) = 	\
{														\
	(void *)symbol,									\
	__fsym_##symbol##_name								\
};
#define FSYM_EXPORT(symbol)  _FSYM_EXPORT(symbol)


#define _FSYM_EXPORT_DETAIL(symbol, addr)				\
const char __vsym_##symbol##_name[] = #symbol;			\
const sym_fw_t __vsym_##symbol __attribute__((used, section("FSymTab")))= 	\
{														\
	(void *)addr,										\
	__vsym_##symbol##_name								\
};

#define FSYM_EXPORT_DETAIL(symbol, addr) _FSYM_EXPORT_DETAIL(symbol, addr)



// for user defined functions and variables
#define _USER_VSYM_EXPORT(symbol)								\
const char __vsym_##symbol##_name[] = #symbol;			\
const sym_fw_t __vsym_##symbol __attribute__((used, section("UserVSymTab"))) = 	\
{														\
	(void *)&symbol,									\
	__vsym_##symbol##_name								\
};
#define USER_VSYM_EXPORT(symbol)	_USER_VSYM_EXPORT(symbol)

#define _USER_FSYM_EXPORT(symbol)					 			\
const char __fsym_##symbol##_name[] = #symbol;			\
const sym_fw_t __fsym_##symbol __attribute__((used, section("UserFSymTab"))) = 	\
{														\
	(void *)symbol,									\
	__fsym_##symbol##_name								\
};
#define USER_FSYM_EXPORT(symbol)  _USER_FSYM_EXPORT(symbol)
//#elif


#endif







typedef int (*fp_addon_entry)( int argc, char *argv[] );
//typedef int (*fp_addon_entry)();





typedef struct sym_fw {
	void* pvalue;
	const char* name_ptr;
}sym_fw_t;

typedef struct sym_dynamic {
	void* pvalue;
	u16_t name_idx;
	u16_t flag;
}sym_dynamic_t;

typedef union sym_addx {
	sym_fw_t sym_rom;
	sym_dynamic_t sym_ram;
}sym_addx_t;

//addlite中的symbol采用string_pool方式存放符号名称，索引方式也是下标，而非指针。

typedef struct addon_info {

	xobj_info_t xobj;

	//执行域
	void* load_addr;
	u32_t load_size;
	//入口
	void* entry;	//for addlite
	u8_t version[4];

	//变量、函数表、串池
	sym_addx_t *fsym_tab;
	sym_addx_t *vsym_tab;
	u16_t fsym_num;
	u16_t vsym_num;
	string_pool_t *pool;	//for addlite

} addon_info_t;



addon_info_t* addon_find(const char* addx_name, size_t addx_name_len_opt);
void* addon_symb(const char* sym);
void* addon_fsymb(const char* sym);
void* addon_vsymb(const char* sym);

void* addon_fsymb_in_paddon(addon_info_t* p_addx, const char* sym_str, size_t sym_str_len_opt);
void* addon_vsymb_in_paddon(addon_info_t* p_addx, const char* sym_str, size_t sym_str_len_opt);
void* addon_fsymb_in_addon(const char* addx_name, const char* sym);
void* addon_vsymb_in_addon(const char* addx_name, const char* sym);

void* addon_fsym_parse(char c, const char* str, size_t str_len_opt, addon_info_t** add);
void* addon_vsym_parse(char c, const char* str, size_t str_len_opt, addon_info_t** add);
void* addon_fsym_suffix_parse(char c, const char* addon_symb, size_t addon_symb_len_opt, const char* suffix, addon_info_t** add);
void* addon_vsym_suffix_parse(char c, const char* addon_symb, size_t addon_symb_len_opt, const char* suffix, addon_info_t** add);

int addon_addlite_remove(char* addlite_name);
int addon_paddlite_remove(addon_info_t* add);



void addon_info(addon_info_t* p_addx);
void addon_List(void);


//---------------------------------------------------------------------------------


#if defined(__GNUC__)     /* GNU GCC Compiler */

extern const void* addlib_image_ld_addr;
extern const void* addlib_image_size;

extern const void* addlib_header_addr;
extern const void* addlib_header_area_size;

extern const void* addlib_ro_addr;
extern const void* addlib_ro_area_size;

extern int addlib_vsymtab_start;
extern int addlib_fsymtab_start;
extern int addlib_fsymtab_area_size;
extern int addlib_vsymtab_area_size;

extern int addlib_user_fsymtab_start;
extern int addlib_user_vsymtab_start;
extern int addlib_user_fsymtab_area_size;
extern int addlib_user_vsymtab_area_size;


//符号表相关
#define ADDLIB_MAP_FSYMB_TAB_ADDR	((void*)&addlib_fsymtab_start)
#define ADDLIB_MAP_VSYMB_TAB_ADDR	((void*)&addlib_vsymtab_start)
#define ADDLIB_MAP_FSYMB_TAB_SIZE	((int)&addlib_fsymtab_area_size)
#define ADDLIB_MAP_VSYMB_TAB_SIZE	((int)&addlib_vsymtab_area_size)

#define ADDLIB_MAP_USER_FSYMB_TAB_ADDR	((void*)&addlib_user_fsymtab_start)
#define ADDLIB_MAP_USER_VSYMB_TAB_ADDR	((void*)&addlib_user_vsymtab_start)
#define ADDLIB_MAP_USER_FSYMB_TAB_SIZE	((int)&addlib_user_fsymtab_area_size)
#define ADDLIB_MAP_USER_VSYMB_TAB_SIZE	((int)&addlib_user_vsymtab_area_size)



#endif


#define ADDLIB_HEADER_MAGIC "addlib"
#define ADDLIB_HDR_MAGIC_SIZE 8


//该结构体描述addlib文件头部的结构体
//addlib固件首部(向量表之前)放置一个描述头部，其用于描述向量表位置、版本、固件占用长度等信息，主要方便bootloader的升级甄别、引导
typedef struct
{
	char magic[ADDLIB_HDR_MAGIC_SIZE];
	char name[XOBJ_NAME_LEN_MAX];
	u8_t version[4];

	u32_t total_size;		// including vector_table and this header

	void *fsym_tab;
	void *vsym_tab;
	u32_t fsym_table_size;
	u32_t vsym_table_size;
} addlib_header_t;


extern const addlib_header_t addlib_header;




#endif /*  __ADDON_H__ */

