
#include "miio_arch.h"
#include <jsmn/jsmn_api.h>

#define TOKEN_VALID(pt) (pt != NULL \
                && ((pt)->type == JSMN_PRIMITIVE \
                        || (pt)->type == JSMN_OBJECT \
                        || (pt)->type == JSMN_ARRAY \
                        || (pt)->type == JSMN_STRING))
#define TOKEN_ISDIGIT (tk,js) ((tk)->type == JSMN_PRIMITIVE && arch_isdigit(js[(tk)->start]))


//用于跳到下一个同层次的节点(从key跳到下一个key 或者 array单元跳到下一个)
jsmntok_t* jsmn_next(jsmntok_t* t)
{
	int range = 1;
	while(range > 0 && TOKEN_VALID(t)){
		range += t->size;
		t++; range--;
	}
	if(TOKEN_VALID(t))
		return t;
	else
		return NULL;
}



//输入原始串 、解析数组 和 需要定位的key
//仅仅扫描当前层面的名值对
jsmntok_t* jsmn_key_value(const char* js, jsmntok_t* tokens, const char* key)
{
	jsmntok_t* t = tokens;
	int pairs = t->size /2;

	//若不在object中 或者空obj则报错
	if(t->type != JSMN_OBJECT || pairs == 0)
		return NULL;

	t += 1;
	while(TOKEN_VALID(t) && pairs > 0){
		if((strlen(key) == (t->end - t->start)) && (0 == strncmp(key, js + t->start, t->end - t->start)))
			return t+1;
		t = jsmn_next(t+1);

		pairs --;
	}
	return NULL;
}

//输入array开头解析数组 和 idx
jsmntok_t* jsmn_array_value(const char* js, jsmntok_t* tokens, u32_t idx)
{
	jsmntok_t* t = tokens;
	u32_t num = t->size;

	//若不是array 或者空 则返回null
	if(t->type != JSMN_ARRAY || num == 0 || idx >= num)
		return NULL;

	t += 1;	//跳过外框

    while(idx > 0 && t != NULL) {
        t = jsmn_next(t);
        idx--;
    }
    return t;
}

int jsmn_tkn2val_u64(const char *js ,jsmntok_t * tk, u64_t *val)
{
	if(NULL == tk || tk->type != JSMN_PRIMITIVE || !arch_isdigit(js[tk->start])) {
        return STATE_NOTFOUND;
    }

	if(val)
		*val = arch_atou64n( (const char *)&(js[tk->start]), tk->end - tk->start);

    return STATE_OK;
}


//通过tkn取得 u32_t
// tk 需要指向 整个对象
int jsmn_tkn2val_uint(const char* js, jsmntok_t* tk, u32_t* val)
{
	if(NULL == tk || tk->type != JSMN_PRIMITIVE || !arch_isdigit(js[tk->start]))
		return STATE_NOTFOUND;
	*val = arch_atoun( (const char *)&(js[tk->start]), tk->end - tk->start);
	return STATE_OK;
}


int jsmn_tkn2val_u16(const char* js, jsmntok_t* tk, u16_t* val)
{
	int ret;
	u32_t tmp;
	ret = jsmn_tkn2val_uint(js, tk, &tmp);
	if(STATE_OK == ret)*val = tmp;
	return ret;
}

int jsmn_tkn2val_u8(const char* js, jsmntok_t* tk, u8_t* val)
{
	int ret;
	u32_t tmp;
	ret = jsmn_tkn2val_uint(js, tk, &tmp);
	if(STATE_OK == ret)*val = tmp;
	return ret;
}

int jsmn_tkn2val_s64(const char *js ,jsmntok_t * tk, s64_t *val)
{
    if(NULL == tk || tk->type != JSMN_PRIMITIVE || (!arch_isdigit(js[tk->start]) && js[tk->start] != '-'))
        return STATE_NOTFOUND;

    if(val)
        *val = arch_atos64n( (const char *)&(js[tk->start]), tk->end - tk->start);

    return STATE_OK;
}

