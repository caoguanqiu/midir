/*
	改动:
	1、jsmntok_t 改为更少的占用6bytes
	2、jsmn_parse 添加长度参数 可以兼容'\0'结尾 或者长度限制 两种结束方式
*/
#include "miio_arch.h"
#include <lib/jsmn/jsmn.h>

/**
 * Allocates a fresh unused token from the token pull.
 */
static jsmntok_t *jsmn_alloc_token(jsmn_parser *parser,
		jsmntok_t *tokens, size_t num_tokens) {
	jsmntok_t *tok;
	if (parser->toknext >= num_tokens) {
		return NULL;
	}
	tok = &tokens[parser->toknext++];
	tok->start = tok->end = -1;
	tok->size = 0;
#ifdef JSMN_PARENT_LINKS
	tok->parent = -1;
#endif
	return tok;
}

/**
 * Fills token type and boundaries.
 */
static void jsmn_fill_token(jsmntok_t *token, jsmntype_t type,
                            int start, int end) {
	token->type = type;
	token->start = start;
	token->end = end;
	token->size = 0;
}

/**
 * Fills next available token with JSON primitive.
 */

 #ifdef JSMN_OPTIMIZE
 static jsmnerr_t jsmn_parse_primitive(jsmn_parser *parser, const char *js, int js_len,
		jsmntok_t *tokens, size_t num_tokens)
#else
static jsmnerr_t jsmn_parse_primitive(jsmn_parser *parser, const char *js,
		jsmntok_t *tokens, size_t num_tokens)
#endif

{
	jsmntok_t *token;
	int start;

	start = parser->pos;

#ifdef JSMN_OPTIMIZE
	for (; js[parser->pos] != '\0' && parser->pos < js_len; parser->pos++)
#else
	for (; js[parser->pos] != '\0'; parser->pos++)
#endif
	 {
		switch (js[parser->pos]) {
#ifndef JSMN_STRICT
			/* In strict mode primitive must be followed by "," or "}" or "]" */
			case ':':
#endif
			case '\t' : case '\r' : case '\n' : case ' ' :
			case ','  : case ']'  : case '}' :
				goto found;
		}
		if (js[parser->pos] < 32 || js[parser->pos] >= 127) {
			parser->pos = start;
			return JSMN_ERROR_INVAL;
		}
	}
#ifdef JSMN_STRICT
	/* In strict mode primitive must be followed by a comma/object/array */
	parser->pos = start;
	return JSMN_ERROR_PART;
#endif

found:
	token = jsmn_alloc_token(parser, tokens, num_tokens);
	if (token == NULL) {
		parser->pos = start;
		return JSMN_ERROR_NOMEM;
	}
	jsmn_fill_token(token, JSMN_PRIMITIVE, start, parser->pos);
#ifdef JSMN_PARENT_LINKS
	token->parent = parser->toksuper;
#endif
	parser->pos--;
	return JSMN_SUCCESS;
}

/**
 * Filsl next token with JSON string.
 */
#ifdef JSMN_OPTIMIZE
static jsmnerr_t jsmn_parse_string(jsmn_parser *parser, const char *js, int js_len,
		jsmntok_t *tokens, size_t num_tokens)
#else
static jsmnerr_t jsmn_parse_string(jsmn_parser *parser, const char *js,
		jsmntok_t *tokens, size_t num_tokens)
#endif

{

	jsmntok_t *token;

	int start = parser->pos;		//记录头位置

	parser->pos++;	//越过"

	/* Skip starting quote */
#ifdef JSMN_OPTIMIZE
	for (; js[parser->pos] != '\0'&& parser->pos < js_len ; parser->pos++) 	//直到尾部
#else
	for (; js[parser->pos] != '\0'; parser->pos++)
#endif


	{
		char c = js[parser->pos];

		/* Quote: end of string */
		if (c == '\"') {		//定位到尾部
			token = jsmn_alloc_token(parser, tokens, num_tokens);
			if (token == NULL) {
				parser->pos = start;
				return JSMN_ERROR_NOMEM;
			}

			jsmn_fill_token(token, JSMN_STRING, start+1, parser->pos);
#ifdef JSMN_PARENT_LINKS
			token->parent = parser->toksuper;
#endif
			return JSMN_SUCCESS;
		}

		/* Backslash: Quoted symbol expected */
		if (c == '\\') {
			parser->pos++;							//简单越过
			switch (js[parser->pos]) {
				/* Allowed escaped symbols */
				case '\"': case '/' : case '\\' : case 'b' :
				case 'f' : case 'r' : case 'n'  : case 't' :
					break;

				/* Allows escaped symbol \uXXXX */
				case 'u':
					/* TODO */
					break;

				/* Unexpected symbol */
				default:
					parser->pos = start;
					return JSMN_ERROR_INVAL;
			}
		}


	}
	parser->pos = start;
	return JSMN_ERROR_PART;
}

