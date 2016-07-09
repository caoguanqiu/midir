
//#include "wrapper_string.h"
#include "miio_arch.h"
#include <util/util.h>


static __inline s32_t divide(s32_t *n, s32_t base)
{
	s32_t res;

	/* optimized for processor which does not support divide instructions. */
	if (base == 10)
	{
		res = ((u32_t)*n) % 10U;
		*n = ((u32_t)*n) / 10U;
	}
	else
	{
		res = ((u32_t)*n) % 16U;
		*n = ((u32_t)*n) / 16U;
	}

	return res;
}



static __inline int skip_atoi(const char **s)
{
	register int i = 0;
	while (arch_isdigit((int)**s))
		i = i * 10 + *((*s)++) - '0';
	return i;
}


#define ZEROPAD 	(1 << 0)	/* pad with zero */
#define SIGN 		(1 << 1)	/* unsigned/signed long */
#define PLUS 		(1 << 2)	/* show plus */
#define SPACE 		(1 << 3)	/* space if plus */
#define LEFT 		(1 << 4)	/* left justified */
#define SPECIAL 	(1 << 5)	/* 0x */
#define LARGE		(1 << 6)	/* use 'ABCDEF' instead of 'abcdef' */

#ifdef PRINTF_PRECISION
static char *print_number(char * buf, char * end, long num, int base, int s, int precision, int type)
#else
static char *print_number(char * buf, char * end, long num, int base, int s, int type)
#endif
{
	char c, sign;
#ifdef PRINTF_LONGLONG
	char tmp[32];
#else
	char tmp[16];
#endif
	const char *digits;
	static const char small_digits[] = "0123456789abcdef";
	static const char large_digits[] = "0123456789ABCDEF";
	register int i;
	register int size;

	size = s;

	digits = (type & LARGE) ? large_digits : small_digits;
	if (type & LEFT) type &= ~ZEROPAD;

	c = (type & ZEROPAD) ? '0' : ' ';

	/* get sign */
	sign = 0;
	if (type & SIGN)
	{
		if (num < 0)
		{
			sign = '-';
			num = -num;
		}
		else if (type & PLUS) sign = '+';
		else if (type & SPACE) sign = ' ';
	}

#ifdef PRINTF_SPECIAL
	if (type & SPECIAL)
	{
		if (base == 16) size -= 2;
		else if (base == 8) size--;
	}
#endif

	i = 0;
	if (num == 0) tmp[i++]='0';
	else
	{
		while (num != 0) tmp[i++] = digits[divide(&num, base)];
	}

#ifdef PRINTF_PRECISION
	if (i > precision) precision = i;
	size -= precision;
#else
	size -= i;
#endif

	if (!(type&(ZEROPAD | LEFT)))
	{
		while(size-->0)
		{
			if (buf <= end) *buf = ' ';
			++buf;
		}
	}

	if (sign)
	{
		if (buf <= end)
		{
			*buf = sign;
			--size;
		}
		++buf;
	}

#ifdef PRINTF_SPECIAL
	if (type & SPECIAL)
	{
		if (base==8)
		{
			if (buf <= end) *buf = '0';
			++buf;
		}
		else if (base==16)
		{
			if (buf <= end) *buf = '0';
			++buf;
			if (buf <= end)
			{
				*buf = type & LARGE? 'X' : 'x';
			}
			++buf;
		}
	}
#endif

	/* no align to the left */
	if (!(type & LEFT))
	{
		while (size-- > 0)
		{
			if (buf <= end) *buf = c;
			++buf;
		}
	}

#ifdef PRINTF_PRECISION
	while (i < precision--)
	{
		if (buf <= end) *buf = '0';
		++buf;
	}
#endif

	/* put number in the temporary buffer */
	while (i-- > 0)
	{
		if (buf <= end) *buf = tmp[i];
		++buf;
	}

	while (size-- > 0)
	{
		if (buf <= end) *buf = ' ';
		++buf;
	}

	return buf;
}

/*
CR '\r' 0x0D; 回车符,c语言'\r'
LF  '\n' 0x0A; 换行符, c语言'\n'
*/
//#define LF_TO_CRLF

//1 注意：该函数尾部总是补0，不需要外部再添加0
//1注意：该函数返回为输出串的长度，不受输入空间长度影响，若size不够输出，该函数不会溢出覆盖

