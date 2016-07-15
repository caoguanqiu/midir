#include <wm_os.h>
#include <app_framework.h>
#include <wmtime.h>
#include <partition.h>
#include <appln_cb.h>
#include <appln_dbg.h>
#include <cli.h>
#include <cli_utils.h>
#include <wlan.h>
#include <rfget.h>
#include <psm.h>
#include <wmstdio.h>
#include <wmsysinfo.h>
#include <wm_net.h>
#include <mdev_aes.h>
#include <board.h>
#include <mdev_gpio.h>
#include <pwrmgr.h>
#include <stdlib.h>
#include <httpd.h>
#include <stdint.h>
#include <lib/lib_generic.h>
#include <lib/jsmn/jsmn_api.h>
#include <lib/cmp.h>
#include <lib/arch_os.h>
#include <wmassert.h>
#include <wmlist.h>
#include <mrvlperf.h>

#include <misc/led_indicator.h>
#include <misc/miio_cli.h>
#include <mi_smart_cfg.h>

#include <lib/crash_trace.h>
#include <lib/uap_diagnosticate.h>
#include <lib/miio_command.h>
#include <lib/ota.h>

#include <ot/lib_xobj.h>
#include <ot/lib_addon.h>
#include <ot/delayzero.h>
#include <ot/d0_otd.h>

#include <util/util.h>
#include <malloc.h>
#include "lib/button_detect_framework.h"
#include "util/dump_hex_info.h"
#include "lib/power_mgr_helper.h"
#include <boot_flags.h>
#include <main_extension.h>
#include <call_on_worker_thread.h>
#include <mdns_helper.h>
#include <firmware_structure.h>
#include <crc32.h>
#include <lib/config_router.h>

#include <communication.h>

#define FACTORY_NET_SSID        "miio_default"
#define FACTORY_NET_PASSPHASE   "0x82562647"

struct tag_cfg_struct cfg_struct = {
    .mac_address   = "TAG_MAC", 
    .device_id      = "TAG_DID",
    .key            = "TAG_KEY",
    .model          = "TAG_MODEL",
    .url            = "TAG_URL",
    .domain         = "TAG_DOMAIN",
};

struct fixed_cfg_data_t fixed_cfg_data = {
    .dport = (u16_t)8053,
    .lport = (u16_t)54321,
    .interval = (u8_t)10,
    .enauth = (u8_t)ENAUTH_DEFAULT_VALUE,
};


struct provisioned_t g_provisioned = {
		.state = 0
};

static char dhcp_name_strval[120];
uint8_t token[16];       //should put into psm
extern os_queue_t main_thread_sched_queue;
static uint8_t mdns_announced;

void read_provision_status(void)
{
    char psm_val[65];
    psm_handle_t handle;
    int ret;

    if ((ret = psm_open(&handle, "network")) != 0) {
        wmprintf("open psm network error\r\n");
        return;
    }
    if (psm_get(&handle, "configured", psm_val, sizeof(psm_val)) != 0) {
        psm_close(&handle);
        return;
    }
    if (0 == strcmp(psm_val, "1"))
        g_provisioned.state = APP_NETWORK_PROVISIONED;
    else
        g_provisioned.state = APP_NETWORK_NOT_PROVISIONED;

    psm_close(&handle);
}

bool is_provisioned(void)
{
    return (APP_NETWORK_PROVISIONED == g_provisioned.state);
}

void led_network_state_observer(device_network_state_t state)
{
    switch(state) {
        case WAIT_SMT_TRIGGER:
			semnetnotify=1;
            if(is_provisioned()) {
                led_connecting();
#ifdef MIIO_COMMANDS
                mcmd_enqueue_raw("MIIO_net_change offline");
#endif
            } else {
                led_waiting_trigger();
#ifdef MIIO_COMMANDS
                mcmd_enqueue_raw("MIIO_net_change unprov");
#endif
            }
            break;
        case UAP_WAIT_SMT:
			semnetnotify=1;
            led_diagnosing();
#ifdef MIIO_COMMANDS
            mcmd_enqueue_raw("MIIO_net_change uap");
#endif
            break;
        case SMT_CONNECTING:
			semnetnotify=1;
            led_connecting();
#ifdef MIIO_COMMANDS
            mcmd_enqueue_raw("MIIO_net_change offline");
#endif
            break;
        case GOT_IP_NORMAL_WORK:
			semnetnotify=1;
            led_connected();
#ifdef MIIO_COMMANDS
            mcmd_enqueue_raw("MIIO_net_change local");
#endif
            break;
        case SMT_CFG_STOP:
			semnetnotify=1;
            led_stop();
#ifdef MIIO_COMMANDS
            mcmd_enqueue_raw("MIIO_net_change stop");
#endif
            break;
    }
}