/**
 * Parse JSON string and fill tokens.
 */

#ifdef JSMN_OPTIMIZE
jsmnerr_t jsmn_parse(jsmn_parser *parser, const char *js, int js_len, jsmntok_t *tokens, unsigned int num_tokens)
#else
jsmnerr_t jsmn_parse(jsmn_parser *parser, const char *js, jsmntok_t *tokens, unsigned int num_tokens)
#endif

{
	jsmnerr_t r;
	int i;
	jsmntok_t *token;


#ifdef JSMN_OPTIMIZE
	for (; js[parser->pos] != '\0' && parser->pos < js_len; parser->pos++)
#else
	for (; js[parser->pos] != '\0'; parser->pos++)
#endif
	 {

		char c;
		jsmntype_t type;

		c = js[parser->pos];
		switch (c) {



			case '{': case '[':
				token = jsmn_alloc_token(parser, tokens, num_tokens);
				if (token == NULL)
					return JSMN_ERROR_NOMEM;
				if (parser->toksuper != -1) {
					tokens[parser->toksuper].size++;
#ifdef JSMN_PARENT_LINKS
					token->parent = parser->toksuper;
#endif
				}
				token->type = (c == '{' ? JSMN_OBJECT : JSMN_ARRAY);
				token->start = parser->pos;
				parser->toksuper = parser->toknext - 1;	//上一层 改为 上一个token
				break;




			case '}': case ']':
				type = (c == '}' ? JSMN_OBJECT : JSMN_ARRAY);
#ifdef JSMN_PARENT_LINKS
				if (parser->toknext < 1) {
					return JSMN_ERROR_INVAL;
				}
				token = &tokens[parser->toknext - 1];	//当前token(尾部的token)

				for (;;) {
					if (token->start != -1 && token->end == -1) {
						if (token->type != type) {
							return JSMN_ERROR_INVAL;
						}
						token->end = parser->pos + 1;
						parser->toksuper = token->parent;
						break;
					}
					if (token->parent == -1) {
						break;
					}
					token = &tokens[token->parent];
				}
#else

				for (i = parser->toknext - 1; i >= 0; i--) {	//回溯
					token = &tokens[i];
					if (token->start != -1 && token->end == -1) {		//找到还没有结尾的token 就是 { [ 的tk
						if (token->type != type) {
							return JSMN_ERROR_INVAL;
						}
						parser->toksuper = -1;
						token->end = parser->pos + 1;		//结尾之
						break;
					}
				}

				/* Error if unmatched closing bracket */
				if (i == -1) return JSMN_ERROR_INVAL;	//找到头也没有

				for (; i >= 0; i--) {				//接着回溯
					token = &tokens[i];
					if (token->start != -1 && token->end == -1) {		//找到再上一级的对象tk
						parser->toksuper = i;
						break;
					}
				}
#endif
				break;





			case '\"':
#ifdef JSMN_OPTIMIZE
				r = jsmn_parse_string(parser, js, js_len, tokens, num_tokens);
#else
				r = jsmn_parse_string(parser, js, tokens, num_tokens);
#endif
				if (r < 0) return r;

				if (parser->toksuper != -1)		//如果有上一层 那么为其添加数量
					tokens[parser->toksuper].size++;
				break;






			case '\t' : case '\r' : case '\n' : case ':' : case ',': case ' ': 	//越过空白字符
				break;


#ifdef JSMN_STRICT
			/* In strict mode primitives are: numbers and booleans */
			case '-': case '0': case '1' : case '2': case '3' : case '4':
			case '5': case '6': case '7' : case '8': case '9':
			case 't': case 'f': case 'n' :
#else
			/* In non-strict mode every unquoted value is a primitive */
			default:
#endif

#ifdef JSMN_OPTIMIZE
				r = jsmn_parse_primitive(parser, js, js_len, tokens, num_tokens);
#else
				r = jsmn_parse_primitive(parser, js, tokens, num_tokens);
#endif

				if (r < 0) return r;
				if (parser->toksuper != -1)
					tokens[parser->toksuper].size++;
				break;

#ifdef JSMN_STRICT
			/* Unexpected char in strict mode */
			default:
				return JSMN_ERROR_INVAL;
#endif

		}
	}

	for (i = parser->toknext - 1; i >= 0; i--) {	//回溯
		/* Unmatched opened object or array */
		if (tokens[i].start != -1 && tokens[i].end == -1) {		//若是还能找到未圆满的tk
			return JSMN_ERROR_PART;
		}
	}

	return JSMN_SUCCESS;
}

/**
 * Creates a new parser based over a given  buffer with an array of tokens
 * available.
 */
void jsmn_init(jsmn_parser *parser) {
	parser->pos = 0;
	parser->toknext = 0;
	parser->toksuper = -1;
}

