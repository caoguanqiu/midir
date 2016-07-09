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
#include <airkiss/airkiss_config.h>

extern struct tag_cfg_struct cfg_struct;
extern struct fixed_cfg_data_t fixed_cfg_data; 

struct runtime_config_t
{
    uint64_t smt_cfg_uid;   //uid passed by smart config
};

struct runtime_config_t runtime_config = {
        .smt_cfg_uid = 0,
}; 

uint64_t get_smt_cfg_uid_in_ram()
{
    return runtime_config.smt_cfg_uid;
}

void set_smt_cfg_uid_in_ram(uint64_t data)
{
    runtime_config.smt_cfg_uid = data;
}

uint8_t get_enauth_data_in_ram()
{
    return fixed_cfg_data.enauth;
}

void set_enauth_data_in_ram(uint8_t enauth)
{
    fixed_cfg_data.enauth = enauth;
}

int get_enauth_data_in_psm(uint8_t *enauth)
{
    char psm_val[20];
    psm_handle_t handle;
    int psm_enauth;

    int ret;

    if ((ret = psm_open(&handle, "ot_config")) != 0) {
        LOG_ERROR("open psm ot_config error\r\n");
        return -1;
    }

    if (psm_get(&handle, "enauth", psm_val, sizeof(psm_val)) != 0) {
        ret = -1;
        goto get_enauth_error_handle;
    }

    ret = sscanf(psm_val,"%d",&psm_enauth);
    if(ret <0) {
        ret = -1;
        goto get_enauth_error_handle;
    } 

    if((psm_enauth != 0) && (psm_enauth != 1)) {
        psm_enauth = fixed_cfg_data.enauth;
        snprintf_safe(psm_val,sizeof(psm_val),"%d",fixed_cfg_data.enauth);
        psm_set(&handle,"enauth",psm_val); 
    }

    *enauth = psm_enauth & 0xff;
    ret = WM_SUCCESS;

get_enauth_error_handle:
    psm_close(&handle);
    return ret;
}

int set_enauth_data_in_psm(uint8_t enauth)
{
    char psm_val[20];
    psm_handle_t handle;

    int ret = WM_SUCCESS;

    if ((ret = psm_open(&handle, "ot_config")) != 0) {
        LOG_ERROR("open psm network error\r\n");
        return -1;
    }

    snprintf_safe(psm_val, sizeof(psm_val), "%d", enauth);
    psm_set(&handle, "enauth", psm_val);
    psm_close(&handle);

    return ret;
}

void set_dport_data_in_ram(uint16_t dport)
{
    fixed_cfg_data.dport = dport;
}

int get_model_in_psm(char *model,size_t length)
{
    psm_handle_t handle;

    int ret;

    if ((ret = psm_open(&handle, "ot_config")) != 0) {
        LOG_ERROR("open psm ot_config error\r\n");
        return -1;
    }

    if (psm_get(&handle, "psm_model", model,length ) != 0) {
        ret = -1;
        goto get_model_error_handle;
    }

    ret = WM_SUCCESS;

get_model_error_handle:
    psm_close(&handle);
    return ret;
}

int set_model_in_psm(const char * model)
{
    psm_handle_t handle;
    int ret = WM_SUCCESS;

    if ((ret = psm_open(&handle, "ot_config")) != 0) {
        LOG_ERROR("open psm network error\r\n");
        return -1;
    }

    psm_set(&handle, "psm_model", model);
    psm_close(&handle);

    return ret;
}

psm_hnd_t psm_get_handle();
#define COUNTRY_DOMAIN_PSM_NAME "country_domain"
int set_country_domain_in_psm(const char * country_domain)
{
    psm_hnd_t handle = psm_get_handle();
    if(!handle) {
        return -WM_FAIL;
    }

    int length = strnlen(country_domain,COUNTRY_DOMAIN_LENGTH + 1);

    return psm_set_variable(handle,COUNTRY_DOMAIN_PSM_NAME,country_domain,length);
}

int get_country_domain_in_psm(char * buffer,int buffer_length)
{
    psm_hnd_t handle = psm_get_handle();
    if(!handle) {
        return -WM_FAIL;
    }
    
    int read_len = psm_get_variable(handle,COUNTRY_DOMAIN_PSM_NAME,buffer, buffer_length - 1);
	if (read_len >= 0) {
		buffer[read_len] = 0;
		return WM_SUCCESS;
	}

	return -WM_FAIL;
}