static int one_time_flag = 0;
static d0_event_ret_t ot_online_notifier(struct d0_event* evt, void* ctx)
{
        LOG_DEBUG("===>online event \r\n");
		semnetnotify=1;
        if(one_time_flag == 0) {
            report_ota_state();
            one_time_flag = 1;
        }
}

static int start_otd(void)
{
    //fill domain
    memset(cfg_struct.domain,0,sizeof(cfg_struct.domain));
    /*check country domain exist or not */
    if(WM_SUCCESS == get_country_domain_in_psm((char *)(&cfg_struct.domain),sizeof(cfg_struct.domain))) {
        LOG_DEBUG("country domain is in psm \r\n");
        // the total lenth is less than 64 
        strcat(cfg_struct.domain , ".");
    }
    strcat(cfg_struct.domain,OT_UDP_DOMAIN);


	char tcp_domain[20];
	memset(tcp_domain,0,20);
	if(WM_SUCCESS == get_country_domain_in_psm(tcp_domain,20)) {
		strcat(tcp_domain,".");
	}
	strcat(tcp_domain,OT_TCP_DOMAIN);

	otn(fixed_cfg_data.lport, 
		cfg_struct.domain, 
		fixed_cfg_data.dport, 
		fixed_cfg_data.enauth,		//If we enable token publish

		tcp_domain, 
		80, 

		(const u8_t*)cfg_struct.device_id, 
		(const u8_t *)cfg_struct.key,
		token);

	return 0;
}

/****************************************
* generate new token to global token array
****************************************/
static int deal_with_token(bool should_re_gen)
{
    int ret;

    #define STR_LEN 35
    char strval[STR_LEN] = {0};
    psm_handle_t handle;

    ret = psm_open(&handle,"ot_config");
    if(ret) {
        LOG_ERROR("open psm area error \r\n");
        return -WM_FAIL;
    }
    
    ret = psm_get(&handle,OT_TOKEN,strval,STR_LEN);
    if(!ret && !should_re_gen) { //token already exist
        //read token_string to token array
        arch_axtobuf_detail(strval, sizeof(strval), token, sizeof(token), NULL);
        goto exit;
    }

    //should regenerator token
    if(ret) { //which mean first time generate token
        memset(token,0,sizeof(token));
    }

    //max length is 10 + 16 + 16 +6 = 48 
    #define GEN_BUF_LEN (52)
    uint8_t  token_generate_buffer[GEN_BUF_LEN];
    int n = 0;

    n = snprintf_safe((char *)token_generate_buffer,GEN_BUF_LEN,"%u",os_ticks_get());
   
    memcpy(token_generate_buffer + n,cfg_struct.key,OT_PROTO_DEVICE_KEY_SIZE);
    n+= OT_PROTO_DEVICE_KEY_SIZE;
    
    memcpy(token_generate_buffer + n,token,sizeof(token));
    n += sizeof(token);

    memcpy(token_generate_buffer + n,cfg_struct.mac_address,6);
    n += 6;
    
    //generate token
    md5(token_generate_buffer,n,token);
   
    snprintf_safe(strval,STR_LEN,"%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x",
                token[0],
                token[1],
                token[2],
                token[3],
                token[4],
                token[5],
                token[6],
                token[7],
                token[8],
                token[9],
                token[10],
                token[11],
                token[12],
                token[13],
                token[14],
                token[15]
                );
    ret = psm_set(&handle,OT_TOKEN,strval);
    if(ret){
        LOG_ERROR("write token fail");
        goto exit;
    }

    LOG_DEBUG("generate token success \r\n");

exit:
    psm_close(&handle);
    return ret;
}

