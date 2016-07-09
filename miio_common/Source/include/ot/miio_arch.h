
#ifndef __MIIO_ARCH_H__
#define __MIIO_ARCH_H__

#include "lib/lib_generic.h"


#include "lib/arch_os.h"


#include <malloc.h>
#include <appln_dbg.h>


#define arch_os_ms_now 		arch_os_tick_now
#define arch_os_ms_elapsed 	arch_os_tick_elapsed

#define arch_os_thread_self() NULL 


#include "lib/lib_crypto.h"

//aes porting 
#define  aes_cbc_enc(pPlainTxt, TextLen, pCipTxt, CipTxtlen, pKEK, KeyLen, IV)    \
         mrvl_aes_wrap(pPlainTxt, TextLen, pCipTxt, pKEK, KeyLen, IV)

#define  aes_cbc_dec(pCipTxt, TextLen, pPlainTxt, PlainTxtlen, pKEK,KeyLen, IV)     \
         mrvl_aes_unwrap(pCipTxt, TextLen, pPlainTxt, pKEK,KeyLen, IV)


#include "util/util.h"


//------ rpc ------

#include "miio_chip_rpc.h"


//------ jsmn ------

#include "lib/jsmn/jsmn_api.h"

//------ net ------

#include <wlan.h>


//------ ot ------
#define ARCH_OT_STACK_SIZE 6144
#include "d0_otd.h"

//------ other ------
#include "main_extension.h"

/*return val > 0 means bytes actually written to the buffer*/
#define snprintf_safe( buffer,size,format,... ) ({int r__ = snprintf(buffer,size,format,##__VA_ARGS__); \
                    (r__ > (size)) ? (size) : r__;})


void hexdump(char* buf, int size, u32_t offset);


//#define XOBJ_PROTECT_BY_MUTEX


#endif /* __MIIO_ARCH_H__ */

