/**
 * @file			addif_net_export.h
 * @brief			Network APIs
 * @author			dy
*/

#ifndef _ADDIF_NET_EXPORT_H_
#define _ADDIF_NET_EXPORT_H_

typedef void net_pkt_t;
typedef void net_conn_t;

typedef struct net_addr {
	uint32_t s_addr;
}net_addr_t;

#define api_net_ntoa   arch_net_ntoa
#define api_net_ntoa_buf   arch_net_ntoa_buf
#define api_net_name2ip   arch_net_name2ip

#define api_net_conn_new   arch_net_conn_new
#define api_net_connect   arch_net_connect
#define api_net_conn_getaddr   arch_net_conn_getaddr
#define api_net_conn_recv   arch_net_conn_recv
#define api_net_conn_write   arch_net_conn_write
#define api_net_conn_sendto   arch_net_conn_sendto
#define api_net_conn_send   arch_net_conn_send

#define api_net_pkt_new   arch_net_pkt_new
#define api_net_pkt_alloc   arch_net_pkt_alloc
#define api_net_pkt_free   arch_net_pkt_free

#define api_net_pkt_faddr   arch_net_pkt_faddr

#define api_net_pkt_fport   arch_net_pkt_fport
#define api_net_pkt_total_len   arch_net_pkt_total_len
#define api_net_pkt_data   arch_net_pkt_data
#define api_net_pkt_next   arch_net_pkt_next


#define api_net_ip2n   arch_net_ip2n

#endif