/**********************************************************
 config cfg struct from otp
**********************************************************/
static void config_cfg_struct()
{
    bool * flag = get_read_otp_flag();

    if(false == (*flag)) {
        
        wlan_set_mac_addr(get_wlan_mac_address());
    
    } else {
    
        int ret;
        //get mac address 
        uint8_t mac[MLAN_MAC_ADDR_LENGTH];
        wlan_get_mac_address((uint8_t *)mac);
        memcpy(cfg_struct.mac_address,mac,MLAN_MAC_ADDR_LENGTH);

        //get key & did,lengthof(key) + lengthof(did) = 24  
        uint8_t buffer[24];
        ret = wlan_get_otp_user_data(buffer,24);
        if(WM_SUCCESS == ret) {
            //copy did & key to cfg_struct
            memcpy(cfg_struct.device_id,buffer,sizeof(cfg_struct.device_id));
            
            memcpy(cfg_struct.key,buffer+sizeof(cfg_struct.device_id),sizeof(buffer) - sizeof(cfg_struct.device_id));
        } else {
            LOG_ERROR("read otp fail \r\n");
            pm_reboot_soc();
        }
    }

    if(WM_SUCCESS != get_model_in_psm(cfg_struct.model,sizeof(cfg_struct.model))) {
        //cfg_struct.url is useless now, define module only
        strcpy(cfg_struct.model,DEFAULT_MODULE);
#if defined(CONFIG_CPU_MC200) && defined (CONFIG_WiFi_8801)
        strcat(cfg_struct.model,"1");
#elif defined(CONFIG_CPU_MC200) && defined (CONFIG_WiFi_878x)
        strcat(cfg_struct.model,"3");
#elif defined(CONFIG_CPU_MW300) && defined(MW300_68PIN)
        strcat(cfg_struct.model,"4");
#elif defined(CONFIG_CPU_MW300)
        strcat(cfg_struct.model,"2");
#endif
        set_model_in_psm(cfg_struct.model);
    
    }

}

uint32_t get_trigger_uap_info();
void set_trigger_uap_info(uint32_t value);

int app_unload_configured_network();
int app_ctrl_notify_event(app_ctrl_event_t event, void *data);