static s32_t __vsnprintf(char *buf, int size, const char *fmt, va_list args)
{
#ifdef PRINTF_LONGLONG
	unsigned long long num;
#else
	u32_t num;
#endif
	int i, len;
	char *str, *end, c;
	const char *s;

	u8_t base;			/* the base of number */
	u8_t flags;			/* flags to print number */
	u8_t qualifier;		/* 'h', 'l', or 'L' for integer fields */
	s32_t field_width;	/* width of output field */

#ifdef PRINTF_PRECISION
	int precision;		/* min. # of digits for integers and max for a string */
#endif

	str = buf;
	end = buf + size - 1;

	/* Make sure end is always >= buf */
	if (end < buf)
	{
		//end = ((char *)-1);
		//size = end - buf;
		//之前处理是虚构近似无限空间
		//改为无作为
		//这个防止size输入负数，导致问题
		return 0;
	}

	for (; *fmt ; ++fmt)
	{
		if (*fmt != '%')
		{

		#ifdef LF_TO_CRLF
			if(str <= end && *fmt == '\n')
				*str++ = '\r';
		#endif

			if (str <= end)
				*str = *fmt;
			++str;
			continue;
		}

		/* process flags */
		flags = 0;

		while(1)
		{
			/* skips the first '%' also */
			++fmt;
			if (*fmt == '-') flags |= LEFT;
			else if (*fmt == '+') flags |= PLUS;
			else if (*fmt == ' ') flags |= SPACE;
			else if (*fmt == '#') flags |= SPECIAL;
			else if (*fmt == '0') flags |= ZEROPAD;
			else break;
		}

		/* get field width */
		field_width = -1;
		if (arch_isdigit((int)*fmt)) field_width = skip_atoi(&fmt);
		else if (*fmt == '*')
		{
			++fmt;
			/* it's the next argument */
			field_width = va_arg(args, int);
			if (field_width < 0)
			{
				field_width = -field_width;
				flags |= LEFT;
			}
		}

#ifdef PRINTF_PRECISION
		/* get the precision */
		precision = -1;
		if (*fmt == '.')
		{
			++fmt;
			if (arch_isdigit((int)*fmt)) precision = skip_atoi(&fmt);
			else if (*fmt == '*')
			{
				++fmt;
				/* it's the next argument */
				precision = va_arg(args, int);
			}
			if (precision < 0) precision = 0;
		}
#endif
		/* get the conversion qualifier */
		qualifier = 0;
		if (*fmt == 'h' || *fmt == 'l'
#ifdef PRINTF_LONGLONG
				|| *fmt == 'L'
#endif
		   )
		{
			qualifier = *fmt;
			++fmt;
#ifdef PRINTF_LONGLONG
			if (qualifier == 'l' && *fmt == 'l')
			{
				qualifier = 'L';
				++fmt;
			}
#endif
		}

		/* the default base */
		base = 10;

		switch (*fmt)
		{
		case 'c':
			if (!(flags & LEFT))
			{
				while (--field_width > 0)
				{
					if (str <= end) *str = ' ';
					++str;
				}
			}

			/* get character */
			c = (u8_t) va_arg(args, int);
			if (str <= end) *str = c;
			++str;

			/* put width */
			while (--field_width > 0)
			{
				if (str <= end) *str = ' ';
				++str;
			}
			continue;

		case 's':
			s = va_arg(args, char *);
			if (!s) s = "(NULL)";

			len = strlen(s);
#ifdef PRINTF_PRECISION
			if (precision > 0 && len > precision) len = precision;
#endif

			if (!(flags & LEFT))
			{
				while (len < field_width--)
				{
					if (str <= end) *str = ' ';
					++str;
				}
			}

			for (i = 0; i < len; ++i)
			{
				if (str <= end) *str = *s;
				++str;
				++s;
			}

			while (len < field_width--)
			{
				if (str <= end) *str = ' ';
				++str;
			}
			continue;

		case 'p':
			if (field_width == -1)
			{
				field_width = sizeof(void *) << 1;
				flags |= ZEROPAD;
			}
#ifdef PRINTF_PRECISION
			str = print_number(str, end,
							   (long) va_arg(args, void *),
							   16, field_width, precision, flags);
#else
			str = print_number(str, end,
							   (long) va_arg(args, void *),
							   16, field_width, flags);
#endif
			continue;

		case '%':
			if (str <= end) *str = '%';
			++str;
			continue;

			/* integer number formats - set up the flags and "break" */
		case 'o':
			base = 8;
			break;

		case 'X':
			flags |= LARGE;
		case 'x':
			base = 16;
			break;

		case 'd':
		case 'i':
			flags |= SIGN;
		case 'u':
			break;

		default:
			if (str <= end) *str = '%';
			++str;

			if (*fmt)
			{
				if (str <= end) *str = *fmt;
				++str;
			}
			else
			{
				--fmt;
			}
			continue;
		}

#ifdef PRINTF_LONGLONG
		if (qualifier == 'L') num = va_arg(args, long long);
		else if (qualifier == 'l')
#else
		if (qualifier == 'l')
#endif
		{
			num = va_arg(args, u32_t);
			if (flags & SIGN) num = (s32_t) num;
		}
		else if (qualifier == 'h')
		{
			num = (u16_t) va_arg(args, s32_t);
			if (flags & SIGN) num = (s16_t) num;
		}
		else
		{
			num = va_arg(args, u32_t);
			if (flags & SIGN) num = (s32_t) num;
		}
#ifdef PRINTF_PRECISION
		str = print_number(str, end, num, base, field_width, precision, flags);
#else
		str = print_number(str, end, num, base, field_width, flags);
#endif
	}

	if (str <= end) *str = '\0';
	else *end = '\0';

	/* the trailing null byte doesn't count towards the total
	* ++str;
	*/
	return str-buf;
}