int erase_country_domain_in_psm()
{
    psm_hnd_t handle = psm_get_handle();
	if (!handle) {
		return -WM_FAIL;
	}

	return psm_object_delete(handle, COUNTRY_DOMAIN_PSM_NAME);
}

uint8_t *get_wlan_mac_address(void)
{
   return (uint8_t *) cfg_struct.mac_address;
}

uint32_t get_device_did_digital(void)
{
    uint32_t digital_did = 0;
    digital_did = ((uint32_t)cfg_struct.device_id[7] & 0xff) | (((uint32_t)cfg_struct.device_id[6] & 0xff) << 8)
                  | (((uint32_t)cfg_struct.device_id[5] & 0xff) << 16) | (((uint32_t)cfg_struct.device_id[4] & 0xff) << 24);
    
    return digital_did;
}

/* device network status*/
struct {
	device_network_state_t state; // FIXME: THIS IS A WORKAROUND, IF NOT STRUCT, GLOABAL VARIABLE WILL CHANGE UNEXPECTEDLY
} g_device_network;
os_mutex_t network_state_mutex;
/*work state observer list*/
LIST_HEAD(network_state_oberver_list);

static void notify_oberver(device_network_state_t state);
void set_device_network_state(device_network_state_t state)
{
    bool state_legal = false;

    switch(state) {
        case WAIT_SMT_TRIGGER:
            g_device_network.state = state;
            state_legal = true;
            LOG_DEBUG("WAIT_SMT_TRIGGER\r\n");
            //put observer call here
            break;
        case SMT_CONNECTING:
            g_device_network.state = state;
            state_legal = true;
            LOG_DEBUG("SMT_CONNECTING\r\n");
            //put observer call here
            break;
        case GOT_IP_NORMAL_WORK:
            g_device_network.state = state;
            state_legal = true;
            LOG_DEBUG("GOT_IP_NORMAL_WORK\r\n");
            //put observer call here
            break;
        case UAP_WAIT_SMT:
            g_device_network.state = state;
            state_legal = true;
            LOG_DEBUG("UAP_WAIT_SMT\r\n");
            break;
        case SMT_CFG_STOP:
            g_device_network.state = state;
            state_legal = true;
            LOG_DEBUG("SMT_CFG_STOP \r\n");
            break;
        default:
            LOG_ERROR("unknown state \r\n");
    }

    if(state_legal) {
        notify_oberver(g_device_network.state);
    }
}

device_network_state_t get_device_network_state(void)
{
    return g_device_network.state;
}

static void notify_oberver(device_network_state_t state)
{
    dev_work_state_obv_item_t *iter = NULL;

    os_mutex_get(&network_state_mutex,OS_WAIT_FOREVER);
    
    list_for_each_entry(iter,&network_state_oberver_list,list) {
        if(iter && iter->func) {
            (iter->func)(state);
        }
    }

    os_mutex_put(&network_state_mutex);
}

int register_device_network_state_observer(dev_state_observer_func func)
{
    int ret = 0;

    os_mutex_get(&network_state_mutex,OS_WAIT_FOREVER);
    
    dev_work_state_obv_item_t * item = malloc(sizeof(dev_work_state_obv_item_t));
    if(!item) {
        ret = -1;
        goto register_error;
    }
    
    item->func = func;  
    list_add(&(item->list),&network_state_oberver_list);
    
register_error:
    os_mutex_put(&network_state_mutex);
    return ret;
}

int deregister_device_network_state_observer(dev_state_observer_func func)
{
    bool found = false;
    dev_work_state_obv_item_t *iter = NULL;

    os_mutex_get(&network_state_mutex,OS_WAIT_FOREVER);
    list_for_each_entry(iter,&network_state_oberver_list,list) {
       if(iter && iter->func == func) {
            found = true;
            break;
       } 
    }

    if(found && iter) {
        list_del(&(iter->list));
        free(iter);
    }

    os_mutex_put(&network_state_mutex);
    return 0;
}

char mcu_fw_version[10]="";

void get_mcu_version_from_psm()
{
    int ret = 0;

    psm_handle_t handle;

    ret = psm_open(&handle, "ot_config");
    if(ret) {
        return;
    }

    ret = psm_get(&handle, "mcu_version", mcu_fw_version, sizeof(mcu_fw_version));
    if(!ret) {
    	LOG_DEBUG("mcu_fw_version is %s\r\n", mcu_fw_version);
    }

    psm_close(&handle);

}

#define STR_BUF_SZ (120)