#ifdef MIIO_COMMANDS
d0_event_ret_t mum_offline_notifier(struct d0_event* evt, void* ctx);
d0_event_ret_t mum_online_notifier(struct d0_event* evt, void* ctx);
#endif
void event_wlan_init_done(void *data)
{
	int ret;
	char temp[20];
    
    //init cfg_struct from otp
    config_cfg_struct();
    
    //print did & mac
    uint32_t digital_did = get_device_did_digital();
    
    //for factory check did and mac
    wmprintf("digital_did is %d \r\n", digital_did);
    snprintf_safe(temp,sizeof(temp),"%.2x%.2x%.2x%.2x%.2x%.2x",
                    cfg_struct.mac_address[0],
                    cfg_struct.mac_address[1],
                    cfg_struct.mac_address[2],
                    cfg_struct.mac_address[3],
                    cfg_struct.mac_address[4],
                    cfg_struct.mac_address[5]
                    );
    wmprintf("mac address is %s\r\n",temp);//for factory checking
    wmprintf("last four byte of key is %.2x%.2x%.2x%.2x\r\n",cfg_struct.key[OT_PROTO_DEVICE_KEY_SIZE -4],
                                                               cfg_struct.key[OT_PROTO_DEVICE_KEY_SIZE -3],
                                                               cfg_struct.key[OT_PROTO_DEVICE_KEY_SIZE -2],
                                                               cfg_struct.key[OT_PROTO_DEVICE_KEY_SIZE -1]
                                                               );

    //print chip boot status info
    if (is_factory_mode_enable()) {
        wmprintf(FACTORY_STARTUP_PASS);
    } else {
        wmprintf(FACTORY_NORMAL_BOOT);
    }

    //gen token
    deal_with_token(false); 

    //set wifi work channel 
    wlan_set_country(COUNTRY_CN);

#ifndef RELEASE
    LOG_DEBUG("dump token value\r\n");
    xiaomi_dump_hex(token,16);
#endif

    ret = netif_add_udp_broadcast_filter(fixed_cfg_data.lport);
    if (ret != WM_SUCCESS) {
        LOG_ERROR("Failed to add ot local port to boardcast filter \r\n");
    }

    //ot start as idle
    start_otd();


#ifdef MIIO_COMMANDS
    OTN_ONLINE(mum_online_notifier);
    OTN_OFFLINE(mum_offline_notifier);
#endif
    OTN_ONLINE(ot_online_notifier);

    //generate dhcp name
    strcpy(dhcp_name_strval,cfg_struct.model);
    char *c = dhcp_name_strval;
    while(*c) {
        if('.' == *c) {
            *c = '-';
        }
        c++;
    }

    strcat(dhcp_name_strval,"_miio");
    snprintf_safe(temp,20,"%lu",digital_did);
    strcat(dhcp_name_strval,temp);

    /*
     * Start mDNS and advertize our hostname using mDNS
     */
    init_mdns_service_struct(dhcp_name_strval);


    ret = is_factory_mode_enable();

    //factory mode,connect to miio_default
    if(ret) {
        wmprintf("in factory mode, will auto connect to miio_default\r\n");

        struct wlan_network factory_net;

        memset(&factory_net,0,sizeof(struct wlan_network));
        strcpy(factory_net.name,FACTORY_NET_SSID);
        strcpy(factory_net.ssid,FACTORY_NET_SSID); 

        factory_net.security.type = WLAN_SECURITY_WPA2;
        
        factory_net.security.psk_len = strlen(FACTORY_NET_PASSPHASE);
        memcpy(factory_net.security.psk,FACTORY_NET_PASSPHASE,strlen(FACTORY_NET_PASSPHASE));

        factory_net.ip.ipv4.addr_type = 1; //use dhcp
        
        //will connect to miio_default
        app_sta_start_by_network(&factory_net);

	    goto safe_exit;

    }

    g_provisioned.state = (int) data;
    //already provisioned
    if(APP_NETWORK_PROVISIONED == g_provisioned.state) {
        LOG_DEBUG("already provisioned \r\n");
        set_device_network_state(SMT_CONNECTING);
        app_sta_start(); //just start sta mode , & it will auto connect to network
	    goto safe_exit;
     }

    //not provisioned & should start uap
    //Note: this interface only should be used for testing
    if(ENABLE_UAP == get_trigger_uap_info()) {
        wmprintf("should start UAP \r\n");
        
        app_uap_start_on_channel_with_dhcp(construct_ssid_for_smt_cfg(), NULL,DEFAULT_AP_CHANNEL);
        goto safe_exit;
    }


#if defined(ENABLE_AIRKISS) && (ENABLE_AIRKISS == 1)
    //not yet provisioned,start kuailian
    set_device_network_state(WAIT_SMT_TRIGGER);

    //trigger prov
	app_ctrl_notify_event(AF_EVT_INTERNAL_PROV_REQUESTED, (void *)NULL);

    //trigger scan
    void * msg;
    msg = (void *)SCAN_ROUTER;
    os_queue_send(&main_thread_sched_queue, &msg, OS_NO_WAIT);

#else
    LOG_DEBUG("start uap mode for smart config \r\n");
    app_uap_start_on_channel_with_dhcp(construct_ssid_for_smt_cfg(), NULL,DEFAULT_AP_CHANNEL);
    
    //start monitor timer 
    if(smt_cfg_monitor_init() ==WM_SUCCESS) {
        smt_cfg_monitor_start();
    }
#endif

safe_exit:
    return;

}


void event_normal_connecting(void *data)
{
    set_device_network_state(SMT_CONNECTING);

    //smart config complete, so stop & destory the monitor_timer
    smt_cfg_monitor_stop();
    smt_cfg_monitor_destory();

    LOG_DEBUG("net dhcp host name set %s\r\n",dhcp_name_strval);
    net_dhcp_hostname_set(dhcp_name_strval);

	LOG_DEBUG("Connecting to Home Network\r\n");
}