/**
 * This function will fill a formatted string to buffer
 *
 * @param buf the buffer to save formatted string
 * @param arg_ptr the arg_ptr
 * @param format the format
 */
s32_t arch_vsprintf(char *buf, const char *format, va_list arg_ptr)
{
	return __vsnprintf(buf, (int)65535, format, arg_ptr);
}


//1 注意：该函数尾部总是补0，不需要外部再添加0

s32_t arch_vsnprintf(char *buf, int size, const char *format, va_list arg_ptr)
{
	return __vsnprintf(buf, size, format, arg_ptr);
}

/**
 * This function will fill a formatted string to buffer
 *
 * @param buf the buffer to save formatted string
 * @param format the format
 */

s32_t arch_sprintf(char *buf ,const char *format,...)
{
	s32_t n;
	va_list arg_ptr;

	va_start(arg_ptr, format);
	n = arch_vsprintf(buf ,format,arg_ptr);
	va_end (arg_ptr);

	return n;
}




//1 注意：该函数尾部总是补0，不需要外部再添加0
s32_t arch_snprintf(char *buf, int size, const char *fmt, ...)
{
	s32_t n;
	va_list args;

	va_start(args, fmt);
	n = arch_vsnprintf(buf, size, fmt, args);
	va_end(args);

	return n;
}






/* ----------------sscanf implementatiuon----------------- */


//
// Reduced version of scanf (%d, %x, %c, %n are supported)
// %d dec integer (E.g.: 12)
// %x hex integer (E.g.: 0xa0)
// %b bin integer (E.g.: b1010100010)
// %n hex, de or bin integer (e.g: 12, 0xa0, b1010100010)
// %c any character
//
int arch_rsscanf(const char* str, const char* format, ...)
{
	va_list ap;
	int value, tmp;
	int count;
	int pos;
	char neg, fmt_code;

	va_start(ap, format);

	for (count = 0; *format != 0 && *str != 0; )
	{
		//格式串skip空格
		while (*format == ' ')format++;
		if (*format == 0)break;

		//目标串skip空格
		while (*str == ' ')str++;
		if (*str == 0)break;


		//格式串遇到特殊处理
		if (*format == '%')
		{
			format++;
			if (*format == 'n')
			{
				if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X'))
				{
				    fmt_code = 'x';
				    str += 2;
				}

				else
				if (str[0] == 'b')
				{
				    fmt_code = 'b';
				    str++;
				}
				else
				    fmt_code = 'd';
			}
			else
				fmt_code = *format;

			switch (fmt_code)
			{
			case 'x':
			case 'X':
				for (value = 0, pos = 0; *str != 0; str++, pos++)
				{
					if ('0' <= *str && *str <= '9')
						tmp = *str - '0';
					else
					if ('a' <= *str && *str <= 'f')
						tmp = *str - 'a' + 10;
					else
					if ('A' <= *str && *str <= 'F')
						tmp = *str - 'A' + 10;
					else
						break;

					value *= 16;
					value += tmp;
				}
				if (pos == 0)
					return count;
				*(va_arg(ap, int*)) = value;
				count++;
				break;

			case 'b':
				for (value = 0, pos = 0; *str != 0; str++, pos++)
				{
					if (*str != '0' && *str != '1')
						break;
					value *= 2;
					value += *str - '0';
				}
				if (pos == 0)
					return count;
				*(va_arg(ap, int*)) = value;
				count++;
				break;

			case 'd':
				if (*str == '-')
				{
					neg = 1;
					str++;
				}
				else
					neg = 0;

				for (value = 0, pos = 0; *str != 0; str++, pos++)
				{
					if ('0' <= *str && *str <= '9')
						value = value*10 + (int)(*str - '0');
					else
						break;
				}

				if (pos == 0)return count;

				*(va_arg(ap, int*)) = neg ? -value : value;
				count++;
				break;

			case 'c':
				*(va_arg(ap, char*)) = *str;
				count++;
				break;

			default:
				return count;
			}


			format++;
		}


		//非特殊处理
		else
		{
			if (*format != *str)
				break;

			format++;
			str++;
		}

	}

	va_end(ap);

	return count;
}






