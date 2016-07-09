
#ifndef __D0_WATCHER_OTT__
#define __D0_WATCHER_OTT__




#include "delayzero.h"

#include "d0_tcpc.h"


#include "lib_addon.h"
#include "jsmn/jsmn_api.h"

#include "ot_def.h"



//内部发生事件 ott连接不力
#define OTT_EVENT_DESOLATE		"ott.desolate"

#define OTT_STATE_TOUCHING	3
#define OTT_STATE_INTOUCH	4



typedef struct ott_session{
	struct d0_session d0;

	//事务的ID
	u32_t method_id;


	fp_json_delegate_ack ack_cb;
	void* ack_ctx;

	u16_t timeout;
}ott_session_t;



typedef struct tcpc_ot{
	struct d0_tcpc tcpc;

	u8_t buf[OT_PACKET_SIZE_MAX];
	u16_t buf_len;

	u8_t OT_DID[OT_PROTO_DEVICE_ID_SIZE];
	u8_t OT_KEY_pub[OT_PROTO_DEVICE_KEY_SIZE];
	u8_t OT_KEY_priv[OT_PROTO_DEVICE_KEY_SIZE];
	u8_t OT_IV[OT_PROTO_DEVICE_KEY_SIZE];

	struct {
		net_addr_t ip;
		u16_t port;
	}backlist[8];		//云端host备份列表
	u8_t backlist_idx;
	u8_t backlist_num;

	u8_t token[OT_PROTO_DEVICE_KEY_SIZE];	//设备控制令牌

	char host_domain_preset[OT_HOST_DOMAIN_SIZE_MAX];
	u16_t host_port_preset;

	net_addr_t host_ip;
	u16_t host_port;

	u32_t kplv_interval;

	int sync_interval;	//和服务器同步时间间隔

	//中立统计数据，请不要使用统计数据 作为底层判断逻辑
	struct{
		u32_t conn_ok;	//statistic: connect time count
		u32_t conn_err;	//statistic: connect err count
		u32_t resolve_3;	//statistic: last 3 "resolve"s avr time.
		u32_t conn_time_3;	//statistic: last 3 "connect establish time" avr val.
	} statistic;

	u32_t start_conn_ts;
	char ot_state;
}tcpc_ot_t;



typedef struct {
	down_ctx_t dn_ctx;
	u32_t method_id;
	size_t js_len;
	char *js;
}ott_down_method_ext_t;




int ott_is_online(tcpc_ot_t* ott);
void ott_info_preset(tcpc_ot_t* ott, const u8_t* did_hex, const u8_t* key_hex, const char* domain, u16_t ott_port,const u8_t token[]);
int ott_method(tcpc_ot_t* ott, const char* js, size_t js_len, fp_json_delegate_ack ack, void* ctx);
int ott_when_netif_change(tcpc_ot_t* ott);
int ott_statistic(tcpc_ot_t* ott, char*buf, int size, char* post);
void connect_restart(tcpc_ot_t* ott);

#endif /* __D0_WATCHER_OTT__ */


