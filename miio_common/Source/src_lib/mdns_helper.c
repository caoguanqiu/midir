/*
 *  Copyright (C) 2008-2016, Marvell International Ltd.
 *  All Rights Reserved.
 */
#include <wmstdio.h>
#include <app_framework.h>
#include <mdns_helper.h>
#include <appln_cb.h>
#include <appln_dbg.h>
#include <wm_utils.h>

static struct mdns_service my_service = {
	.servname = NULL,
	.servtype = "miio",
	.domain = "local",
	.proto = MDNS_PROTO_UDP,
	.port = 54321,
};

/***********************************************
* server name must be 0 end string
************************************************/
void init_mdns_service_struct(char *service_name)
{
    my_service.servname = service_name; 
}

#define MAX_MDNS_TXT_SIZE 128
char mytxt[MAX_MDNS_TXT_SIZE];

uint8_t *get_wlan_mac_address(void);

void hp_mdns_announce(void *iface, int state)
{
	if (state == UP) {
        unsigned char * mac_addr = get_wlan_mac_address();

        //FIXME: new sdk has no method sys_get_epoch, so jsut set epoch=1
		snprintf(mytxt, MAX_MDNS_TXT_SIZE, "epoch=%d:mac=%.2x%.2x%.2x%.2x%.2x%.2x",1,mac_addr[0],
                                                                          mac_addr[1],
                                                                          mac_addr[2],
                                                                          mac_addr[3],
                                                                          mac_addr[4],
                                                                          mac_addr[5]);
		mdns_set_txt_rec(&my_service, mytxt, ':');
		app_mdns_add_service(&my_service, iface);
	} else {
		app_mdns_iface_state_change(iface, state);
	}
}

void hp_mdns_deannounce(void *iface)
{
	app_mdns_remove_service(&my_service, iface);
}

void hp_mdns_down_up(void *iface)
{
	/* mdns interface state is changed to DOWN to
	 * to remove previous mdns service from
	 * mdns querier cache. Later state is changed
	 * to UP to add services in querier cache.
	 */
	app_mdns_iface_state_change(iface, DOWN);
	/* 100ms sleep is added to send mdns down packet before
	 * announcing service again.
	 */
	os_thread_sleep(os_msec_to_ticks(100));
	hp_mdns_announce(iface, UP);

}