//only mc200 + 8801 use psm to store tag info, other platform use OTP instead
#if ( defined(CONFIG_CPU_MC200) && defined (CONFIG_WiFi_8801) ) || defined (FORCE_CFG_IN_PSM)
static int write_taged_data2psm(psm_handle_t handle,char *strval)
{
    int ret;

    /* Store cfg info to system */
    /* did */
    snprintf_safe(strval,STR_BUF_SZ,"%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x",
            cfg_struct.device_id[0],
            cfg_struct.device_id[1],
            cfg_struct.device_id[2],
            cfg_struct.device_id[3],
            cfg_struct.device_id[4],
            cfg_struct.device_id[5],
            cfg_struct.device_id[6],
            cfg_struct.device_id[7]
            );

    ret = psm_set(&handle,"psm_did",strval);
    if(ret) {
    	LOG_ERROR("Error writing psm did \r\n");
        return -WM_FAIL;
    }

    /* Mac address */
    snprintf_safe(strval,STR_BUF_SZ,"%.2x%.2x%.2x%.2x%.2x%.2x",
            cfg_struct.mac_address[0],
            cfg_struct.mac_address[1],
            cfg_struct.mac_address[2],
            cfg_struct.mac_address[3],
            cfg_struct.mac_address[4],
            cfg_struct.mac_address[5]
            );

    ret = psm_set(&handle, "psm_mac", strval);
    if (ret) {
        LOG_ERROR("Error writing psm_mac\r\n");
        return -WM_FAIL;
    }

    /* key is also binary */
    snprintf_safe(strval,STR_BUF_SZ,"%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x",
            cfg_struct.key[0],
            cfg_struct.key[1],
            cfg_struct.key[2],
            cfg_struct.key[3],
            cfg_struct.key[4],
            cfg_struct.key[5],
            cfg_struct.key[6],
            cfg_struct.key[7],
            cfg_struct.key[8],
            cfg_struct.key[9],
            cfg_struct.key[10],
            cfg_struct.key[11],
            cfg_struct.key[12],
            cfg_struct.key[13],
            cfg_struct.key[14],
            cfg_struct.key[15]
            );
    ret = psm_set(&handle,"psm_key",strval);
    if(ret){
        LOG_ERROR("Error writing key \r\n");
        return -WM_FAIL;
    }

    /* model */
    ret = psm_set(&handle,"psm_model",cfg_struct.model);
    if(ret) {
        LOG_ERROR("Error writing psm model\r\n");
        return -WM_FAIL;
    }

    /* url */
    ret = psm_set(&handle,"psm_url",cfg_struct.url);
    if(ret){
        LOG_ERROR("Error writing psm url\r\n");
        return -WM_FAIL;
    }
    
    /* enauth */
    snprintf_safe(strval,STR_BUF_SZ,"%d",fixed_cfg_data.enauth);
    ret = psm_set(&handle,"enauth",strval);
    if(ret) {
        LOG_ERROR("Error write enauth to psm \r\n");
        return -WM_FAIL;
    }

    return WM_SUCCESS;

}
#endif

static void init_enauth_config(psm_handle_t handle, char *strval)
{
    int inner_enauth, ret;

    ret = psm_get(&handle,"enauth",strval,STR_BUF_SZ);
    if(WM_SUCCESS ==ret) {
        sscanf(strval,"%d",&inner_enauth);
        LOG_DEBUG("enauth in psm is %d \r\n",inner_enauth);
    } else {
        inner_enauth = ENAUTH_DEFAULT_VALUE;
        snprintf_safe(strval,STR_BUF_SZ,"%d",fixed_cfg_data.enauth);
        psm_set(&handle,"enauth",strval);
    }

    if((inner_enauth != 0) && (inner_enauth != 1)) {
        inner_enauth = ENAUTH_DEFAULT_VALUE;
    }

    fixed_cfg_data.enauth = inner_enauth & 0xff;
}