/*
style:
bit7: if hex in upcase form
bit0~6: indicate spliter char
*/
int snprintf_hex(char *buf, size_t buf_size, const u8_t *data,
				    size_t len, char style)
{
	size_t i;
	char spliter,uppercase;
	char *pos = buf, *end = buf + buf_size;
	int n;
	if (buf_size == 0)
		return 0;

	spliter = style & 0x7F;
	uppercase = (style & 0x80)?1:0;

	if(!arch_isprint(spliter))
		spliter = 0;



	for (i = 0; i < len; i++) {
		//若需要打印分隔符
		if(i < len-1 && spliter)
			n = arch_snprintf(pos, end - pos, (uppercase? "%02X%c" : "%02x%c"), data[i], spliter);
		else
			n = arch_snprintf(pos, end - pos, (uppercase? "%02X" : "%02x"), data[i]);
		pos += n;

		//如果将要溢出，那么尾部补0，退出
		if (pos >= end) break;
	}

	return pos - buf;
}


// in format '003:32:06.020'
int snprint_time_stamp(u32_t tick, char* buf, int size)
{
	u32_t hour,min,sec,ms;

	hour = tick/(1000 * 3600);
	tick %= (1000 * 3600);

	min = tick/(1000 * 60);
	tick %= (1000 * 60);

	sec = tick/(1000);
	tick %= (1000);

	ms = tick*(1000/1000);
	return arch_snprintf(buf, size, "%03u:%02u:%02u.%03u: ",hour, min, sec, ms);
}


//将内存中字符串 转换为 可以使用""包裹的 带有转义字符的字符串

//len 输入最大长度
//size输出最大长度
int str_escape_from_buf(char* src, size_t src_len, char* dest, size_t dest_size)
{
	char* ptr = dest;
	char* end = dest+dest_size;
	while (*src && src_len && ptr+1 < end) {
		switch(*((u8_t*)src)) {
			case '\"': *ptr++ = '\\'; *ptr++ = '\"'; break;
			case '\\': *ptr++ = '\\'; *ptr++ = '\\'; break;
//			case '\/': *ptr++ = '\\'; *ptr++ = '\/'; break;
			case '\b': *ptr++ = '\\'; *ptr++ = 'b'; break;
			case '\f': *ptr++ = '\\'; *ptr++ = 'f'; break;
			case '\n': *ptr++ = '\\'; *ptr++ = 'n'; break;
			case '\r': *ptr++ = '\\'; *ptr++ = 'r'; break;
			case '\t': *ptr++ = '\\'; *ptr++ = 't'; break;
			default: *ptr++ = *((u8_t*)src);
		}
		src++; src_len--;
	}
	if(ptr < end)*ptr = '\0';
	return ptr-dest;
}







//将带有转义表示的串转化为 内存中字符串
//输入指针、长度，输出新长度
//新串头部设置$(取代")，尾部设置'\0'，空余设置为空格
/*
\a	响铃(BEL)	007
\b	退格(BS) ，将当前位置移到前一列	008
\f	换页(FF)，将当前位置移到下页开头	012
\n	换行(LF) ，将当前位置移到下一行开头	010
\r	回车(CR) ，将当前位置移到本行开头	013
\t	水平制表(HT) （跳到下一个TAB位置）	009
\v	垂直制表(VT)	011
\\	代表一个反斜线字符''\'	092
\'	代表一个单引号（撇号）字符	039
\"	代表一个双引号字符	034
\0	空字符(NULL)	000
\/	Slash	047
*/

