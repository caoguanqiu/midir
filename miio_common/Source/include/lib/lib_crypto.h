#ifndef _LIB_CRYPTO_H_
#define _LIB_CRYPTO_H_

#include "mdev_aes.h"
#include "arch_specific/data_type.h"


void mrvl_crypto_md5(u8 *input, int len, u8 *hash, int hlen);
int mrvl_aes_wrap(u8 *pPlainTxt, u32 TextLen, u8 *pCipTxt, u8 *pKEK,
		  u32 KeyLen, u8 *IV);

int mrvl_aes_unwrap(u8 *pCipTxt, u32 TextLen, u8 *pPlainTxt, u8 *pKEK,
		    u32 KeyLen, u8 *IV);

//md5 porting 
#define md5(in, length,out)  mrvl_crypto_md5(in,length,out,16) //para 16 is useless 

//aes porting 
#define  aes_cbc_encrypt(pPlainTxt, TextLen, pCipTxt, pKEK, KeyLen, IV)    \
         mrvl_aes_wrap(pPlainTxt, TextLen, pCipTxt, pKEK, KeyLen, IV)

#define  aes_cbc_decrypt(pCipTxt, TextLen, pPlainTxt, pKEK,KeyLen, IV)     \
         mrvl_aes_unwrap(pCipTxt, TextLen, pPlainTxt, pKEK,KeyLen, IV)



#define MD5_SIGNATURE_SIZE (16)


//为buf 按照base为对齐padding 同时返回padding的长度
static inline size_t PKCS7_padding(u8_t* buf, size_t len, u8_t base)
{
	u8_t padding_len;

	//取填充字节数(若对齐 则取16)
	padding_len = base - len%base;
	if(0 == padding_len)padding_len = base;

	//填充需要的内容
	int i;
	for(i = 0; i < padding_len; i++)
		buf[len+i] = padding_len;

	return padding_len;
}

#endif //_LIB_CRYPTO_H_
