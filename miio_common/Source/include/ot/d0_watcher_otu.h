#ifndef __D0_WATCHER_OTU_H__
#define __D0_WATCHER_OTU_H__


#include "delayzero.h"
#include "d0_udp.h"

#include "lib_addon.h"
#include "jsmn/jsmn_api.h"

#include "ot_def.h"


#ifndef OTU_NEIGHBOR_SYNC_INTERVAL
#define OTU_NEIGHBOR_SYNC_INTERVAL	30000	// 60s neighbor probe interval
#endif

#ifndef NEIGHBOR_NUM_MAX
#define NEIGHBOR_NUM_MAX	32
#endif


typedef struct {
		u8_t did[OT_PROTO_DEVICE_ID_SIZE];
		net_addr_t ip;
		u32_t last_ack;
		s32_t ts_offset;
		u8_t token_pub[OT_PROTO_DEVICE_KEY_SIZE];
		u8_t token_priv[OT_PROTO_DEVICE_KEY_SIZE];
		u8_t token_iv[OT_PROTO_DEVICE_KEY_SIZE];
}neighbor_cache_t;




#ifndef OTU_SYNC_RESTART_INTERVAL
#define OTU_SYNC_RESTART_INTERVAL 2000		// 2s 失联同步时间报文 间隔时间
#endif

//#ifndef OTU_KPLV_INIT_INTERVAL
//#define OTU_KPLV_INIT_INTERVAL 1000				// 1s 第一次连接初期保活报文 间隔时间
//#endif

#ifndef OTU_UP_INFO_RETRY_INTERVAL
#define OTU_UP_INFO_RETRY_INTERVAL	3000	// 3s info报文重发间隔时间
#endif



//该Bit map 用于设置和读取当前状态
#define OTU_FLAG_ENAUTH	0x0001	//该标记设置 将使能允许局域网内Token获取
//#define OTU_FLAG_ONLINE	0x0002	//该标记 对外指示 是否正常连接服务器
#define OTU_FLAG_LOCAL_ONLY	0x0004	//该标记设置 是否独立运行(不主动连接服务器)
#define OTU_FLAG_DNS_TOUCHED	0x0008	//该标记表示 DNS过程是否成功过
#define OTU_FLAG_SVR_TOUCHED	0x0010	//该标记表示 连接过服务器是否成功过
//#define OTU_FLAG_SYNC_TOUCHED	0x0020	//该标记表示 时间戳同步是否成功过



//OT状态机
#define OTU_STATE_IDLE	'I'
#define OTU_STATE_PASSIVE	'P'		//表示现在处在被动状态
#define OTU_STATE_TOUCHING	'C'	//connecting
#define OTU_STATE_INTOUCH	'T'		//touch




typedef struct {		//
	down_ctx_t dn_ctx;
	u8_t did_from[OT_PROTO_DEVICE_ID_SIZE];
	u32_t method_id;
	net_addr_t peer_ip;
	u16_t peer_port;
	size_t js_len;
	char *js;
}otu_down_method_ext_t;


typedef enum{
	SESS_UP,
	SESS_DOWN,
	SESS_BOTH
}session_type_t;

typedef struct otu_session{
	struct d0_session d0;
	session_type_t type;
	u32_t ts;	//发送时的时间戳计数
	u32_t method_id;	//事务的ID
	u16_t pkt_buf_len;	//会缓存 packet
	u8_t *pkt_buf;

	net_addr_t peer_ip;
	u16_t peer_port;

	union{
		struct{
			fp_json_delegate_ack ack_cb;
			void* ack_ctx;
			u8_t retry_left;
			u8_t retry_times;
			u16_t timeout;
		}up_sess;
		struct{
//			net_addr_t peer_ip;
//			u16_t peer_port;
		}dn_sess;
	}d;

}otu_session_t;



typedef struct udp_ot{
	struct d0_udp udp;
	u8_t OT_DID[OT_PROTO_DEVICE_ID_SIZE];
	u8_t OT_KEY_pub[OT_PROTO_DEVICE_KEY_SIZE];
	u8_t OT_KEY_priv[OT_PROTO_DEVICE_KEY_SIZE];
	u8_t OT_IV[OT_PROTO_DEVICE_KEY_SIZE];

	char host_domain_preset[OT_HOST_DOMAIN_SIZE_MAX];
	u16_t host_port_preset;

	net_addr_t host_ip;
	u16_t host_port;


	net_addr_t last_ip;
	u16_t last_port;

	u32_t kplv_interval;	//定时上报的时间间隔

	//用于分辨局域网访问
	u32_t local_if_mask;	//掩码
	u32_t local_if_netid;		//网络地址

	u32_t uap_if_mask;	//掩码
	u32_t uap_if_netid;		//网络地址

	struct {
		net_addr_t ip;
		u16_t port;
	}backlist[4];		//云端host备份列表
	u8_t backlist_idx;	//
	u8_t backlist_num;	//



//	char ot_log[OT_LOG_BUF_SIZE_MAX];	//用于缓存所有命令
//	size_t ot_log_len;


	u16_t ot_flag;	//bit map型 标识
	char ot_state;	//枚举型 运行状态

	int sync_interval;	//和服务器同步时间间隔

	//中立统计数据，请不要使用统计数据 作为底层判断逻辑
	struct{
		u32_t req_ack_10;	//statistic: last 10 "one-time-req-ack"s avr time. avr'' = (avr'x 9 + new)/10
		u32_t req_ack_100;	//statistic: last 100 "one-time-req-ack"s avr time. avr'' = (avr'x 99 + new)/100
		u32_t req;		//statistic: request with session.
		u32_t req_fail;
		u32_t req_ok;	//statistic: request success with one try.

		u32_t resolve_3;	//statistic: last 3 "resolve"s avr time.

	} statistic;

	neighbor_cache_t *neighbor;	//first slot is its self ,did token stuff
	u8_t neighbor_num;

	u16_t last_err_idx;
	char last_err[64];
}udp_ot_t;




typedef enum{
	OTU_HAPPEN_KEEPALIVE_TIMEOUT,
	OTU_HAPPEN_SET_PASSIVE,	//这个事件表示 被设置为passive
	OTU_HAPPEN_NETIF_CHANGE,
	OTU_HAPPEN_LOGIN_OK,
}otu_state_src_t;




//内部发生事件 otu无法连接服务器
#define OTU_EVENT_DESOLATE		"otu.desolate"




int otu_cloud_active(udp_ot_t* otu);

int otu_is_online(udp_ot_t* otu);
int otu_has_neighbor(udp_ot_t* otu);
void otu_info_preset(udp_ot_t* otu, const u8_t* did_hex, const u8_t* key_hex, u16_t local_port, const char* domain, u16_t ot_port, const u8_t token[], char enauth);
int otu_method(udp_ot_t* otu, u64_t did64, const char* js, size_t js_len, fp_json_delegate_ack ack, void* ctx);
int otu_when_netif_change(udp_ot_t* otu);
int otu_statistic(udp_ot_t* otu, char*buf, int size, char* post);



#endif /* __D0_WATCHER_OTU_H__ */


