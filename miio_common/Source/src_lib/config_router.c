#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <lwip/lwipopts.h>
#include <lwip/arch.h>
#include <wlan.h>
#include <lib/miio_chip_rpc.h>
#include <util/util.h>
#include <ot/lib_addon.h>
#include <lib/arch_net.h>
#include <lib/arch_os.h>
#include <lib/jsmn/jsmn_api.h>
#include <malloc.h>
#include <ot/d0_otd.h>
#include <misc/led_indicator.h>
#include <app_framework.h>
#include <main_extension.h>
#include <cli.h>
#include <appln_dbg.h>
#include <call_on_worker_thread.h>
#include <lib/third_party_mcu_ota.h>
#include <lib/ota.h>
#include <partition.h>
#include <lib/uap_diagnosticate.h>
#include <lib/power_mgr_helper.h>
#include <miio_version.h>
#include <mi_smart_cfg.h>

#define  SSID_CHANED    (0xBE49)
#define  PASSWD_CHANED  (0x5A87)
#define  ANY_ONE        (0xF0F0F0F0)

struct wlan_network new_network;

static volatile uint32_t ap_info_change  __attribute__((section(".nvram_uninit")));

static void mark_ap_info_change_flag()
{
    ap_info_change = ANY_ONE;
}

void clear_ap_info_change_flag()
{
    ap_info_change = 0;
}

static bool is_ssid_changed()
{
    return (0xffff & ap_info_change) == SSID_CHANED;
}

static bool is_passwd_changed()
{
    return ((ap_info_change >> 16) & 0xffff) == PASSWD_CHANED;
}

bool is_ap_info_change()
{
    return (ANY_ONE == ap_info_change || (is_ssid_changed()) || (is_passwd_changed()));
}

static bool report_success = false;
static int report_ack(jsmn_node_t* pjn, void* ctx)
{
    jsmntok_t *jt;

    if (NULL != (jt = jsmn_key_value(pjn->js, pjn->tkn, "result"))) {
        report_success = true;
        LOG_INFO("AP info change report to server success.\r\n");
    } else {
        LOG_INFO("AP info change report fail, ack is :");
        LOG_INFO("%s\r\n",pjn->js);
    }

    return STATE_OK;
}

int report_ap_info_change_event()
{
#define JS_BUF_LEN (128)

   char *js_buf = malloc(JS_BUF_LEN);
   if(!js_buf) {
       LOG_ERROR("no enough mem to report event\r\n");
       
       //clear flags
       clear_ap_info_change_flag();
       return -1;
   }

   int n = snprintf_safe(js_buf,JS_BUF_LEN,"{\"method\":\"event.router_changed\",\"params\":");
   
   if(ap_info_change == ANY_ONE) {
       n += snprintf_safe(js_buf+n,JS_BUF_LEN - n,"\"ssid or passwd changed\"}");
   } else {
       n += snprintf_safe(js_buf+n,JS_BUF_LEN-n,"\""); 

       if(is_ssid_changed()) {
           n += snprintf_safe(js_buf+n,JS_BUF_LEN -n,"ssid changed ");
       }

       if(is_passwd_changed()) {
           n += snprintf_safe(js_buf+n,JS_BUF_LEN-n,"passwd changed");
       }
   
       n += snprintf_safe(js_buf+n,JS_BUF_LEN-n,"\"}");
   }

   LOG_DEBUG("report :len is %d %s \r\n",n,js_buf);

   //clear flags
   clear_ap_info_change_flag();
   
   report_success = false;
   int retry_count = 3;

   while(!report_success && retry_count > 0) {
       ot_api_method(js_buf,n,report_ack, NULL);
       retry_count --; 

       //it's diffcult to query ot status, so just wait
       os_thread_sleep(3000);
   }

   free(js_buf);

   return 0;
}

static int reboot_warpper()
{
    pm_reboot_soc();
    return 0;
}

