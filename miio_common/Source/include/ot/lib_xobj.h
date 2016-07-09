#ifndef __XOBJ_H__
#define __XOBJ_H__

#include "lib_generic.h"




#ifndef XOBJ_NAME_LEN_MAX
#define XOBJ_NAME_LEN_MAX 16	//xobj 名称最大长度
#endif


#define XOBJ_FLAG_TYPE_MASK	0x0007
#define XOBJ_FLAG_TYPE_ADDLITE	0x0001
#define XOBJ_FLAG_TYPE_ADDLIB	0x0002
#define XOBJ_FLAG_TYPE_NB	0x0003
#define XOBJ_FLAG_TYPE_D0	0x0004
//隐藏属性
#define XOBJ_FLAG_HIDDEN	0x0008

#define XOBJ_TYPE(x) FLD_GET(((xobj_info_t*)(x))->flag, XOBJ_FLAG_TYPE_MASK)
#define XOBJ_SET_TYPE(x, t) FLD_SET(((xobj_info_t*)(x))->flag, XOBJ_FLAG_TYPE_MASK, (t))

#define XOBJ_SET_HIDEN(x) BIT_SET(((xobj_info_t*)(x))->flag, XOBJ_FLAG_HIDDEN)
#define XOBJ_IS_HIDEN(x) BIT_IS_SET(((xobj_info_t*)(x))->flag, XOBJ_FLAG_HIDDEN)


#define XOBJ_NAME(x) xobj_name((xobj_info_t*)(x))




typedef struct xobj_info {
	struct xobj_info* next;
	char name[XOBJ_NAME_LEN_MAX];
	u16_t flag;
}xobj_info_t;





int xobj_sys_init(xobj_info_t* pxobj);
void xobj_info_reset(xobj_info_t* pxobj, char* name, u16_t type);
xobj_info_t* xobj_remove_byname(char* name, u16_t type);
int xobj_remove(xobj_info_t* unit);
xobj_info_t* xobj_find(const char* name, size_t name_len_opt, u16_t type);
int xobj_append(xobj_info_t* xobj);
int xobj_api_foreach(fp_iterator func, void* args);
char* xobj_name(xobj_info_t* xobj);



#endif /* __XOBJ_H__ */
