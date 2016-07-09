
#include <util/util.h>
#include "lib/arch_net.h"
#include "netif/etharp.h"

static char *net_addr_ntoa(u32_t addr)
{
  static char str[16];
  u32_t s_addr = addr;
  char inv[3];
  char *rp;
  u8_t *ap;
  u8_t rem;
  u8_t n;
  u8_t i;

  rp = str;
  ap = (u8_t *)&s_addr;
  for(n = 0; n < 4; n++) {
    i = 0;
    do {
      rem = *ap % (u8_t)10;
      *ap /= (u8_t)10;
      inv[i++] = '0' + rem;
    } while(*ap);
    while(i--)
      *rp++ = inv[i];
    *rp++ = '.';
    ap++;
  }
  *--rp = 0;
  return str;
}


static int net_addr_ntoa_(u32_t addr, char *str)
{
  u32_t s_addr = addr;
  char inv[3];
  char *rp;
  u8_t *ap;
  u8_t rem;
  u8_t n;
  u8_t i;

  rp = str;
  ap = (u8_t *)&s_addr;

  for(n = 0; n < 4; n++) {
    i = 0;

    do {
      rem = *ap % (u8_t)10;
      *ap /= (u8_t)10;
      inv[i++] = '0' + rem;
    } while(*ap);

    while(i--)
      *rp++ = inv[i];

    *rp++ = '.';
    ap++;
  }
  *--rp = 0;

  return rp - str;
}


u32_t arch_net_htonl(u32_t h)
{
	return htonl(h);
}

u32_t arch_net_ntohl(u32_t n)
{
	return ntohl(n);
}

u16_t arch_net_htons(u16_t h)
{
	return htons(h);
}

u16_t arch_net_ntohs(u16_t n)
{
	return ntohs(n);
}

char *arch_net_ntoa(net_addr_t* paddr)
{
	return net_addr_ntoa(paddr->s_addr);
}

int arch_net_ntoa_buf(net_addr_t* paddr, char* buf)
{
	return net_addr_ntoa_(paddr->s_addr, buf);
}


int arch_net_name2ip(const char *name, net_addr_t *addr)
{
	return netconn_gethostbyname(name, (struct ip_addr *)addr);
}


int arch_net_get_gw_mac(ip_addr_t gw_ip, u8_t *gw_mac)
{
	struct eth_addr* ethaddr_ret;
	ip_addr_t* ipaddr_ret;
	struct netif *netif;

	if (etharp_find_addr(NULL, &gw_ip, &ethaddr_ret, &ipaddr_ret) > -1){

		memcpy((void*)gw_mac, (void*)&ethaddr_ret->addr[0], sizeof(struct eth_addr));
		return STATE_OK;
	}
	return STATE_ERROR;
}

net_conn_t* arch_net_conn_new(u32_t type)
{
	return netconn_new((enum netconn_type )type);
}

//addr and port should all be Net-endian
int arch_net_connect(net_conn_t* conn, net_addr_t *addr, u16_t port)
{
    //FIXME: change for porting 
	//return netconn_connect((struct netconn *)conn, (struct ip_addr *)addr, port, 0);

	//ugly: we need this function input addr & port in NET-endian
	//But netconn_connect() accept port in Host-endian.
	u16_t p = arch_net_ntohs(port);
	return netconn_connect((struct netconn *)conn, (struct ip_addr *)addr, p);
}

int arch_net_conn_getaddr(net_conn_t* conn, net_addr_t *addr, u16_t *port, u8_t local)
{
	return netconn_getaddr((struct netconn *)conn, (struct ip_addr *)addr, port, local);
}

int arch_net_conn_recv(net_conn_t* conn, net_pkt_t** pkt)
{
	return netconn_recv((struct netconn *)(conn), (struct netbuf **)pkt);
}


int arch_net_conn_write(net_conn_t* conn, const void*buf, size_t size)
{
	return netconn_write((struct netconn *)conn , buf, size, NETCONN_COPY);
}

int arch_net_conn_sendto(net_conn_t* conn, net_pkt_t* pkt, net_addr_t*peer_ip, u16_t peer_port)
{
	return netconn_sendto((struct netconn *)conn , (struct netbuf *)pkt, (struct ip_addr *)peer_ip, peer_port);
}

int arch_net_conn_send(net_conn_t* conn, net_pkt_t* pkt)
{
	return netconn_send((struct netconn *)conn , (struct netbuf *)pkt);
}


net_pkt_t* arch_net_pkt_new(void)
{
	return (net_pkt_t*)netbuf_new();
}

u8_t *arch_net_pkt_alloc(net_pkt_t* pkt, size_t size)
{
	return (u8_t*)netbuf_alloc((struct netbuf *)pkt, size);
}


void arch_net_pkt_free(net_pkt_t* pkt)
{
	netbuf_delete((struct netbuf *)pkt);
}

//Net-endian
net_addr_t* arch_net_pkt_faddr(net_pkt_t* pkt)
{
	return (net_addr_t*)netbuf_fromaddr((struct netbuf*)pkt);
}

//Net-endian
u16_t arch_net_pkt_fport(net_pkt_t* pkt)
{
	//ugly: we need this function return port in NET-endian
	u16_t port = netbuf_fromport((struct netbuf*)pkt);
	return arch_net_htons(port);
}


u16_t arch_net_pkt_total_len(net_pkt_t* pkt)
{
	return ((struct netbuf*)pkt)->p->tot_len;
}


int arch_net_pkt_data(net_pkt_t* pkt, u8_t** ptr, u16_t *len)
{
	if(ERR_OK == netbuf_data(pkt, (void**)ptr, len))
		return STATE_OK;
	else
		return STATE_ERROR;
}


int arch_net_pkt_next(net_pkt_t* pkt)
{
	return netbuf_next((struct netbuf *)pkt);
}

int arch_net_pkt_ref(net_pkt_t* pkt, const void *dataptr,u16_t sizep)
{
	return netbuf_ref((struct netbuf *)pkt, dataptr, sizep);
}


u32_t arch_net_ip2n(const char* ip)
{
	int i1 = 0, i2 = 0,i3 = 0,i4 = 0;
	arch_rsscanf(ip, "%d.%d.%d.%d", &i1, &i2, &i3, &i4);
	return LONGVAL_LE(i1,i2,i3,i4);
}


//This auto adjust
void arch_net_hton64_buf(u64_t d64, u8_t *p)
{
	int i;
	u8_t *from = (u8_t *)&d64;
	if(12345 == htonl(12345)){
		for(i = 0; i < sizeof(u64_t); i++)
			p[i] = from[i];
	}
	else{
		for(i = 0; i < sizeof(u64_t); i++)
			p[i] = from[sizeof(u64_t)-1-i];
	}

}


//有些平台需要转换为u32_t才能被printf接受，请注意。
//snprintf(from, sizeof(from), "nbl.%u", (u32_t)d);
u64_t arch_net_ntoh64_buf(const u8_t *p)
{
	int i;
	u64_t d64;
	u8_t *to = (u8_t *)&d64;

	if(12345 == htonl(12345)){
		for(i = 0; i < sizeof(u64_t); i++)
			to[i] = p[i];
	}
	else{
		for(i = 0; i < sizeof(u64_t); i++)
			to[i] = p[sizeof(u64_t)-1-i];
	}

	return d64;
}