int str_unescape_to_buf(const char* src, size_t src_len, char* dst, size_t dst_size)
{
	const char *src_org;
	char *dst_org;

	src_org = src;
	dst_org = dst;

	while(src - src_org < src_len && dst - dst_org < dst_size-1){	//dst尾部需要加0

		if(*src == '\\'){	//若是转义字符
			src++;
			switch(*src){
			case 'a':*dst = 7;break;
			case 'b':*dst = 8;break;
			case 'f':*dst = 12;break;
			case 'n':*dst = 10;break;
			case 'r':*dst = 13;break;
			case 't':*dst = 9;break;
			case 'v':*dst = 11;break;
			case '\\':*dst = 92;break;
			case '\'':*dst = 39;break;
			case '"':*dst = 34;break;
			case '/':*dst = 47;break;
			case '0':	*dst = 0;
				//("invalid char(\'\\0\') in string.");
				break;
			}
			dst++;
			src++;
		}



		else		//若非转义字符
			*dst++ = *src++;


	};


	*dst = '\0';
	return dst - dst_org;
}






//判断内存是否全部填充某个字符
//返回长度 和 样本长度相同 表示匹配
//小于样本长度 表示发现不匹配的首位置
u32_t memchcmp(const void* s, u8_t c, size_t n)
{
	u32_t u = 0;
	u8_t* p = (u8_t*)s;

	for(u = 0; u < n; u++)
		if(c != p[u])break;

	return u;
}


//检查str是否以start开头，若通过，返回start长度，否则返回0
 int arch_strstart(const char *str, const char *start)
{
	int ret = 0;

	while (*str == *start && *str != '\0') {
		str++;start++;
		ret++;
	}

	if (*start == '\0')
		return ret;
	else
		return 0;
}





//转换为无符号数
//n表示读取数字字符的最大长度
u32_t arch_atoun(const char* c, size_t n)
{
	u32_t dig = 0;
	const char *org = c;
	while(arch_isdigit((int)*c) && (c-org < n) ){
		dig = dig*10 + *c - '0';
		c++;
	}
	return dig;
}

s32_t arch_atoi(const char * c, size_t n)
{
    if(*c == '-')
        return 0 - (s32_t) arch_atoun(c + 1, n - 1);
    else
        return (s32_t) arch_atoun(c, n);
}

//转换为无符号数
//len返回采样数字字符的长度
u32_t arch_atou_len(const char* c, size_t* len)
{
	u32_t dig = 0;
	const char *org = c;
	while(arch_isdigit((int)*c)){
		dig = dig*10 + *c - '0';
		c++;
	}
	*len = c-org;
	return dig;
}

//16进制数串转换为无符号数
u32_t arch_axtou(const char* c)
{
	u32_t dig = 0;
//	const char *org = c;
	while(arch_isxdigit((int)*c)){
		dig <<= 4;
		dig += 0x0000000F & hex_char_value((int)*c);
		c++;
	}
	return dig;
}

//16进制数串转换为无符号数 通过len 返回采样数字字符的长度
u32_t arch_axtou_len(const char* c, size_t* len)
{
	int dig = 0;
	const char *org = c;
	while(arch_isxdigit((int)*c)){
		dig <<= 4;
		dig+= 0x0000000F & hex_char_value((int)*c);
		c++;
	}
	*len = c-org;
	return dig;
}


//16进制数串转换为无符号数 n限制采样字符数
u32_t arch_axtoun(const char* c, size_t n)
{
	int dig = 0;
	const char *org = c;
	while(arch_isxdigit((int)*c) && (c-org < n) ){
		dig = dig*16 + hex_char_value((int)*c);
		c++;
	}
	return dig;
}

//将HEX字符串转换为二进制数组
//in尺寸有限制
//out空间有限制
//in_len输出已处理输入长度

