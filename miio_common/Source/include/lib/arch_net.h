
#ifndef __ARCH_NET_H__
#define __ARCH_NET_H__

#include "lwip/def.h"
#include "lwip/api.h"
#include "lwip/ip.h"
#include "lwip/tcp.h"
#include "lwip/udp.h"

#include "lib/lib_generic.h"
#include "lib/addif_net_export.h"


#define MAKE_IPV4(a, b, c, d)                   LONGVAL_LE(a, b, c, d)



u32_t arch_net_htonl(u32_t h);
u32_t arch_net_ntohl(u32_t n);

u16_t arch_net_htons(u16_t h);
u16_t arch_net_ntohs(u16_t n);

char *arch_net_ntoa(net_addr_t* paddr);
int arch_net_ntoa_buf(net_addr_t* paddr, char* buf);
int arch_net_name2ip(const char *name, net_addr_t *addr);
net_conn_t* arch_net_conn_new(u32_t type);
int arch_net_connect(net_conn_t* conn, net_addr_t *addr, u16_t port);
int arch_net_conn_getaddr(net_conn_t* conn, net_addr_t *addr, u16_t *port, u8_t local);
int arch_net_conn_recv(net_conn_t* conn, net_pkt_t** pkt);
int arch_net_conn_write(net_conn_t* conn, const void*buf, size_t len);
int arch_net_conn_sendto(net_conn_t* conn, net_pkt_t* pkt, net_addr_t*peer_ip, u16_t peer_port);
int arch_net_conn_send(net_conn_t* conn, net_pkt_t* pkt);

net_pkt_t* arch_net_pkt_new(void);
u8_t *arch_net_pkt_alloc(net_pkt_t* pkt, size_t size);
void arch_net_pkt_free(net_pkt_t* pkt);
net_addr_t* arch_net_pkt_faddr(net_pkt_t* pkt);
u16_t arch_net_pkt_fport(net_pkt_t* pkt);
u16_t arch_net_pkt_total_len(net_pkt_t* pkt);
int arch_net_pkt_data(net_pkt_t* pkt,u8_t** ptr, u16_t *len);
int arch_net_pkt_next(net_pkt_t* pkt);


u32_t arch_net_ip2n(const char* ip);

void arch_net_hton64_buf(u64_t d64, u8_t *p);
u64_t arch_net_ntoh64_buf(const u8_t *p);

#endif /* __ARCH_NET_H__ */

