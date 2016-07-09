


#ifndef __OT_H__
#define __OT_H__


#include "ot_cfg.h"

#ifndef OT_PACKED
#if defined(__GNUC__)     /* GNU GCC Compiler */
#define OT_PACKED __attribute__ ((__packed__))
#elif defined (__ICCARM__)
#define OT_PACKED
#endif
#endif
////////////////////////////////////////////
//json rpc error code:	-32767 < x < -32000
////////////////////////////////////////////

#define OT_ERROR_CODE_SYS_MIN	(-32767)
#define OT_ERROR_CODE_SYS_MAX	(-30000)

#define OT_ERR_INFO_PARSE "Invalid json."
#define OT_ERR_CODE_PARSE	(-32700)			//	Parse error	Invalid JSON was received by the server.


#define OT_ERR_INFO_REQ_INVALID	"Req object invalid."
#define OT_ERR_CODE_REQ_INVALID	(-32600)		//	Invalid Request	The JSON sent is not a valid Request object.


#define OT_ERR_INFO_METHOD_INVALID "Method not found."
#define OT_ERR_CODE_METHOD_INVALID	(-32601)	//	Method not found	The method does not exist / is not available.


#define OT_ERR_INFO_PARAM_INVALID "Invalid param."
#define OT_ERR_CODE_PARAM_INVALID	(-32602)	//	Invalid params	Invalid method parameter(s).

#define OT_ERROR_INFO_CONFIG_ROUTER "Config router fail"
#define OT_ERROR_CODE_CONFIG_ROUTER (-32603)

#define OT_ERROR_INFO_ACK_CODE_INVALID "Invalid ack code"	//call json_delegate_ack_result() with wrong ack_code
#define OT_ERROR_CODE_ACK_INVALID 	(-32604)


#define OT_ERROR_INFO_TOO_MANY_OBJ "Too many obj"	//too many object
#define OT_ERROR_CODE_TOO_MANY 	(-32605)


///error code and message about xset 
#define OT_ERR_INFO_WRITE_PSM "write psm fail"
#define OT_ERR_CODE_WRITE_PSM (-32699)

#define OT_ERR_INFO_ERASE_PSM "erase psm fail"
#define OT_ERR_CODE_ERASE_PSM (-32698)

#define OT_ERR_INFO_READ_PSM "read psm fail"
#define OT_ERR_CODE_READ_PSM (-32697)

#define OT_ERR_INFO_OPEN_PSM "open psm fail"
#define OT_ERR_CODE_OPEN_PSM (-32696)

#define OT_ERR_INFO_CLOSE_PSM "close psm fail"
#define OT_ERR_CODE_CLOSE_PSM (-32695)

#define OT_ERR_INFO_TOO_MANY_TIMER "too many timer"
#define OT_ERR_CODE_TOO_MANY_TIMER (-32694)
////////////////////////////////////////////
//otd sys error code:	-32000 < x < -30000
////////////////////////////////////////////


#define OT_ERR_INFO_RESP_ERROR "Resp general error."
#define OT_ERR_CODE_RESP_ERROR	(-30000)


#define OT_ERR_INFO_RESP_PARSE "Resp invalid json."
#define OT_ERR_CODE_RESP_PARSE	(-30001)

//3
#define OT_ERR_INFO_REQ_ERROR "Req error."
#define OT_ERR_CODE_REQ_ERROR	(-30010)

//3
#define OT_ERR_INFO_TRYOUT "Try out. "
#define OT_ERR_CODE_TRYOUT	(-30011)


#define OT_ERR_INFO_BUSY "Busy. "
#define OT_ERR_CODE_BUSY	(-30012)


#define OT_ERR_INFO_OFFLINE "Offline."
#define OT_ERR_CODE_OFFLINE	(-30013)


#define OT_ERR_INFO_NOT_SYNC ((const char*)"not sync.")
#define OT_ERR_CODE_NOT_SYNC (-30014)


#define OT_ERR_INFO_SERVICE_NOT_AVAILABLE ((const char*)"OTd not available.")
#define OT_ERR_CODE_SERVICE_NOT_AVAILABLE (-30020)


#define OT_ERR_INFO_REDIRECT_FAILED ((const char*)"redirect failed.")
#define OT_ERR_CODE_REDIRECT_FAILED (-30030)





////////////////////////////////////////////
//user error code:  -10000 <= x <= -5000
////////////////////////////////////////////
#define OT_ERR_CODE_USR_MIN -10000
#define OT_ERR_CODE_USR_MAX -5000

// UART-1 down command user defined error
#define OT_ERR_INFO_UART_UNDEF_ERROR "UART undefined error"
#define OT_ERR_CODE_UART_UNDEF_ERROR (OT_ERR_CODE_USR_MIN)

// UART-1 down command timeout
#define OT_ERR_INFO_UART_TIMEOUT "UART timeout"
#define OT_ERR_CODE_UART_TIMEOUT (OT_ERR_CODE_USR_MIN + 1)





#ifndef OT_DLD_URL_SIZE_MAX
#define OT_DLD_URL_SIZE_MAX 512
#endif



#ifndef OT_HOST_DOMAIN_SIZE_MAX
#define OT_HOST_DOMAIN_SIZE_MAX 64
#endif

#ifndef OT_PACKET_SIZE_MAX
#define OT_PACKET_SIZE_MAX 1000
#endif