//返回输出长度
//arch_axtobuf("12EF985xyz",out) 返回6，最后一个'5'被抛弃
size_t arch_axtobuf_detail(const char* in, size_t in_size, u8_t* out, size_t out_size, size_t *in_len)
{
	const char *org_in = in;
	u8_t *org_out = out;

	while( arch_isxdigit((int)*in) && arch_isxdigit((int)*(in+1)) &&	//输入不硌牙
			(in  - org_in <  in_size) && (out - org_out < out_size)){	// 不超限

		*out = (0x0F & hex_char_value((int)*(in+1))) | (0xF0 & (hex_char_value((int)*in) << 4));	//转换处理

		in += 2; out += 1;	//调整指针
 	}

	if(in_len)
		*in_len = in  - org_in;

	return out - org_out;
}



/* 64位乘法运算
c3:c2:c1:c0 = a1:a0 * b1:b0
(1) a0 * b0 = d1:d0        // d1 是高 32 位，d0 是低 32 位
(2) a1 * b0 = e1:e0        // e1 是高 32 位，e0 是低 32 位
(3) a0 * b1 = f1:f0        // f1 是高 32 位，f0 是低 32 位
(4) a1 * b1 = h1:h0        // h1 是高 32 位，h0 是低 32 位

(1) c0 = d0
(2) c1 = d1 + e0 + f0
(3) c2 = e1 + f1 + h0 + carry	//这个 carry 进位值是由第 2 个加法式子得到
(4) c3 = h1 + carry				//这个 carry 进位值是由第 3 个加法式子得到
*/
#define SET_ZERO(d)  memset(&(d), 0, sizeof((d)))

int mulu64(du32_t *a, du32_t *b, du64_t *out )
{
	du64_t c;
	du32_t d,e,f,h;
	u8_t carry = 0;

	SET_ZERO(c);
	SET_ZERO(d);
	SET_ZERO(e);
	SET_ZERO(f);
	SET_ZERO(h);


	d.d32.u0 = a->d16.u0 * b->d16.u0;
	e.d32.u0 = a->d16.u1 * b->d16.u0;
	f.d32.u0 = a->d16.u0 * b->d16.u1;
	h.d32.u0 = a->d16.u1 * b->d16.u1;

	c.d16.u0 = d.d16.u0;


	c.d16.u1 = d.d16.u1 + e.d16.u0 + f.d16.u0;
	if(d.d16.u1 + e.d16.u0 + f.d16.u0 > 65535)
		carry = 1;
	else
		carry = 0;

	c.d16.u2 = e.d16.u1 + f.d16.u1 + h.d16.u0 + carry;
	if(e.d16.u1 + f.d16.u1 + h.d16.u0 + carry > 65535)
		carry = 1;
	else
		carry = 0;

	c.d16.u3 = h.d16.u1 + carry;

	memcpy(out, &c, sizeof(du64_t));

	return 0;
}



int mulu128(du64_t *a, du64_t *b, du128_t *out )
{
	du128_t c;
	du64_t d,e,f,h;
	u8_t carry = 0;

	SET_ZERO(c);
	SET_ZERO(d);
	SET_ZERO(e);
	SET_ZERO(f);
	SET_ZERO(h);

	mulu64((du32_t*)&(a->d32.u0), (du32_t*)&(b->d32.u0), &(d));
	mulu64((du32_t*)&(a->d32.u1), (du32_t*)&(b->d32.u0), &(e));
	mulu64((du32_t*)&(a->d32.u0), (du32_t*)&( b->d32.u1), &(f));
	mulu64((du32_t*)&(a->d32.u1 ), (du32_t*)&(b->d32.u1), &(h));


	c.d32.u0 = d.d32.u0;

	c.d32.u1 = d.d32.u1 + e.d32.u0 + f.d32.u0;
	if(d.d32.u1 + e.d32.u0 < d.d32.u1 || d.d32.u1 + e.d32.u0 + f.d32.u0 < d.d32.u1 + e.d32.u0)
		carry = 1;
	else
		carry = 0;

	c.d32.u2 = e.d32.u1 + f.d32.u1 + h.d32.u0 + carry;
	if( e.d32.u1 + f.d32.u1 < e.d32.u1 || e.d32.u1 + f.d32.u1 + h.d32.u0 < e.d32.u1 + f.d32.u1 || e.d32.u1 + f.d32.u1 + h.d32.u0 + carry < e.d32.u1 + f.d32.u1 + h.d32.u0)
		carry = 1;
	else
		carry = 0;

	c.d32.u3 = h.d32.u1 + carry;

	memcpy(out, &c, sizeof(du128_t));

	return 0;
}

void du64_plus_u32(du64_t *a, u32_t n, du64_t *out)
{
	if(a->d32.u0 + n < a->d32.u0){
		a->d32.u1 += 1;
	}
	a->d32.u0 += n;
}