int jsmn_tkn2val_sint(const char* js, jsmntok_t* tk, s32_t* val)
{
    if(NULL == tk || tk->type != JSMN_PRIMITIVE || (!arch_isdigit(js[tk->start]) && js[tk->start] != '-'))
        return STATE_NOTFOUND;
    *val = arch_atoi( (const char *)&(js[tk->start]), tk->end - tk->start);
    return STATE_OK;
}

int jsmn_tkn2val_s16(const char* js, jsmntok_t* tk, s16_t* val)
{
    int ret;
    s32_t tmp;
    ret = jsmn_tkn2val_sint(js, tk, &tmp);
    if(STATE_OK == ret)*val = tmp;
    return ret;
}

int jsmn_tkn2val_s8(const char* js, jsmntok_t* tk, s8_t* val)
{
    int ret;
    s32_t tmp;
    ret = jsmn_tkn2val_sint(js, tk, &tmp);
    if(STATE_OK == ret)*val = tmp;
    return ret;
}


int jsmn_tkn2val_str(const char* js, jsmntok_t* tk, char* str, size_t str_size, size_t *str_len)
{
	if(NULL == tk || tk->type != JSMN_STRING)return STATE_NOTFOUND;
	size_t rlen = str_unescape_to_buf(&(js[tk->start]), tk->end - tk->start, str, str_size);
	if(str_len)*str_len = rlen;
	return STATE_OK;
}

int jsmn_tkn2val_xbuf(const char* js, jsmntok_t* tk, u8_t* buf, size_t buf_size, size_t *buf_len)
{
	if(NULL == tk || tk->type != JSMN_STRING)return STATE_NOTFOUND;
	size_t rlen =  arch_axtobuf_detail(&(js[tk->start]), tk->end - tk->start, buf, buf_size, NULL);
	if(buf_len)*buf_len = rlen;
	return STATE_OK;
}

int jsmn_tkn2val_bool(const char* js, jsmntok_t* tk, u8_t* val)
{
	if(NULL == tk || tk->type != JSMN_PRIMITIVE)return STATE_NOTFOUND;
	if(js[tk->start] == 't')*val = TRUE;
	else if(js[tk->start] == 'f')*val = FALSE;
	else return STATE_NOTFOUND;
	return STATE_OK;
}

int jsmn_key2val_u64(const char *js ,jsmntok_t* tokens, const char* key, u64_t* val)
{
    return jsmn_tkn2val_u64(js, jsmn_key_value(js, tokens, key), val);
}

//通过key取得 u32_t
// tokens 需要指向 整个对象
int jsmn_key2val_uint(const char* js, jsmntok_t* tokens, const char* key, u32_t* val)
{
	return jsmn_tkn2val_uint(js, jsmn_key_value(js, tokens, key), val);
}

int jsmn_key2val_u16(const char* js, jsmntok_t* tokens, const char* key, u16_t* val)
{
	return jsmn_tkn2val_u16(js, jsmn_key_value(js, tokens, key), val);
}

int jsmn_key2val_u8(const char* js, jsmntok_t* tokens, const char* key, u8_t* val)
{
	return jsmn_tkn2val_u8(js, jsmn_key_value(js, tokens, key), val);
}


int jsmn_key2val_str(const char* js, jsmntok_t* tokens, const char* key, char* str, size_t str_size, size_t *str_len)
{
	return jsmn_tkn2val_str(js, jsmn_key_value(js, tokens, key), str, str_size, str_len);
}

int jsmn_key2val_xbuf(const char* js, jsmntok_t* tokens, const char* key, u8_t* buf, size_t buf_size, size_t *buf_len)
{
	return jsmn_tkn2val_xbuf(js, jsmn_key_value(js, tokens, key), buf, buf_size, buf_len);
}

int jsmn_key2val_bool(const char* js, jsmntok_t* tokens, const char* key, u8_t* val)
{
	return jsmn_tkn2val_bool(js, jsmn_key_value(js, tokens, key), val);
}


