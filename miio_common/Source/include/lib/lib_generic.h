/**
 * @file			lib_generic.h
 * @brief			Generic definitions
 * @author			dy
*/


#ifndef __LIB_GENERIC_H__
#define __LIB_GENERIC_H__

#ifndef __LIB_GENERIC_TYPE__
#define __LIB_GENERIC_TYPE__

typedef signed char  s8_t;
typedef signed short s16_t;
typedef signed long  s32_t;
typedef signed long long s64_t;
typedef unsigned char  u8_t;
typedef unsigned short u16_t;
typedef unsigned long  u32_t;
typedef unsigned long long u64_t;

typedef unsigned int size_t;
//typedef int ssize_t;

#endif


typedef union{
	struct{
	u16_t u0;
	u16_t u1;}d16;
	struct{
	u32_t u0;}d32;
}du32_t;


typedef union{
	struct{
	u16_t u0;
	u16_t u1;
	u16_t u2;
	u16_t u3;}d16;
	struct{
	u32_t u0;
	u32_t u1;}d32;
}du64_t;


typedef union{
	struct{
	u16_t u0;
	u16_t u1;
	u16_t u2;
	u16_t u3;
	u16_t u4;
	u16_t u5;
	u16_t u6;
	u16_t u7;}d16;
	struct{
	u32_t u0;
	u32_t u1;
	u32_t u2;
	u32_t u3;}d32;
}du128_t;




#ifndef NULL
#define NULL ((void *)0)
#endif

#ifndef FALSE
#define FALSE 0
#endif


#ifndef TRUE
#define TRUE 1
#endif



#ifndef PACK_STRUCT_FIELD

#if defined(__CC_ARM)   /* ARMCC compiler */
#define PACK_STRUCT_FIELD(x) x
#define PACK_STRUCT_STRUCT __attribute__ ((__packed__))
#define PACK_STRUCT_BEGIN
#define PACK_STRUCT_END
#elif defined(__ICCARM__)   /* IAR Compiler */
#define PACK_STRUCT_BEGIN __packed
#define PACK_STRUCT_STRUCT
#define PACK_STRUCT_END
#define PACK_STRUCT_FIELD(x) x
#define PACK_STRUCT_USE_INCLUDES
#elif defined(__GNUC__)     /* GNU GCC Compiler */
#define PACK_STRUCT_FIELD(x) x
#define PACK_STRUCT_STRUCT __attribute__((packed))
#define PACK_STRUCT_BEGIN
#define PACK_STRUCT_END
#else
#define PACK_STRUCT_FIELD(x) x
#define PACK_STRUCT_STRUCT
#define PACK_STRUCT_BEGIN
#define PACK_STRUCT_END
#endif
#endif



// ---------------- CONST MACRO -----------------

#define STATE_OK							0				/* There is no error 						*/
#define STATE_ERROR						(-1)				/* A generic error happens 					*/
#define STATE_TIMEOUT						(-2)				/* Timed out 								*/
#define STATE_FULL						(-3)				/* The resource is full						*/
#define STATE_EMPTY						(-4)				/* The resource is empty 					*/
#define STATE_NOMEM						(-5)				/* No memory								*/
#define STATE_NOSYS						(-6)				/* No system 								*/
#define STATE_BUSY						(-7)				/* Busy										*/
#define STATE_TRYOUT						(-8)				/* try enough times						*/
#define STATE_NOTFOUND					(-9)
#define STATE_PARAM					(-10)

#define STATE_ERR_SIZE					(-11)

//迭代函数返回
#define ITERATOR_CONTINUE STATE_OK
#define ITERATOR_ABORT STATE_ERROR

// ---------------- BITs -----------------

#define BIT_SET(flag,bit) {(flag) |= (bit);}
#define BIT_CLEAR(flag,bit) {(flag) &= ~(bit);}
#define BIT_IS_SET(flag,bit) ((flag) & (bit))

#define FLD_SET(flag,mask,val) {(flag) &= ~(mask); (flag) |= (val);}
#define FLD_GET(flag,mask) ((flag) & (mask))

#define BITMAP_U32_GET(x, bit)     (x[bit / 32] & (1 << (bit&0x1F)))
#define BITMAP_U32_SET(x, bit)     (x[bit / 32] |= (1 << (bit&0x1F)))

// ---------------- Math -----------------
#ifndef MIN
#define MIN(a,b)		((a) < (b) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a,b)		((a) > (b) ? (a) : (b))
#endif

#ifndef ABS
#define ABS(a)	((a)>0?a:(-a))
#endif

// ---------------- struct  -----------------
#define NELEMENTS(x) (sizeof(x)/sizeof(x[0]))
#define SIZEOF(s, m) (sizeof(((s*)0)->m))

#define ATTRIBUTE_OFFSET(structure,elem) ((int)(&(((structure *)0)->elem)))
// ---------------- Ascii -----------------

#define dec_char_value(c) ( (c)-'0' )
#define hex_char_value(c) ((u8_t)(arch_isdigit(c)? dec_char_value(c) : (((c) >='A' && (c)<= 'F')? ((c)-'A' + 10) : ((c)-'a' + 10))))


#define arch_isdigit(ch)	((unsigned)((ch) - '0') < 10)
#define arch_isalpha(ch)	((unsigned int)((ch | 0x20) - 'a') < 26u)
#define arch_isxdigit(c) (((c) >='0' && (c)<= '9')||((c) >='A' && (c)<= 'F')||((c) >='a' && (c)<= 'f'))

//isprint(0x20~0x7E)
#define arch_isprint(c) ((c) >= ' ' && (c)<= '~')
#define arch_toupper(a) ((a) >= 'a' && ((a) <= 'z') ? ((a)-('a'-'A')):(a))