static int read_taged_data_from_psm(psm_handle_t handle,char *strval)
{
    int ret = WM_SUCCESS,length;

    /* will add code to read data from psm & pill cfg_struct*/
    LOG_DEBUG("device id is %s\r\n",strval);
    sscanf(strval,"%02x%02x%02x%02x%02x%02x%02x%02x",
            (unsigned int *)&cfg_struct.device_id[0],
            (unsigned int *)&cfg_struct.device_id[1],
            (unsigned int *)&cfg_struct.device_id[2],
            (unsigned int *)&cfg_struct.device_id[3],
            (unsigned int *)&cfg_struct.device_id[4],
            (unsigned int *)&cfg_struct.device_id[5],
            (unsigned int *)&cfg_struct.device_id[6],
            (unsigned int *)&cfg_struct.device_id[7]
            );
    
    psm_get(&handle,"psm_mac",strval,STR_BUF_SZ);
    length = strlen(strval);
    if(length == 12) {
        sscanf(strval,"%02x%02x%02x%02x%02x%02x",
                (unsigned int *)&cfg_struct.mac_address[0],
                (unsigned int *)&cfg_struct.mac_address[1],
                (unsigned int *)&cfg_struct.mac_address[2],
                (unsigned int *)&cfg_struct.mac_address[3],
                (unsigned int *)&cfg_struct.mac_address[4],
                (unsigned int *)&cfg_struct.mac_address[5]
                );
    } else {
    	LOG_ERROR("wrong mac address \r\n");
    }
    
    ret = psm_get(&handle,"psm_key",strval,STR_BUF_SZ);
    if(ret == 0) {
    	LOG_DEBUG("get key success\r\n");

        sscanf(strval,"%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
                (unsigned int *)&cfg_struct.key[0],
                (unsigned int *)&cfg_struct.key[1],
                (unsigned int *)&cfg_struct.key[2],
                (unsigned int *)&cfg_struct.key[3],
                (unsigned int *)&cfg_struct.key[4],
                (unsigned int *)&cfg_struct.key[5],
                (unsigned int *)&cfg_struct.key[6],
                (unsigned int *)&cfg_struct.key[7],
                (unsigned int *)&cfg_struct.key[8],
                (unsigned int *)&cfg_struct.key[9],
                (unsigned int *)&cfg_struct.key[10],
                (unsigned int *)&cfg_struct.key[11],
                (unsigned int *)&cfg_struct.key[12],
                (unsigned int *)&cfg_struct.key[13],
                (unsigned int *)&cfg_struct.key[14],
                (unsigned int *)&cfg_struct.key[15]
                );

    } else {
    	LOG_ERROR("get key fail \r\n");
    }
    
#if ( defined(CONFIG_CPU_MC200) && defined (CONFIG_WiFi_8801) ) || defined (FORCE_CFG_IN_PSM)
    ret = psm_get(&handle,"psm_url",strval,STR_BUF_SZ);
    if(ret) {
        LOG_DEBUG("get url fail");
    } else {
        strncpy(cfg_struct.url,strval,sizeof(cfg_struct.url));
        LOG_DEBUG("url is %s\r\n",strval);
    }

    ret = psm_get(&handle,"psm_model",strval,STR_BUF_SZ);
    if(!ret) {
        strncpy(cfg_struct.model,strval,sizeof(cfg_struct.model)); //for dhcp use
        LOG_DEBUG("model is %s\r\n",strval);
    }
#endif

    return WM_SUCCESS;
}

bool* get_read_otp_flag()
{
    static bool read_otp_flag = false;
    
    return &read_otp_flag;
}

void set_read_otp_flag(bool flag)
{
    bool* ret = get_read_otp_flag();
    
    * ret = flag;
}

void store_or_retrive_taged_config(void )
{
    int ret;
    psm_handle_t handle;

    //set default uid value
    runtime_config.smt_cfg_uid = 0;

    /* Open handle for specific PSM module name */
    ret = psm_open(&handle, "ot_config");
    if (ret) {
        LOG_ERROR("Error opening PSM module\r\n");
        goto store_error;
    }

    char * strval = malloc(STR_BUF_SZ); 
    if(!strval){
        goto store_error2;
    }

    /* Check is did exist ? if it exist all exist, if it not all not */
    ret = psm_get(&handle, "psm_did", strval, STR_BUF_SZ);
    if (ret) {
        LOG_DEBUG("first boot No did value found.\r\n");
        
        //for mc200+8801 do store operation
#if ( defined(CONFIG_CPU_MC200) && defined (CONFIG_WiFi_8801) ) || defined (FORCE_CFG_IN_PSM)
        ret = write_taged_data2psm(handle,strval);
#else 
        //for mw300 or mc200+8777, flags that to read OTP
        ret = WM_SUCCESS;

        set_read_otp_flag(true);

#endif
    } else {
        LOG_DEBUG("cfg data has already write \r\n");

        read_taged_data_from_psm(handle,strval);
        
        //if read mac from flash, set it to system
        wlan_set_mac_addr(get_wlan_mac_address());
    }

    //init enauth config from psm
    init_enauth_config(handle,strval);

    if(strval) {
        free(strval);
    }

    psm_close(&handle);
    
    //this statement is for ret = write_taged_data2psm(handle,strval);
    if(ret) { 
        goto store_error;
    }

    return;

store_error2:
    psm_close(&handle);

store_error:
    pm_reboot_soc(); //error occure on main thread , reboot

}