void event_normal_connected(void *data)
{
    char ip[16];

    set_device_network_state(GOT_IP_NORMAL_WORK);

	app_network_ip_get(ip);
	LOG_DEBUG("Connected to provisioned network with ip address =%s\r\n", ip);
    
    if(is_factory_mode_enable())wmprintf(FACTORY_JOINAP_PASS);

    LOG_DEBUG("Starting mdns\r\n");
    app_mdns_start(dhcp_name_strval);

    {
        void *iface_handle;
        iface_handle = net_get_sta_handle();

        if (!mdns_announced) {
            hp_mdns_announce(iface_handle, UP);
            mdns_announced = 1;
        } else {
            hp_mdns_down_up(iface_handle);
        }
    }


	OTN_NETIF_CHANGE();

    //start nb
    void *msg;
    msg = (void *)NB_OPERATION;
    os_queue_send(&main_thread_sched_queue,&msg,OS_NO_WAIT);
}

static void report_reset_event()
{
#define JS_BUF_LEN (64)

   char *js_buf = malloc(JS_BUF_LEN);
   if(!js_buf) {
       return;
   }

   int n = snprintf_safe(js_buf,JS_BUF_LEN,"{\"method\":\"event.dev_is_reset\",\"params\":\"\"}");

   LOG_DEBUG("report :len is %d %s \r\n",n,js_buf);

   ot_api_method(js_buf,n, NULL, NULL);

   //make sure the report is send out before reboot, but do not assure reach the server
   os_thread_sleep(100);

   free(js_buf);
}

void event_normal_pre_reset_prov(void *data) 
{
    report_reset_event();

    hp_mdns_deannounce(net_get_sta_handle());

    //regenerator token
    deal_with_token(true);

    //this function can be called even contry domain donot exist in psm
    erase_country_domain_in_psm();

    //reset enauth at the same time
    uint8_t enauth;
    if(WM_SUCCESS == get_enauth_data_in_psm(&enauth)) {
        if(enauth == ENAUTH_DEFAULT_VALUE) {
            LOG_DEBUG("should not rewrite enauth\r\n");
            return;
        }
    }
    
    //come here, so we should reset enauth data
    set_enauth_data_in_psm(ENAUTH_DEFAULT_VALUE);
}


static void event_normal_dhcp_renew(void *data)
{
	void *iface_handle = net_get_sta_handle();

	OTN_NETIF_CHANGE();
	hp_mdns_announce(iface_handle, REANNOUNCE);
}

void event_normal_reset_prov(void *data) {
    pm_reboot_soc();
}

static void event_uap_started(void *data)
{
	LOG_DEBUG("Event: Micro-AP Started.\r\n");

    set_device_network_state(UAP_WAIT_SMT);
    //event otd
	OTN_NETIF_CHANGE();
	
    /*{
		void *iface_handle = net_get_uap_handle();
		hp_mdns_announce(iface_handle, UP);
	}*/


}

static void event_uap_stopped(void *data)
{
	OTN_NETIF_CHANGE();
	LOG_DEBUG("Event: Micro-AP Stopped.\r\n");
}

static void event_normal_link_lost(void *data)
{
	OTN_NETIF_CHANGE();
	LOG_DEBUG("Event: Link lost.\r\n");
	app_mdns_stop();
}

static void ps_state_to_desc(char *ps_state_desc, int ps_state)
{
	switch (ps_state) {
	case WLAN_IEEE:
		strncpy(ps_state_desc, "IEEE PS",32);
		break;
	case WLAN_DEEP_SLEEP:
		strncpy(ps_state_desc, "Deep sleep",32);
		break;
	case WLAN_PDN:
		strncpy(ps_state_desc, "Power down",32);
		break;
	case WLAN_ACTIVE:
		strncpy(ps_state_desc, "WLAN Active",32);
		break;
	default:
		strncpy(ps_state_desc, "Unknown",32);
		break;
	}
}

