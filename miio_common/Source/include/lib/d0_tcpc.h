


#ifndef __D0_TCPC_H__
#define __D0_TCPC_H__
#include "delayzero.h"

#include "arch_net.h"


typedef enum tcpc_msg_type{
	MSG_TYPE_TCPC_CONN_OK,
	MSG_TYPE_TCPC_CONN_ERR,
	MSG_TYPE_TCPC_DISCONN,
	MSG_TYPE_TCPC_CLOSE_CONN,
	MSG_TYPE_TCPC_RCV,
	MSG_TYPE_TCPC_MAX
}tcpc_msg_type_t;




typedef enum {
	D0_TCPC_STATE_IDLE,
	D0_TCPC_STATE_CONNECTING,
	D0_TCPC_STATE_CONNECTED
}tcpc_state_t;


struct d0_tcpc;

typedef int(*fp_d0_tcpc_op)(struct d0_tcpc*);


typedef struct tcpc_if{
	fp_d0_tcpc_op enter_hook;
	fp_d0_tcpc_op exit_hook;
	fp_d0_tcpc_op connect_ok_hook;
	fp_d0_tcpc_op connect_err_hook;
	fp_d0_tcpc_op disconnect_hook;
	fp_d0_tcpc_op recv_hook;
}tcpc_if_t;



struct d0_tcpc{
	struct d0_watcher w;
	tcpc_state_t state;
	net_conn_t* base_conn;
	u16_t port;
	const tcpc_if_t *tcpc_if;
};


struct invoke_ext_req_conn{
	u32_t peer_ip;
	u16_t port;
};


int d0_tcpc_req_close(struct d0_tcpc* tcpc);
int d0_tcpc_req_conn(struct d0_tcpc* tcpc, u32_t peer_ip, u16_t port);
int d0_tcpc_info_preset(struct d0_tcpc* tcpc, u16_t port,const tcpc_if_t* tcpcif);
int d0_tcpc_send(struct d0_tcpc* tcpc, const void*buf, int len);
int d0_tcpc_recv(struct d0_tcpc* tcpc, net_pkt_t** pkt);
tcpc_state_t d0_tcpc_state(struct d0_tcpc* tcpc);
int d0_tcpc_close_sync(struct d0_tcpc* tcpc);
int d0_tcpc_conn_sync(struct d0_tcpc* tcpc, u32_t ip, u16_t port);



#endif /* __D0_TCPC_H__ */