void WEAK product_set_wifi_cal_data(void)
{
    
}

static addon_info_t app_addon_info;
static addon_info_t user_addon_info;
void init_addon_interface(void)
{
    /****************Init xobject list*********************/
    // containing miio general purpose functions
    memset(&app_addon_info, 0, sizeof(addon_info_t));
    strcpy(app_addon_info.xobj.name, ADDX_BASE_NAME);
    app_addon_info.xobj.next = NULL;

    XOBJ_SET_TYPE(&app_addon_info.xobj, XOBJ_FLAG_TYPE_ADDLIB);

    app_addon_info.fsym_tab = ADDLIB_MAP_FSYMB_TAB_ADDR;
    app_addon_info.vsym_tab = ADDLIB_MAP_VSYMB_TAB_ADDR;
    app_addon_info.fsym_num = ADDLIB_MAP_FSYMB_TAB_SIZE / sizeof(sym_fw_t);
    app_addon_info.vsym_num = ADDLIB_MAP_VSYMB_TAB_SIZE / sizeof(sym_fw_t);

    xobj_sys_init(&app_addon_info.xobj);

    // containing user defined functions
    memset(&user_addon_info, 0, sizeof(addon_info_t));
    strcpy(user_addon_info.xobj.name, USER_ADDLIB_NAME);
    user_addon_info.xobj.next = NULL;

    XOBJ_SET_TYPE(&user_addon_info.xobj, XOBJ_FLAG_TYPE_ADDLIB);

    user_addon_info.fsym_tab = ADDLIB_MAP_USER_FSYMB_TAB_ADDR;
    user_addon_info.vsym_tab = ADDLIB_MAP_USER_VSYMB_TAB_ADDR;
    user_addon_info.fsym_num = ADDLIB_MAP_USER_FSYMB_TAB_SIZE / sizeof(sym_fw_t);
    user_addon_info.vsym_num = ADDLIB_MAP_USER_VSYMB_TAB_SIZE / sizeof(sym_fw_t);

    xobj_append(&user_addon_info.xobj);
    /*end*/

    LOG_DEBUG("xobj sys init complete\r\n");
}


//Get ip in host-endian
void netif_refresh(u32_t* local_if_mask, u32_t* local_if_netid, u32_t* uap_if_mask, u32_t* uap_if_netid)
{
	struct wlan_ip_config ip_config;
	if(WLAN_ERROR_NONE == wlan_get_address(&ip_config)){
		if(local_if_mask)*local_if_mask = arch_net_ntohl(ip_config.ipv4.netmask);
		if(local_if_netid)*local_if_netid = arch_net_ntohl(ip_config.ipv4.address & ip_config.ipv4.netmask);
	}else{
		if(local_if_mask)*local_if_mask = 0;
		if(local_if_netid)*local_if_netid = 0;
	}

	if(WLAN_ERROR_NONE == wlan_get_uap_address(&ip_config)){
		if(uap_if_mask)*uap_if_mask = arch_net_ntohl(ip_config.ipv4.netmask);
		if(uap_if_netid)*uap_if_netid = arch_net_ntohl(ip_config.ipv4.address & ip_config.ipv4.netmask);
	}else{
		if(uap_if_mask)*uap_if_mask = 0;
		if(uap_if_netid)*uap_if_netid = 0;
	}
}

/* used to schedule message queue */
os_queue_t main_thread_sched_queue;
static os_queue_pool_define(main_queue_data, 2 * sizeof(void*));

static int handle_smart_config(void)
{
    int ret;

    /* create main thread queue */
    ret = os_queue_create(&main_thread_sched_queue, "main_queue", sizeof(void *), &main_queue_data);
    if (ret) {
        LOG_ERROR("create main queue fail should restart ");
        goto handle_smt_cfg_err;
    }

    void *msg = NULL;
    // main thread loop
    while (1) {

        ret = os_queue_recv(&main_thread_sched_queue, &msg, OS_WAIT_FOREVER);
        if (ret != WM_SUCCESS) {
            LOG_ERROR("get mesage error on main thread");
            continue;
        }

        if (SCAN_ROUTER == (main_loop_message_type) msg) {
#if defined(ENABLE_AIRKISS) && (ENABLE_AIRKISS == 1)
            airkiss_config_with_sniffer(construct_ssid_for_smt_cfg());
#endif
        } else if (NB_OPERATION == (main_loop_message_type) msg) {
            break;
        } 

	}

    //release memory
    os_queue_delete(&main_thread_sched_queue);

    //double check 
    if (NB_OPERATION == (main_loop_message_type) msg)
        return WM_SUCCESS;

handle_smt_cfg_err:
    pm_reboot_soc();
    return -1;
}

