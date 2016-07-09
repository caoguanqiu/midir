/*
 *  Copyright (C) 2008-2014, Marvell International Ltd.
 *  All Rights Reserved.
 */

#ifndef MDNS_HELPER_H_
#define MDNS_HELPER_H_

/* mDNS */
void init_mdns_service_struct(char *service_name);
void hp_mdns_announce(void *iface, int state);
void hp_mdns_deannounce(void *iface);
void hp_mdns_down_up(void *iface);
#endif /* ! _HELPERS_H_ */
