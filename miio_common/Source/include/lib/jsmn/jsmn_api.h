/*! \file jsmn_api.h
 *  \brief json解析库 jsmn
 *
 *  json解析库 jsmn 
 *
 */

#ifndef __JSMN_API_H__
#define __JSMN_API_H__



/*
jsmn的token结构经过精简优化，每一个单元仅仅占用6字节，代价是：
	json串最长支持65535字节
	数组最多支持255个

*/

#include <jsmn/jsmn.h>


typedef struct jsmn_node{
	const char* js;
	int js_len;
	jsmntok_t* tkn;
}jsmn_node_t;


typedef int (*fp_json_delegate_ack)(jsmn_node_t*, void* ctx);	//继承于 fp_delegate_ack
typedef int (*fp_json_delegate_start)(jsmn_node_t*, fp_json_delegate_ack, void*);	//继承于 fp_delegate_start
typedef int (*fp_json_delegate_start_with_sid)(jsmn_node_t*, char*, fp_json_delegate_ack, void*);	//用于有sid参数的场合




/**
 *	@brief 		Jump to next same level token.
 *	@details	
 *	@return		Next token
 */
jsmntok_t* jsmn_next(jsmntok_t* t);

/**
 *	@brief 		Get token val from father token via 'key' string.
 *	@details	Token A point to "{"abc":123}", call jsmn_api.key_value(js, A, "abc") will return token points to '123'.
 *	@return		Value token
 */
jsmntok_t* jsmn_key_value(const char* js, jsmntok_t* tokens, const char* key);

/**
 *	@brief 		Get token val from father array token via index.
 *	@details	Token A point to "[1,2,3]", call jsmn_api.array_value(js, A, 2) will return token points to '3'.
 *	@return		Value token
 */
jsmntok_t* jsmn_array_value(const char* js, jsmntok_t* tokens, u32_t idx);


int jsmn_tkn2val_u64(const char *js ,jsmntok_t * tk, u64_t *val);
int jsmn_tkn2val_s64(const char *js ,jsmntok_t * tk, s64_t *val);


/**
 *	@brief 		Get unsigned/signed integer val(32bits) from jsmn token object
 *	@details	
 *	@return		STATE_OK : Successful.
 *				other : if an error occurred.
 */
int jsmn_tkn2val_uint(const char* js, jsmntok_t* tk, u32_t* val);
int jsmn_tkn2val_sint(const char* js, jsmntok_t* tk, s32_t* val);

/**
 *	@brief 		Get unsigned/signed integer val(16bits) from jsmn token object
 *	@details	
 *	@return		STATE_OK : Successful.
 *				other : if an error occurred.
 */
int jsmn_tkn2val_u16(const char* js, jsmntok_t* tk, u16_t* val);
int jsmn_tkn2val_s16(const char* js, jsmntok_t* tk, s16_t* val);

/**
 *	@brief 		Get unsigned/signed integer val(8bits) from jsmn token object
 *	@details	
 *	@return		STATE_OK : Successful.
 *				other : if an error occurred.
 */
int jsmn_tkn2val_u8(const char* js, jsmntok_t* tk, u8_t* val);
int jsmn_tkn2val_s8(const char* js, jsmntok_t* tk, s8_t* val);

/**
 *	@brief 		Get string val from jsmn token object
 *	@details	
 *	@return		STATE_OK : Successful.
 *				other : if an error occurred.
 */
int jsmn_tkn2val_str(const char* js, jsmntok_t* tk, char* str, size_t str_size, size_t *str_len);


/**
 *	@brief 		Get buf from string token object like "0123456789abcdef"
 *	@details	
 *	@return		STATE_OK : Successful.
 *				other : if an error occurred.
 */
int jsmn_tkn2val_xbuf(const char* js, jsmntok_t* tk, u8_t* buf, size_t buf_size, size_t *buf_len);


/**
 *	@brief 		Get 0 or 1 from bool token object.
 *	@details	
 *	@return		STATE_OK : Successful.
 *				other : if an error occurred.
 */
int jsmn_tkn2val_bool(const char* js, jsmntok_t* tk, u8_t* val);


/**
 *	@brief 		Find value token form father object token via its key string, and get 0 or 1 from it.
 *	@details	Same result as: tkn2val_bool(js, key_value(js, tokens, key), &val)
 *	@return		STATE_OK : Successful.
 *				other : if an error occurred.
 */
int jsmn_key2val_bool(const char* js, jsmntok_t* tokens, const char* key, u8_t* val);


/**
 *	@brief 		Find value token form father object token via its key string, and get unsigned 32bits integer from it.
 *	@details	Same result as: tkn2val_uint(js, key_value(js, tokens, key), &val)
 *	@return		STATE_OK : Successful.
 *				other : if an error occurred.
 */
int jsmn_key2val_uint(const char* js, jsmntok_t* tokens, const char* key, u32_t* val);
/**
 *	@brief 		Find value token form father object token via its key string, and get unsigned 16bits integer from it.
 *	@details	Same result as: tkn2val_u16(js, key_value(js, tokens, key), &val)
 *	@return		STATE_OK : Successful.
 *				other : if an error occurred.
 */
int jsmn_key2val_u16(const char* js, jsmntok_t* tokens, const char* key, u16_t* val);
/**
 *	@brief 		Find value token form father object token via its key string, and get unsigned 8bits integer from it.
 *	@details	Same result as: tkn2val_u8(js, key_value(js, tokens, key), &val)
 *	@return 	STATE_OK : Successful.
 *				other : if an error occurred.
 */
int jsmn_key2val_u8(const char* js, jsmntok_t* tokens, const char* key, u8_t* val);

/**
 *	@brief 		Find string token form father object token via its key string, and get string from it.
 *	@details	Same result as: tkn2val_str(js, key_value(js, tokens, key), str)
 *	@return 	STATE_OK : Successful.
 *				other : if an error occurred.
 */
int jsmn_key2val_str(const char* js, jsmntok_t* tokens, const char* key, char* str, size_t str_size, size_t *str_len);

/**
 *	@brief 		Get buf from father string token like "0123456789abcdef"
 *	@details	Same result as: jsmn_tkn2val_xbuf(js, key_value(js, tokens, key), buf, buf_size, &buf_len)
 *	@return		STATE_OK : Successful.
 *				other : if an error occurred.
 */
int jsmn_key2val_xbuf(const char* js, jsmntok_t* tokens, const char* key, u8_t* buf, size_t buf_size, size_t *buf_len);


int jsmn_key2val_u64(const char *js ,jsmntok_t* tokens, const char* key, u64_t* val);


int jsmn_cli_init(void);



#endif /* __JSMN_API_H__ */