void xiaomi_app_hardfault_handler(unsigned long *pmsp, unsigned long *ppsp, SCB_Type *scb, const char *task_name)
{
    save_crash_trace(pmsp, ppsp, scb, task_name);
    pm_reboot_soc();
}

void main_loop_run()
{
    int ret;

    wifi_set_packet_retry_count(10);
    //control smart config procedure
    ret = handle_smart_config();
    if(ret == WM_SUCCESS) {
        //used for lock delayed_task_list
        ret = init_delayed_task_mutex();
        if(WM_SUCCESS != ret) {
            LOG_ERROR("init delayed_task_mutex fail \r\n");
            return;
        }

        ret = init_worker_thread_queue();
        if(WM_SUCCESS != ret) {
            LOG_ERROR("main loop error exit");
            return;
        }

        //should only active otd, provisioned by uap
        if(ENABLE_UAP == get_trigger_uap_info()) {
            set_trigger_uap_info(DISABLE_UAP);
        }

        if(is_uap_started()) {
            LOG_DEBUG("uap started so delay 60s stop uap \r\n");
            //delay 60s to stop uap
            call_delayed_task_on_worker_thread(app_uap_stop,NULL,60000,0);
        }

        //otd started,function test pass
        if (is_factory_mode_enable())
            wmprintf(FACTORY_FNTEST_PASS);

        call_on_worker_thread(crash_assert_check,NULL,0);

        if(is_ap_info_change()) {
            LOG_DEBUG("AP info changed so report \r\n");

            //delay 5s to wait ot online
            call_delayed_task_on_worker_thread(report_ap_info_change_event,NULL,5000,0);
        }

        //start worker thread
        worker_thread_loop(NULL);

    } else {
        LOG_ERROR("main thread exit unexpectively ");
    }
}

#define MIIO_SCAN_AP_NUM 10
//struct wlan_scan_result miio_scan_res[MIIO_SCAN_AP_NUM];
//int miio_scan_num = 0;

int miio_scan_cb(unsigned int count)
{
	int i, n, size;
	struct wlan_scan_result res;

#define DESC_SIZE_PER_AP sizeof("[\"ssid\":\"012345678901234567890123456789012\",\"bssid\":\"ABCDEF012345\",\"channel\":12,\"rssi\":-20], ") 

	count = MIN(count, MIIO_SCAN_AP_NUM);
	size = 100+count*DESC_SIZE_PER_AP;
	char *buf = malloc(size);
	n = 0;

	n += snprintf_safe(buf + n, size - n, "{\"method\":\"_otc.scan\", \"param\":[");

	for (i = 0; i < count; i++) {
		wlan_get_scan_result(i, &res);
        	//dump_scan_result_info(&res);
		n += snprintf_safe(buf + n, size - n, "{\"ssid\":\"%s\"", res.ssid);
		n += snprintf_safe(buf + n, size - n, ",\"bssid\":\"");
   		n += snprintf_hex(buf + n, size - n, (const uint8_t *) res.bssid, 6, 0x80 | ':');
		n += snprintf_safe(buf + n, size - n, "\",\"channel\":%u", res.channel);
		n += snprintf_safe(buf + n, size - n, ",\"rssi\":%u},", res.rssi);
	}
	if(i)n--;	//remove last ,
	n += snprintf_safe(buf + n, size - n, "]}");

	ot_api_method(buf, n, NULL, NULL);
	free(buf);

	return 0;
}

static char ssid_name_strval[32];
char * construct_ssid_for_smt_cfg()
{
    if (strlen(cfg_struct.model) > MODEL_MAX_SIZE) {
        LOG_WARN("model size too large (must less than 23 bytes)");
    }

    strncpy(ssid_name_strval, cfg_struct.model, MODEL_MAX_SIZE);

    char *c = ssid_name_strval;
    while (*c) {
        if ('.' == *c) {
            *c = '-';
        }
        c++;
    }

    strcat(ssid_name_strval, "_miap");
    snprintf_hex(ssid_name_strval + strlen(ssid_name_strval), sizeof(ssid_name_strval) - strlen(ssid_name_strval),
            (const uint8_t *) cfg_struct.mac_address + 4, 2, 0);
    LOG_DEBUG("smart cfg ssid is %s\r\n",ssid_name_strval);

    return ssid_name_strval;
}




