
#ifndef __D0_UDP_H__
#define __D0_UDP_H__
#include "delayzero.h"
#include "arch_net.h"


struct d0_udp;

typedef int(*fp_d0_udp_op)(struct d0_udp*);

typedef struct d0_udp_if{
	fp_d0_udp_op enter_hook;
	fp_d0_udp_op exit_hook;
	fp_d0_udp_op recv_hook;
}d0_udp_if_t;





struct d0_udp{
	struct d0_watcher w;
	u16_t port;
	net_conn_t* base_conn;
	const d0_udp_if_t* udp_if;
};




int d0_udp_info_preset(struct d0_udp *udp, u16_t port, const d0_udp_if_t * udpif);
int d0_udp_buf_send(struct d0_udp *udp,void* buf, int len, net_addr_t* ip, u16_t port);
int d0_udp_pkt_send(struct d0_udp *udp, net_pkt_t* pkt, net_addr_t* ip, u16_t port);
int d0_udp_recv(struct d0_udp *udp, net_pkt_t** pkt);



#endif /* __D0_UDP_H__ */