/*
 * Event: PS ENTER
 *
 * Application framework event to indicate the user that WIFI
 * has entered Power save mode.
 */

static void event_ps_enter(void *data)
{
	int ps_state = (int) data;
	char ps_state_desc[32];
	if (ps_state == WLAN_PDN) {
		LOG_DEBUG("NOTE: Due to un-availability of "
		    "enough dynamic memory for ");
		LOG_DEBUG("de-compression of WLAN Firmware, "
		    "exit from PDn will not\r\nwork with wm_demo.");
		LOG_DEBUG("Instead of wm_demo, pm_mc200_wifi_demo"
		    " application demonstrates\r\nthe seamless"
		    " exit from PDn functionality today.");
		LOG_DEBUG("This will be fixed in subsequent "
		    "software release.\r\n");
	}
	ps_state_to_desc(ps_state_desc, ps_state);
	LOG_DEBUG("Power save enter : %s\r\n", ps_state_desc);

}

/*
 * Event: PS EXIT
 *
 * Application framework event to indicate the user that WIFI
 * has exited Power save mode.
 */

static void event_ps_exit(void *data)
{
	int ps_state = (int) data;
	char ps_state_desc[32];
	ps_state_to_desc(ps_state_desc, ps_state);
	LOG_DEBUG("Power save exit : %s", ps_state_desc);
}

void channel_switch_handle(int channel_num)
{
    //only use uap mode for smart config, so channel_switch event is useless
}

/* This is the main event handler for this project. The application framework
 * calls this function in response to the various events in the system.
 */
int common_event_handler(int event, void *data)
{
    switch (event) {
    case AF_EVT_WLAN_INIT_DONE:
        event_wlan_init_done(data);
        break;
    case AF_EVT_NORMAL_CONNECTING:
        g_provisioned.state = APP_NETWORK_PROVISIONED;
        event_normal_connecting(data);
        break;
    case AF_EVT_NORMAL_CONNECTED:
        event_normal_connected(data);
        break;
    case AF_EVT_NORMAL_CONNECT_FAILED:
        app_mdns_stop();
        break;
    case AF_EVT_NORMAL_LINK_LOST:
	    event_normal_link_lost(data);
        break;
    case AF_EVT_NORMAL_USER_DISCONNECT:
        LOG_WARN("wifi disconnected!\r\n");
        break;
    case AF_EVT_NORMAL_PRE_RESET_PROV:
        LOG_DEBUG("pre reset to prov\r\n");
        event_normal_pre_reset_prov(data);
        break;
    case AF_EVT_NORMAL_RESET_PROV:
        LOG_DEBUG("reset to prov\r\n");
        g_provisioned.state = APP_NETWORK_NOT_PROVISIONED;
        event_normal_reset_prov(data);
        break;
    case AF_EVT_NORMAL_DHCP_RENEW:
		event_normal_dhcp_renew(data);
		break;
	case AF_EVT_UAP_STARTED:
		event_uap_started(data);
		break;
	case AF_EVT_UAP_STOPPED:
		event_uap_stopped(data);
		break;
    case AF_EVT_PROV_DONE:
		wmprintf("event provision done \r\n");
        break;
	case AF_EVT_PROV_CLIENT_DONE:
		wmprintf("event prov client done\r\n");
        break;
	case AF_EVT_PS_ENTER:
		event_ps_enter(data);
		break;
	case AF_EVT_PS_EXIT:
		event_ps_exit(data);
		break;
    case AF_EVT_NORMAL_CHANNEL_SWITCH:
        {
            wlan_channel_switch_t *chan_switch = (wlan_channel_switch_t *)data;
            if (chan_switch->event_subtype == EVENT_SUBTYPE_LAST_PACKET) {
                //channel switch handle
                channel_switch_handle(chan_switch->chan_number);
		        LOG_DEBUG("End of Channel: %d\r\n", chan_switch->chan_number);
            } else if (chan_switch->event_subtype == EVENT_SUBTYPE_FIRST_PACKET) {
		        LOG_DEBUG("Channel switch to: %d\r\n", chan_switch->chan_number);
            }
            
        }
        break;
	default:
		break;
	}

    return 0;
}