void hexdump(char* buf, int size, u32_t offset)
{
	char skip_flag = 0;
	char* ptr;
	u32_t ofs = offset % 16;
	ptr = buf - ofs;

	while(ptr < buf + size){
		int i;
		char NotAll0 = 0;


		//下面判断是否全零，若是跳过
		//check if all zero's
		for(i = 0; i < 16; i++)
			NotAll0 |= ptr[i];

		//如果all 0, 并且处在skip
		if(!NotAll0 && skip_flag){
			goto skip;			
		}

		if(!NotAll0)
			skip_flag = 1;
		else{
			if(skip_flag)
				wmprintf("\t\t...all zeros...\r\n");
			skip_flag = 0;
		}

		//打印地址
		wmprintf("0x%04x  ", (unsigned short)(offset + ptr - buf));	//addr

		//打印十六进制
		for(i = 0; i < 16; i++){
			if((&(ptr[i]) >= buf + size) || (&(ptr[i]) < buf))
	    			wmprintf("   ");
	         	else
	    			wmprintf("%02X ",(ptr[i]&0xFF));

			if(i == 7)wmprintf(" ");
		}

		wmprintf("  ");

		//打印ASCII
		for(i = 0; i < 16; i++){
			if((&(ptr[i]) >= buf + size) || (&(ptr[i]) < buf))
				wmprintf(" ");
			else
				wmprintf("%c",(ptr[i]>30 && ptr[i]<127)?ptr[i]:'.');
		}

		wmprintf("\n\r");

skip:
		ptr += 16;
	}

}


#ifndef DELETET_THIS_OT_METHOD_WITH_DID_API_TEST


/*
test flow:

设备A
ip:192.168.1.102
did:what ever
token:what ever

设备B:
ip:192.168.1.101
did:1384053
token:dd76c14aa87c8f2edba4ca1815c6eec9


1、获取设备B信息，如上: ip did token；(可以通过串口log查看)
2、在云端后台，向设备攻 发送:

method: "_otd.neighbor"
param: [
	{"ip":"192.168.1.101","did":1384053,"token":"dd76c14aa87c8f2edba4ca1815c6eec9"}, 
//注意，可以多放几个item
	{"ip":"192.168.1.109","did":12354,"token":"1278ABEF1278ABEF1278ABEF1278ABEF"}]

这样设备攻 便有了设备B的权限，可以在log中看到其会主动访问对方，以确定信息有效；


3、在云端后台，向设备攻 发送:

method: "miIO.rpc_test"	//该rpc仅用于测试，验证后可以删除
params: 1384053

可以看到 设备攻 向 设备B 发送了 标准的 info 指令，可以看到通讯OK


*/


static int rpc_test_ack(jsmn_node_t* pjn, void* ctx)
{
	//若是error，则解析提示
	if(pjn && pjn->js &&  NULL == pjn->tkn){
		LOG_DEBUG("otu:rpc_test err:%s.\r\n", pjn->js);
	} 

	//得到回复
	else{
		LOG_DEBUG("neighbor ret:\r\n %s \r\n", pjn->js);
	}

	return STATE_OK;
}


/*
 * {"method":"miIO.rpc_test","params":did}
 */
int JSON_DELEGATE(rpc_test)(jsmn_node_t* pjn, fp_json_delegate_ack ack, void* ctx)
{
	int ret = STATE_OK;

//	const char* js = "{\"method\":\"miIO.info\",\"params\":{}}";
	const char* js = "{\"method\":\"miIO.rpc_from\",\"params\":{}}";

	u64_t did64;
	if(STATE_OK == jsmn_tkn2val_u64(pjn->js, pjn->tkn, &did64)){

		//向邻居发info请求
		ret = ot_api_method_with_did(did64, js, strlen(js), rpc_test_ack, NULL);		// <--------- 关键语句

		LOG_DEBUG("rpc issued, ret:%d\r\n", ret);
		json_delegate_ack_result(ack, ctx, ret);
	}
	else{
		LOG_ERROR("err: param must be did.\r\n");
		json_delegate_ack_err(ack, ctx, -1, "param must be did");
	}


	return ret;
}



FSYM_EXPORT(JSON_DELEGATE(rpc_test));


#endif






