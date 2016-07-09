


#ifndef __D0_OTD_H__
#define __D0_OTD_H__


#include "delayzero.h"

#include "d0_watcher_otu.h"
#include "d0_watcher_ott.h"
#include "d0_watcher_httpc.h"

#include "ot_def.h"



#ifndef OTN_XOBJ_NAME_DEFAULT
#define OTN_XOBJ_NAME_DEFAULT "otn"	//ot new
#endif






#define OTN_NETIF_CHANGE()	otn_event_emit(OT_EVENT_NETIF_CHANGE, NULL)


#define OTN_ONLINE(fp) otn_event_on(OT_EVENT_OT_ONLINE, (fp))
#define OTN_OFFLINE(fp) otn_event_on(OT_EVENT_OT_OFFLINE, (fp))





struct otn{
	struct d0_loop loop;
	udp_ot_t* otu;	//udp watcher
	tcpc_ot_t* ott;		//tcp watcher
	tcpc_httpc_t* httpc;
};


int otn(u16_t local_uport, 
		const char* udomain, 
		u16_t cloud_uport, 
		char enauth,		//If we enable token publish

		const char* tdomain, 
		u16_t cloud_tport, 

		const u8_t* did_hex, 
		const u8_t* key_hex, 
		const u8_t token[]);

int otn_event_emit(const char*evt, void* ctx);
int otn_event_on(const char*evt, fp_listener_op handler);

int otu_stat(char*buf, int size, char* post);
int ott_stat(char*buf, int size, char* post);


#define ot_api_method(js, js_len, ack, ctx)  ot_api_method_with_did(0, (js), (js_len), (ack), (ctx))

int ot_api_method_with_did(u64_t did64, const char* js, size_t js_len, fp_json_delegate_ack ack, void* ctx);
const char* otn_is_online(void);


int ot_delegate_ctx_query(void* ctx, char* proto, char* is_local, char* from, size_t from_size, size_t *from_len);
int ot_httpc_start(const char* url, const u8_t md5[], const stream_in_if_t* in_if, stream_ctx_t* ps_ctx);


int json_delegate_ack_err(fp_json_delegate_ack ack, void* ctx, int err_code, const char* err_info);
int json_delegate_ack_result(fp_json_delegate_ack ack, void* ctx, int ret_code);






#endif /* __D0_OTD_H__ */