// ---------------- Endian -----------------
#define LONGVAL_BE(c0,c1,c2,c3) (((u32_t)((u8_t)c0)<<24) | (((u32_t)((u8_t)c1))<<16) | (((u32_t)((u8_t)c2))<<8) | ((u32_t)((u8_t)c3)))
#define LONGVAL_LE(c0,c1,c2,c3) (((u32_t)((u8_t)c3)<<24) | (((u32_t)((u8_t)c2))<<16) | (((u32_t)((u8_t)c1))<<8) | ((u32_t)((u8_t)c0)))


#define PTR2LONG_BE(ptr) (((u32_t)((u8_t)(ptr)[0])<<24) | (((u32_t)((u8_t)(ptr)[1]))<<16) | (((u32_t)((u8_t)(ptr)[2]))<<8) | ((u32_t)((u8_t)(ptr)[3])))
#define PTR2LONG_LE(ptr) (((u32_t)((u8_t)(ptr)[3])<<24) | (((u32_t)((u8_t)(ptr)[2]))<<16) | (((u32_t)((u8_t)(ptr)[1]))<<8) | ((u32_t)((u8_t)(ptr)[0])))

#define SHORTVAL_BE(c0,c1) ((((u16_t)((u8_t)c0))<<8) | ((u16_t)((u8_t)c1)))
#define SHORTVAL_LE(c0,c1) ((((u16_t)((u8_t)c1))<<8) | ((u16_t)((u8_t)c0)))

#define PTR2SHORT_BE(ptr) (((u32_t)(ptr)[0]<<8) | ((u32_t)(ptr)[1]))
#define PTR2SHORT_LE(ptr) (((u32_t)(ptr)[1]<<8) | ((u32_t)(ptr)[0]))

#define BYTE1(u32)	((u8_t)((u32)&0xFF))
#define BYTE2(u32)	((u8_t)(((u32) >> 8)&0xFF))
#define BYTE3(u32)	((u8_t)(((u32) >> 16)&0xFF))
#define BYTE4(u32)	((u8_t)(((u32) >> 24)&0xFF))





// ---------------- Other ultilities -----------------
//链表定位
#define LINK_MOVE_TO_LAST(p) {while((p)->next)(p) = (p)->next;}
#define LINK_FOR_EACH(p) for(;NULL != (p);(p) = (p)->next)

#define LINK_ITEM_COUNT(i, n)		while(i){(n)++; (i) = (i)->next;}

#define U32_ALIGN4(a) ((u32_t)(((u32_t)(a) + 3) & ~(u32_t)(3)))
#define U32_ALIGN16(a) ((u32_t)(((u32_t)(a) + 15) & ~(u32_t)(15)))


// ---------------- Assertion -----------------

#ifdef __GNUC__
#define BREAKPOINT() __asm__("bkpt")
#elif defined ( __IAR_SYSTEMS_ICC__ )
#define BREAKPOINT() __asm("bkpt 0")
#else
#define BREAKPOINT() while(1){}
#endif

extern void save_assert_trace(const char * file_name,const char * function,int line);
#define ARCH_ASSERT(info, assertion) do{if(!(assertion)){save_assert_trace(__FILE__,__func__,__LINE__);} }while(0)

#define NO_OPTIMIZE __attribute__((optimize("O0")))

// ---------------- Symb -----------------

#define _STRING(str) #str
#define STRING(str) _STRING(str)

#define _SYMB_ADD_SUFFIX(sym, suffix) sym##suffix
#define SYMB_ADD_SUFFIX(sym, suffix) _SYMB_ADD_SUFFIX(sym, suffix)

//消费者前驱的函数类型指针
#define SYMB_SUFFIX_JS_FACTORY _JsFac
#define JSON_FACTORY(func)	 SYMB_ADD_SUFFIX(func, SYMB_SUFFIX_JS_FACTORY)

//httpd的后端处理逻辑
#define SYMB_SUFFIX_HTTPD_LOGIC _HLogic
#define HTTPD_BACKEND_LOGIC(func) 	 SYMB_ADD_SUFFIX(func, SYMB_SUFFIX_HTTPD_LOGIC)

//json交换的delegate调用接口
#define SYMB_SUFFIX_JSON_DELEGATE _JsDlgt
#define JSON_DELEGATE(func)		SYMB_ADD_SUFFIX(func, SYMB_SUFFIX_JSON_DELEGATE)


//json交换的delegate调用接口(with sid)
#define SYMB_SUFFIX_JSON_DELEGATE_WITH_SID _JsDlgtS
#define JSON_DELEGATE_SID(func)		SYMB_ADD_SUFFIX(func, SYMB_SUFFIX_JSON_DELEGATE_WITH_SID)


#define JSON_RPC(func)		__attribute__ ((visibility ("default")))  SYMB_ADD_SUFFIX(func, SYMB_SUFFIX_JSON_DELEGATE)



//通用的消费者函数类型指针
typedef int(* fp_consumer_generic)(void* ctx, u8_t *p, int len);

//通用的消费者前驱的函数类型指针
typedef int(*fp_factory)(char*arg, int len, fp_consumer_generic consumer, void *consumer_ctx);


//generic decrypt function
typedef void (*fp_decrypt)(void*ctx, u8_t* in, int len, u8_t* out);


//迭代处理函数指针类型(返回非0表示跳出迭代)
typedef int (* fp_iterator)(void* item, void* args);



//委托接口
typedef int (*fp_delegate_ack)(void* ret, void* ctx);
typedef int (*fp_delegate_start)(void* param, fp_delegate_ack ack, void *ctx);





#endif /* __LIB_GENERIC_H__ */