#define DELETET_THIS_OT_DNLD_TEST
#ifndef DELETET_THIS_OT_DNLD_TEST


struct test_ctx{
	size_t cnt;
};


int test_open(stream_ctx_t* ctx, void* id)
{
	struct test_ctx* c = (struct test_ctx*)ctx;
	c->cnt = 0;
	LOG_INFO("test:Open.\r\n");
	return STATE_OK;
}
int test_write(stream_ctx_t* ctx, u8_t* in, size_t len)
{
	struct test_ctx* c = (struct test_ctx*)ctx;
	c->cnt += len;
	LOG_INFO("test:Write %u.\r\n", len);
	return STATE_OK;
}

int test_close(stream_ctx_t* ctx)
{
	LOG_INFO("test:Close.\r\n");
	free(ctx);
	return STATE_OK;
}
int test_error(stream_ctx_t* ctx, void* ret)
{
	LOG_INFO("test:Error:%s.\r\n", ret);
	free(ctx);
	return STATE_OK;
}

stream_in_if_t in_if = {
	//在DNS完成，连接建立，头部正确，且拿到数据时被调用。
	//返回 < 0，将断开连接恢复IDLE状态；其不会其他回调，使用者自己发现问题自己现场解决；
	.open = test_open,

	//在连接建立，头部正确，且拿到数据时，调用open之后被调用。
	//返回 < 0，将认定放弃下载，断开连接恢复IDLE状态；其不会其他回调，使用者自己发现问题自己现场解决；
	.write = test_write,

	//在下载完成时被调用，之后断开连接恢复IDLE状态；
	.close = test_close,

	//在调用open之前(DNS完成，建立连接，检查头部，拿到数据)任何的错误都会导致error，将放弃下载，断开连接恢复IDLE状态
	.error = test_error,
};


/*
 * {"method":"miIO.test_dnld","params":{"url":xxx}}
 */
 
int JSON_DELEGATE(test_dnld)(jsmn_node_t* pjn, fp_json_delegate_ack ack, void* ctx)
{
	int ret = STATE_OK;

	jsmntok_t* url_tk = jsmn_key_value(pjn->js, pjn->tkn, "url");

	size_t ulen = url_tk->end - url_tk->start;
	char* purl = malloc(ulen + 4);

	ulen = str_unescape_to_buf(pjn->js + url_tk->start, ulen, purl, ulen);
	LOG_INFO("httpc:get:%s.\r\n", purl);


	u8_t md5_val[16], *pmd5;
	if(STATE_OK ==	jsmn_key2val_xbuf(pjn->js, pjn->tkn, "md5", md5_val, sizeof(md5_val), NULL)){	//可选
		pmd5 = md5_val;
		LOG_INFO("httpc:need md5.\r\n");
	}
	else
		pmd5 = NULL;


	struct test_ctx* c = malloc(sizeof(struct test_ctx));

	ret = ot_httpc_start(purl, pmd5, &in_if, c);

	//If dnld start ok, this will be freed in callbacks. Or you'll free by yourself
	if(STATE_OK != ret)free(c);

	free(purl);
	json_delegate_ack_result(ack, ctx, ret);

	return ret;
}



FSYM_EXPORT(JSON_DELEGATE(test_dnld));




#endif






#define DELETET_THIS_OT_DELEGATE_QUERY_FROM_TEST
#ifndef DELETET_THIS_OT_DELEGATE_QUERY_FROM_TEST

/*
 * {"method":"miIO.rpc_from","params":{}}
 */
int JSON_DELEGATE(rpc_from)(jsmn_node_t* pjn, fp_json_delegate_ack ack, void* ctx)
{
	int n = 0, ret = STATE_OK;
	size_t flen = 0;
	char proto, is_local;
	char from[128];


	n += snprintf_safe(from + n, sizeof(from) - n, "{\"result\":\"from:");
	ret = ot_delegate_ctx_query(ctx, &proto, &is_local, from + n, sizeof(from) - n, &flen);
	if(ret == STATE_OK)
		n += flen;
	n += snprintf_safe(from + n, sizeof(from) - n, ",proto:'%c',is_local:'%c'\"}", proto, is_local);
	LOG_INFO("DLGT_QUERY_RET:%s.\r\n", from);




	if(ack){
		jsmn_node_t jn;
		jn.js_len = n;
		jn.js = from;
		jn.tkn = NULL;
		return ack(&jn, ctx);
	}

	return ret;
}



FSYM_EXPORT(JSON_DELEGATE(rpc_from));



#endif