#ifndef OT_METHOD_TIMEOUT_DEFAULT
#define OT_METHOD_TIMEOUT_DEFAULT 2000
#endif

#ifndef OT_METHOD_TIMEOUT_MAX
#define OT_METHOD_TIMEOUT_MAX 60000
#endif

#ifndef OT_METHOD_TRY_DEFAULT
#define OT_METHOD_TRY_DEFAULT 3
#endif

#ifndef OT_METHOD_TRY_MAX
#define OT_METHOD_TRY_MAX 10
#endif

#ifndef OT_UP_CONCURRENT_SESSION_NUM_MAX
#define OT_UP_CONCURRENT_SESSION_NUM_MAX 5
#endif

#ifndef OT_DN_CONCURRENT_SESSION_NUM_MAX
#define OT_DN_CONCURRENT_SESSION_NUM_MAX 6
#endif

#ifndef OT_DN_SESSION_WAIT_TIME
#define OT_DN_SESSION_WAIT_TIME 2000
#endif

#ifndef OT_UP_METHOD_JSMN_TOKEN_NUM_MAX
#define OT_UP_METHOD_JSMN_TOKEN_NUM_MAX 400
#endif


#ifndef OT_UP_INFO_INTERVAL
#define OT_UP_INFO_INTERVAL	1800000
#endif

#ifndef OT_KPLV_TRYTIMES
#define OT_KPLV_TRYTIMES	3
#endif

#ifndef OT_KPLV_INTERVAL
#define OT_KPLV_INTERVAL 15000
#endif

#ifndef OT_KPLV_TIMEOUT_INTERVAL
#define OT_KPLV_TIMEOUT_INTERVAL (OT_KPLV_INTERVAL*OT_KPLV_TRYTIMES)
#endif


#ifndef OT_OUT_OF_SYNC_SECOND_MAX
#define OT_OUT_OF_SYNC_SECOND_MAX   60
#endif


#ifndef OT_SYNC_INIT_INTERVAL
#define OT_SYNC_INIT_INTERVAL 4000
#endif


#ifndef OT_GATEWAY_SID_SIZE_MAX
#define OT_GATEWAY_SID_SIZE_MAX 64
#endif


/*
	Event
*/
#define OT_EVENT_NETIF_CHANGE	"ot.netif_change"

#define OT_EVENT_OT_OFFLINE		"ot.offline"

#define OT_EVENT_OT_ONLINE		"ot.online"


#ifndef OT_UPLINK_ERR_NUM_MAX
#define OT_UPLINK_ERR_NUM_MAX 3
#endif



#define OT_PROTO_VER_V3A 0x3121
#define OT_PROTO_VER_V3U 0x3021

#define  OT_PROTO_DEVICE_ID_SIZE 8
#define OT_PROTO_DEVICE_KEY_SIZE MD5_SIGNATURE_SIZE

#ifndef OT_LOG_BUF_SIZE_MAX
#define OT_LOG_BUF_SIZE_MAX 800
#endif


#ifndef OT_SYNC_TIME_MAX
#define OT_SYNC_TIME_MAX	1800000
#endif

#ifndef OT_SYNC_TIME_MIN
#define OT_SYNC_TIME_MIN	60000
#endif




#if defined(__GNUC__)     /* GNU GCC Compiler */

struct  ot_pkt_hdr{
	u16_t proto_ver;
	u16_t len;
	u8_t DID[OT_PROTO_DEVICE_ID_SIZE];
	u32_t ts;
	u8_t sign[MD5_SIGNATURE_SIZE];
} __attribute__ ((__packed__));
typedef struct ot_pkt_hdr ot_pkt_hdr_t;

#elif defined ( __ICCARM__ )
struct  ot_pkt_hdr{
	u16_t proto_ver;
	u16_t len;
	u8_t DID[OT_PROTO_DEVICE_ID_SIZE];
	u32_t ts;
	u8_t sign[MD5_SIGNATURE_SIZE];
} OT_PACKED;
typedef struct ot_pkt_hdr ot_pkt_hdr_t;

#endif



typedef struct {
	u8_t did_to[OT_PROTO_DEVICE_ID_SIZE];
	fp_json_delegate_ack ack;
	void* ctx;
	size_t js_len;	//json
	char js[0];
}ot_invoke_up_req_ext_t;




typedef struct{
	char host[OT_HOST_DOMAIN_SIZE_MAX];
	u16_t port;
	u8_t trying;
	u8_t padd;
}ot_resolve_tmr_ext_t;



#ifndef NEIGHBOR_CACHE_NUM
#define NEIGHBOR_CACHE_NUM 8
#endif


//{method:xx, param:xx, from:xxx}  "from" max size
#ifndef OT_REQ_FROM_SIZE_MAX
#define OT_REQ_FROM_SIZE_MAX 32
#endif


typedef struct{
	u32_t magic;
	char from[OT_REQ_FROM_SIZE_MAX];
	u8_t proto;
	u8_t is_local;
}down_ctx_t;	//used for distingish down req ctx


#define OT_DELEGATE_CTX_MAGIC 0x82562647

#define OT_DWON_CTX_TYPE_TCP 't'
#define OT_DWON_CTX_TYPE_UDP 'u'
#define OT_DWON_CTX_IS_LOCAL 'l'
#define OT_DWON_CTX_NOT_LOCAL 'c'





#endif /* __OT_H__ */