int app_network_get_nw_state(void);
int app_network_get_nw(struct wlan_network *network);
int app_network_set_nw(struct wlan_network *network);
//{"method":"miIO.config_router","params":{"ssid":"xxxxxx","passwd":"xxxxx"}}
int JSON_DELEGATE(config_router)(jsmn_node_t* pjn, fp_json_delegate_ack ack, void* ctx)
{
    int ret = STATE_OK;
    const char * err;
    char *ssid = NULL, *passwd = NULL;
    size_t ssid_len, passwd_len;
    uint64_t uid = 0;

    ssid = malloc(IEEEtypes_SSID_SIZE + 1);
    if (!ssid) {
        err = "no mem";
        ret = STATE_NOMEM;
        goto err_exit;
    }

    passwd = malloc(WLAN_PSK_MAX_LENGTH); //64+1
    if (!passwd) {
        err = "no mem";
        ret = STATE_NOMEM;
        goto err_exit;
    }

    if (STATE_OK != jsmn_key2val_str(pjn->js, pjn->tkn, "ssid", ssid, IEEEtypes_SSID_SIZE + 1, &ssid_len)) {
        ret = OT_ERR_CODE_PARAM_INVALID;
        err = OT_ERR_INFO_PARAM_INVALID;
        goto err_exit;
    }

    if (STATE_OK != jsmn_key2val_str(pjn->js, pjn->tkn, "passwd", passwd, WLAN_PSK_MAX_LENGTH, &passwd_len)) {
        ret = OT_ERR_CODE_PARAM_INVALID;
        err = OT_ERR_INFO_PARAM_INVALID;
        goto err_exit;
    }


    if(STATE_OK == jsmn_key2val_u64(pjn->js,pjn->tkn,"uid",&uid)) {
        set_smt_cfg_uid_in_ram(uid);
    } else {
        set_smt_cfg_uid_in_ram(0);
    }

    char domain[10];
    size_t domain_len = 0;
    if(STATE_OK == jsmn_key2val_str(pjn->js,pjn->tkn,"country_domain",domain,COUNTRY_DOMAIN_LENGTH + 1,&domain_len)) {
        //store to psm 
        set_country_domain_in_psm(domain);
        
        //change domain stored in OT 
        struct otn* loop_ot = (struct otn*)d0_api_find(OTN_XOBJ_NAME_DEFAULT);
        if(loop_ot){
            
            udp_ot_t* otu = loop_ot->otu;
			tcpc_ot_t* ott = loop_ot->ott;

            strcpy(otu->host_domain_preset, domain);
            strcat(otu->host_domain_preset, ".");
            strcat(otu->host_domain_preset, OT_UDP_DOMAIN);

			//Does it needed ? 
            strcpy(ott->host_domain_preset, domain);
            strcat(ott->host_domain_preset, ".");
            strcat(ott->host_domain_preset, OT_TCP_DOMAIN);
        }

    }

    extern struct wlan_network new_network;

    memset(&new_network, 0, sizeof(struct wlan_network));
    //security
    new_network.security.type = WLAN_SECURITY_WILDCARD;

    //passphase
    new_network.security.psk_len = passwd_len;
    memcpy(new_network.security.psk, passwd, passwd_len);

    //ssid 
    memcpy(new_network.ssid, ssid, ssid_len);
    new_network.ssid[ssid_len] = 0;

    new_network.ip.ipv4.addr_type = 1; //use dhcp

    bool should_reboot = false;
    
    ap_info_change = 0;
    //check reconfig or not
    if(APP_NETWORK_PROVISIONED == app_network_get_nw_state()) {
        struct wlan_network network;
        ret = app_network_get_nw(&network);
        if(WM_SUCCESS == ret) { 

            if(0 != strncmp(network.ssid,new_network.ssid,IEEEtypes_SSID_SIZE)) {
                ap_info_change |= SSID_CHANED;
            }
                
            if((network.security.psk_len != new_network.security.psk_len) || 
                    (0 != memcmp(network.security.psk,new_network.security.psk,network.security.psk_len))) {
                ap_info_change |= (PASSWD_CHANED << 16);
             }
                    
            if(0 == ap_info_change) {
                LOG_DEBUG("reconfig with the same ssid & passwd \r\n");
                goto err_exit1;
            }
        }
        
        //could not load network or set a new ssid || passwd
        ret = app_network_set_nw(&new_network);
        if(ret != WM_SUCCESS) {
            err = OT_ERROR_INFO_CONFIG_ROUTER;
            ret = OT_ERROR_CODE_CONFIG_ROUTER; //not known
            goto err_exit;
        }

        should_reboot = true;
    } else {
        
        //provision ap in UAP mode
        should_reboot = false;
        ret = app_sta_save_network_and_start(&new_network);
    } 

    //should_reboot but ap_info_change not set,which means could not load configed network 
    if(should_reboot && (0 == ap_info_change)) {
        mark_ap_info_change_flag();
    } 
    
err_exit1:    
    if(ssid)
		free(ssid);
	if(passwd)
		free(passwd);
    json_delegate_ack_result(ack, ctx, ret);
   
    if(should_reboot) {
        //make sure send ack to cloud 
        call_delayed_task_on_worker_thread(reboot_warpper,NULL,2000,0);
    }
    return ret;

err_exit: 
    if (ssid)
        free(ssid);
    if (passwd)
        free(passwd);
    json_delegate_ack_err(ack, ctx, ret, err);
    return ret;
}

FSYM_EXPORT(JSON_DELEGATE(config_router));