/*
//64bit test
int main1()
{
	du32_t a, b;
    du64_t c;

    SET_ZERO(a);
    SET_ZERO(b);
    SET_ZERO(c);


	a.d16.u0 = 65535;
	a.d16.u1 = 65535;

	b.d16.u0 = 10;
	b.d16.u1 = 10;

	mulu64(&a, &b, &c);

	printf("0x%04x 0x%04x 0x%04x 0x%04x", c.d16.u0, c.d16.u1, c.d16.u2, c.d16.u3);

	system("pause");

    return 0;
}

//128bit test
int main()
{
	du64_t a, b;
    du128_t c;

    SET_ZERO(a);
    SET_ZERO(b);
    SET_ZERO(c);


	a.d32.u0 = 0xFFFFFFFF;
	a.d32.u1 = 0x0FFFFFFF;

	b.d32.u0 = 10;
	b.d32.u1 = 0;

	mulu128(&a, &b, &c);

	printf("0x%08x 0x%08x 0x%08x 0x%08x", c.d32.u0, c.d32.u1, c.d32.u2, c.d32.u3);

	system("pause");

    return 0;
}
*/


//参见 \tools\mul128
//用于读取一个10进制串到u64结构中，输入数字长度小于19
void arch_ato64u(const char* c, du64_t *out)
{
	du128_t dig;
	const char *org = c;

	SET_ZERO(dig);

	while(arch_isdigit((int)*c) && (c-org < 19)){	//如果确定是bit64 那么10进制数不可能大过19字符

		du64_t dig10;
		SET_ZERO(dig10);
		dig10.d32.u0 = 10;	//乘数10也要做成u64

		mulu128((du64_t*)&dig, &dig10, &dig);

		du64_plus_u32((du64_t*)&dig, (*c - '0'), (du64_t*)&dig);

		c++;
	}

	memcpy(out, &dig, sizeof(du64_t));
}




//转换为无符号数
//n表示读取数字字符的最大长度
u64_t arch_atou64n(const char* c, size_t n)
{
	u64_t dig = 0;
	const char *org = c;
	while(arch_isdigit((int)*c) && (c-org < n) ){
		dig = dig*10 + *c - '0';
		c++;
	}
	return dig;
}

s64_t arch_atos64n(const char* c, size_t n)
{
    if (*c == '-')
        return 0 - (s64_t) arch_atou64n(c + 1, n - 1);
    else
        return (s64_t) arch_atou64n(c, n);
}







const int id_to_idx(const id_str_pair_t* array, size_t array_len, int id)
{
	int i;
	for(i = 0; i < array_len; i++)
		if(array[i].id == id)return i;
	return STATE_NOTFOUND;
}

const int str_to_idx(const id_str_pair_t* array, size_t array_len, const char*str)
{
	int i;
	for(i = 0; i < array_len; i++)
		if(0 == strcmp(array[i].str, str))return i;
	return STATE_NOTFOUND;
}

//可能有找不到 缺省值为 [0]
int str_to_id(const id_str_pair_t* array, size_t array_len, const char*str)
{
	int idx = str_to_idx(array, array_len, str);
	if(idx >=0)
		return array[idx].id;
	else
		return array[0].id;
}

//可能有找不到 缺省值为 [0]
const char* id_to_str(const id_str_pair_t* array, size_t array_len, int id)
{
	int idx = id_to_idx(array, array_len, id);
	if(idx >=0)
		return array[idx].str;
	else
		return array[0].str;
}

//改进函数如下
//比较双方是 str_mem 和 str
// str_mem 同时使用'\0'结束字串 和 str_mem_len 长度约束 两个条件来确定长度
//若'\0'结束符在 str_mem_len 长度约束之内，本函数和 strcmp类似
//若 str_mem 不是以'\0'结束符结尾，那么
// lstr = length string
 int memstrcmp(const char *str_mem, size_t str_mem_len, const char *str)
{
	if (str_mem_len == 0)
		return 0;

	while (*str_mem == *str) {
		if (*str_mem == '\0')
			break;
		str_mem++;
		str++; str_mem_len--;

		if (str_mem_len == 0){		//达到长度
		
			if(*str == '\0')	//只有对方也达到尾部 才表示相等
				return 0;
			else
				break;
		}
	}

	return *str_mem - *str;
}


