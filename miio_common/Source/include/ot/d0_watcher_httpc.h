
#ifndef __D0_WATCHER_HTTPC__
#define __D0_WATCHER_HTTPC__

#include "delayzero.h"
#include "d0_tcpc.h"
#include "ot_def.h"

#define DNLD_ERR_NUM_MAX		50//by zengfan
#define HTTP_HEAD_SIZE	1500

typedef void stream_ctx_t;

typedef int (*fp_stream_open)(stream_ctx_t* ctx, void* id);		//return negative means error
typedef int (*fp_stream_isopen)(stream_ctx_t* ctx);
typedef int (*fp_stream_read)(stream_ctx_t* ctx, u8_t* out, size_t len);	//return byte num, negative means error
typedef int (*fp_stream_write)(stream_ctx_t* ctx, u8_t* in, size_t len);	//return negative means error
typedef int (*fp_stream_close)(stream_ctx_t* ctx);
typedef int (*fp_stream_error)(stream_ctx_t* ctx, void* ret);





typedef struct{
	//在DNS完成，连接建立，头部正确，且拿到数据时被调用。
	//返回 < 0，将断开连接恢复IDLE状态；其不会其他回调，使用者自己发现问题自己现场解决；
	fp_stream_open open;

	//在连接建立，头部正确，且拿到数据时，调用open之后被调用。
	//返回 < 0，将认定放弃下载，断开连接恢复IDLE状态；其不会其他回调，使用者自己发现问题自己现场解决；
	fp_stream_write write;

	//在下载完成时被调用，之后断开连接恢复IDLE状态；
	fp_stream_close close;

	//在调用open之前(DNS完成，建立连接，检查头部，拿到数据)任何的错误都会导致error，将放弃下载，断开连接恢复IDLE状态
	fp_stream_error error;
}stream_in_if_t;



typedef int (*fp_block_open)(stream_ctx_t* ctx, void* id);
typedef int (*fp_block_read)(stream_ctx_t* ctx, u8_t* out, size_t out_size);
typedef int (*fp_block_close)(stream_ctx_t* ctx);
typedef int (*fp_block_error)(stream_ctx_t* ctx, void* ret);



typedef struct{

	//在DNS完成，连接建立时被调用。
	//必须返回块尺寸，用于http 头部 content-length；
	//返回 <= 0，将断开连接恢复IDLE状态；其不会其他回调，使用者自己发现问题自己现场解决；
	fp_block_open open;

	//在头部发送完成后调用。
	//返回 读取长度
	//返回 <= 0，将认定放弃上传，断开连接恢复IDLE状态；其不会其他回调，使用者自己发现问题自己现场解决；
	fp_block_read read;

	//在上传完成时被调用，之后断开连接恢复IDLE状态；
	fp_block_close close;

	fp_block_error error;
}block_out_if_t;





typedef struct{
	struct d0_tcpc tcpc;

//	url: http://xxxx.com/download/xxx/xxx/xx.onion -> xxxx.com\0___/download/xxx/xxx/xx.onion
	char* url;
//	url_path:  /download/xxx/xxx/xx.onion
	char* url_path;
	u16_t url_len;
	u16_t port;
	net_addr_t peer_ip;

	//处理接口
	stream_ctx_t* up_dn_ctx;
	const stream_in_if_t* dnld_if;
	const block_out_if_t* upld_if;

	//运行内部参数
	int content_len;
	size_t body_processed;
	u8_t md5[16];


//	int err_cnt;	//故障计数器
	char last_msg[32];
	int done_flag;	// 0: Initial val;  1: Done ok;  -1: Error occur
//head,by zengfan
	u8_t* phead;
	u16_t head_len;
}tcpc_httpc_t;



typedef struct{
	u16_t port;
	u8_t trying;
	u8_t padd;

}httpc_resolve_tmr_ext_t;



#define httpc_start_download(h, url, inif, m5, ctx)  httpc_start((h), (url), (inif), (m5), NULL, (ctx))
#define httpc_start_upload(h, url, outif, ctx)  httpc_start((h), (url), NULL, NULL, (outif), (ctx))

int httpc_start(tcpc_httpc_t *httpc, const char* url, const stream_in_if_t* in_if, const u8_t in_md5[], const block_out_if_t* out_if, stream_ctx_t* up_dn_ctx);
void httpc_info_preset(tcpc_httpc_t* httpc);




#endif /* __D0_WATCHER_HTTPC__ */



