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

#include <pwrmgr.h>
#include <rfget.h>
#include <partition.h>
#include <psm.h>
#include <board.h>
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

#define URL_BUF_LEN 256

extern const int MIIO_APP_VERSION;

/************ assist function ************/
int net_info(char*buf, int size, char* key, char* post)
{
    int n = 0;
    net_addr_t addr;
    net_addr_t gw;
    net_addr_t mask;
    u8_t gw_mac[6];

    struct wlan_ip_config ip_config;
    wlan_get_address(&ip_config);

    addr.s_addr = ip_config.ipv4.address;
    gw.s_addr = ip_config.ipv4.gw;
    mask.s_addr = ip_config.ipv4.netmask;
    int ret= arch_net_get_gw_mac(gw, gw_mac);

    if (key)
        n += snprintf_safe(buf + n, size - n, "\"%s\":", key);

    n += snprintf_safe(buf + n, size - n, "{\"localIp\":\"");
    n += arch_net_ntoa_buf((net_addr_t*) &(addr), buf + n);
    n += snprintf_safe(buf + n, size - n, "\",\"mask\":\"");
    n += arch_net_ntoa_buf((net_addr_t*) &(mask), buf + n);
    n += snprintf_safe(buf + n, size - n, "\",\"gw\":\"");
    n += arch_net_ntoa_buf((net_addr_t*) &(gw), buf + n);
    n += snprintf_safe(buf + n, size - n, "\",\"gw_mac\":\"");
    if(STATE_OK == ret)
    	n += snprintf_hex(buf + n, size - n, (const uint8_t *) gw_mac, 6, 0x80 | ':');

    n += snprintf_safe(buf + n, size - n, "\"}");

    if (post)
        n += snprintf_safe(buf + n, size - n, "%s", post);
    return n;
}

int mac_info(char*buf, int size, char* key, char* post)
{
    int n = 0;
    if (key)
        n += snprintf_safe(buf + n, size - n, "\"%s\":\"", key);

    n += snprintf_hex(buf + n, size - n, (const uint8_t *) cfg_struct.mac_address, 6, 0x80 | ':');
    buf[n++] = '"';

    if (post)
        n += snprintf_safe(buf + n, size - n, "%s", post);

    return n;

}

int model_info(char*buf, int size, char* key, char* post)
{
    int n = 0;

    if (key)
        n += snprintf_safe(buf + n, size - n, "\"%s\":", key);

    n += snprintf_safe(buf + n, size - n, "\"%s\"", cfg_struct.model);

    if (post)
        n += snprintf_safe(buf + n, size - n, "%s", post);
    return n;
}

/**********************************
 // " ---> \"
 // \ ---> \\
// / ---> \/
 //  ---> \b
 //  ---> \f
 //  ---> \n
 //  ---> \r
 //  ---> \t
 //  ---> \u
 ************************************/
int ssid_escape(char in[32], char out[])
{
    char *pin, *pout;

    pin = in;
    pout = out;

    while (*pin && pin - in < 32) {
        switch (*pin) {
        case '"':
        case '\\':
        case '/':
            *pout++ = '\\';
            break;
        default:
            break;
        }
        *pout++ = *pin++;
    }

    *pout = '\0';
    return pout - out;
}

#ifdef SSID_ESCAPE_TEST

#include <stdio.h>
#include <conio.h>

int main(int argc, char **argv)
{
    char *ssid = "cscsc\"sds\\sds\/ds";
    char ssid_[64];

    printf("%d, %s\n", ssid_escape(ssid, ssid_), ssid_);

    getch();

    return 0;
}

#endif

static uint32_t start_package_cap_time = 0;
void set_start_package_cap_time(uint32_t time)
{
    start_package_cap_time = time;
}

int ap_info(char*buf, int size, char* key, char* post)
{
    int n = 0;
    short rssi;
    char _ssid[64];

    wlan_get_current_rssi(&rssi);

    struct wlan_network wlannetwork;
    wlan_get_current_network(&wlannetwork);

    ssid_escape(wlannetwork.ssid, _ssid);

//	LOG_DEBUG("del_me: AP_INFO:%s.\r\n", _ssid);

    if (key)
        n += snprintf_safe(buf + n, size - n, "\"%s\":", key);

    n += snprintf_safe(buf + n, size - n, "{\"rssi\":%hd,\"ssid\":\"%s\",\"bssid\":\"", rssi, _ssid);
    n += snprintf_hex(buf + n, size - n, (const uint8_t *) wlannetwork.bssid, 6, 0x80 | ':');
    n += snprintf_safe(buf + n, size - n, "\"}");

    if (post)
        n += snprintf_safe(buf + n, size - n, "%s", post);
    return n;
}

int ver_info(char*buf, int size, char* key, char* post)
{
    int n = 0;

    if (key)
        n += snprintf_safe(buf + n, size - n, "\"%s\":", key);

    char miio_sdk_ver[16];
    get_miio_sdk_ver(miio_sdk_ver, sizeof(miio_sdk_ver));

    n += snprintf_safe(buf + n, size - n, "\"%s", miio_sdk_ver);

    if(MIIO_APP_VERSION > 0)
        n += snprintf_safe(buf + n, size-n, "_%u", MIIO_APP_VERSION);

    n += snprintf_safe(buf + n, size-n, "\"");

    if (post)
        n += snprintf_safe(buf + n, size - n, "%s", post);
    return n;
}

int mcu_ver_info(char*buf, int size, char* key, char* post)
{
    int n = 0;

    if (strlen(mcu_fw_version) > 0) {
        if (key)
            n += snprintf_safe(buf + n, size - n, "\"%s\":", key);
        n += snprintf_safe(buf + n, size - n, "\"%s\"", mcu_fw_version);
        if (post)
            n += snprintf_safe(buf + n, size - n, "%s", post);
    }
    return n;
}

int wifi_ver_info(char*buf, int size, char* key, char* post)
{
    int n = 0, ret;
    wifi_fw_version_t wifi_version;

    ret = wifi_get_firmware_version(&wifi_version);
    if (WM_SUCCESS != ret) {
        LOG_ERROR("get wifi version failed\r\n");
        return 0;
    }

    if (key)
        n += snprintf_safe(buf + n, size - n, "\"%s\":", key);
    n += snprintf_safe(buf + n, size - n, "\"%s\"", wifi_version.version_str);
    if (post)
        n += snprintf_safe(buf + n, size - n, "%s", post);
    return n;
}


uint32_t miio_dev_info(char * buf, uint32_t buf_size, const char * post)
{
    uint32_t n = 0;

    n += snprintf_safe(buf + n, buf_size - n, "\"life\":%lu,", arch_os_time_now());
    n += snprintf_safe(buf + n, buf_size - n, "\"cfg_time\":%lu,", start_package_cap_time);

    n += snprintf_safe(buf + n, buf_size - n, "\"token\":\"");
    n += snprintf_hex(buf + n, buf_size - n, token, sizeof(token), 0);
    n += snprintf_safe(buf + n, buf_size - n, "\",");

    n += mac_info(buf + n, buf_size - n, "mac", ",");
    n += ver_info(buf + n, buf_size - n, "fw_ver", ",");
#if defined(CONFIG_CPU_MW300)
    n += snprintf_safe(buf + n, buf_size - n, "\"hw_ver\":\"%s\",", "MW300");
#elif defined(CONFIG_CPU_MC200) 
    n += snprintf_safe(buf + n, buf_size - n, "\"hw_ver\":\"%s\",", "MC200");
#else 
#error not supported platform
#endif

    uint32_t smt_cfg_uid = get_smt_cfg_uid_in_ram();
    if (0 != smt_cfg_uid) {
        n += snprintf_safe(buf + n, buf_size - n, "\"uid\":%lu,", smt_cfg_uid);
    }

    n += model_info(buf + n, buf_size - n, "model", ",");
    n += mcu_ver_info(buf + n, buf_size - n, "mcu_fw_ver", ",");
    n += wifi_ver_info(buf + n, buf_size - n, "wifi_fw_ver", ",");
    n += ap_info(buf + n, buf_size - n, "ap", ",");
    n += net_info(buf + n, buf_size - n, "netif", ",");

    n += snprintf_safe(buf + n, buf_size - n, "\"mmfree\":%u", xPortGetFreeHeapSize());


    const char* s = otn_is_online();
    if(s)
        n += snprintf_safe(buf + n, buf_size - n, ",\"ot\":\"%s\"", s);

    if(post)
        n += snprintf_safe(buf + n, buf_size - n, "%s", post);

    return n;
}






int JSON_DELEGATE(info)(jsmn_node_t* pjn, fp_json_delegate_ack ack, void* ctx)
{
    char *js_buf = (char *) malloc(OT_PACKET_SIZE_MAX);

    if (js_buf) {
        u32_t n = 0;


	n += snprintf_safe(js_buf + n, OT_PACKET_SIZE_MAX - n, "{\"result\":{");
	n += miio_dev_info(js_buf + n, OT_PACKET_SIZE_MAX - n, ",");
	n += otu_stat(js_buf + n, OT_PACKET_SIZE_MAX - n, ",");
	n += ott_stat(js_buf + n, OT_PACKET_SIZE_MAX - n, "}}");



        jsmn_node_t jsn = { js_buf, n, NULL };

        if (ack)
            ack(&jsn, ctx);

        free(js_buf);
    } else
        json_delegate_ack_err(ack, ctx, STATE_NOMEM, "Mem error.");



    return STATE_OK;
}

static d0_ctx_call_ret_t ot_restore(struct d0_loop*l, struct d0_invoke* invk)
{
    if(!is_ota_ongoing())
        app_reset_saved_network();
    return D0_INVOKE_RET_VAL_RELEASE;
}

static d0_ctx_call_ret_t restore_to_uap(struct d0_loop*l, struct d0_invoke* invk)
{
    if(!is_ota_ongoing()) {
        set_trigger_uap_info(ENABLE_UAP);

        if(is_provisioned()) {
            app_reset_saved_network();
        } else {
            pm_reboot_soc();
        }
    }
	return D0_INVOKE_RET_VAL_RELEASE;

}

/*
 * {"method":"miIO.restore","params":{"enable_uap":true}}
 */
int JSON_DELEGATE(restore)(jsmn_node_t* pjn, fp_json_delegate_ack ack, void* ctx)
{
	json_delegate_ack_result(ack, ctx, STATE_OK);
   
	uint8_t value = 0;


    if (STATE_OK == jsmn_key2val_bool(pjn->js, pjn->tkn, "enable_uap", &value)) {
        if(TRUE == value) {
            d0_invoke_start(d0_invoke_loop_alloc(d0_api_find(OTN_XOBJ_NAME_DEFAULT), restore_to_uap, 0), NULL);
            return STATE_OK;
        }    
    }

    d0_invoke_start(d0_invoke_loop_alloc(d0_api_find(OTN_XOBJ_NAME_DEFAULT), ot_restore, 0), NULL);
    return STATE_OK;
}

/*
 * {"method":"miIO.config","params":{"enauth":1}}
 */
int JSON_DELEGATE(config)(jsmn_node_t* pjn, fp_json_delegate_ack ack, void* ctx)
{
    int ret = STATE_OK;
    uint8_t set_value;

    ret = jsmn_key2val_u8(pjn->js, pjn->tkn, "enauth", &set_value);
    if (STATE_OK != ret) {
        json_delegate_ack_err(ack, ctx, OT_ERR_CODE_PARSE, OT_ERR_INFO_PARSE);
        goto config_error_handle;
    }

    LOG_DEBUG("set_value is %u \r\n",set_value);

    if ((set_value != 0) && (set_value != 1)) {
        ret = STATE_ERROR;
        json_delegate_ack_err(ack, ctx, OT_ERR_CODE_PARAM_INVALID, OT_ERR_INFO_PARAM_INVALID);
        goto config_error_handle;
    }


	struct otn* loop_ot = (struct otn*)d0_api_find(OTN_XOBJ_NAME_DEFAULT);
	if(NULL == loop_ot){
        ret = STATE_ERROR;
        json_delegate_ack_err(ack, ctx, OT_ERR_CODE_SERVICE_NOT_AVAILABLE, OT_ERR_INFO_SERVICE_NOT_AVAILABLE);
        LOG_ERROR("not find otd");
        goto config_error_handle;
    }
	
	udp_ot_t* otu = loop_ot->otu;
	u8_t ot_enauth = BIT_IS_SET(otu->ot_flag, OTU_FLAG_ENAUTH) ? 1 : 0;
	set_value = set_value ? 1 : 0;

    if (set_value != ot_enauth) {
        LOG_DEBUG("will change enauth config");

        if (set_value)
            BIT_SET(otu->ot_flag, OTU_FLAG_ENAUTH)
        else
            BIT_CLEAR(otu->ot_flag, OTU_FLAG_ENAUTH)

        set_enauth_data_in_ram(set_value);
        //change enauth data in psm
        set_enauth_data_in_psm(set_value);
    }

    json_delegate_ack_result(ack, ctx, ret);

    config_error_handle: return ret;
}

static void cmd_reboot(int argc, char **argv)
{
    if(!is_ota_ongoing())
        pm_reboot_soc();
}

static void cmd_restore(int argc, char **argv)
{
    if(!is_ota_ongoing()) {
        if(is_provisioned()) {
            app_reset_saved_network();
        } 
    }
}

static void cmd_updatefw(int argc, char **argv)
{
    if (argc != 2) {
        wmprintf("Usage: updatefw <url_str>\r\n");
        return;
    }

    led_enter_ota_mode();
    if(!is_ota_ongoing())
        update_app_fw(argv[1]);
    led_exit_ota_mode();
}

static void cmd_updatewififw(int argc, char **argv)
{
    if (argc != 2) {
        wmprintf("Usage: updatewififw <url_str>\r\n");
        return;
    }

    led_enter_ota_mode();
    if(!is_ota_ongoing())
        update_wifi_fw(argv[1]);
    led_exit_ota_mode();
}

static void cmd_print_partition(int argc, char **argv)
{
    short index = 0;
    struct partition_entry *p1 = part_get_layout_by_id(FC_COMP_FW, &index);
    struct partition_entry *p2 = part_get_layout_by_id(FC_COMP_FW, &index);

    print_partition_table();
    print_partition_info(p1);
    print_partition_info(p2);
}

static void cmd_print_versions(int argc, char **argv)
{
    print_versions();
}

static struct cli_command miio_chip_rpc_commands[] = {
        { "reboot", NULL, cmd_reboot },
        { "restore", NULL, cmd_restore },
        { "updatefw", "<http_url>", cmd_updatefw },
        { "updatewififw", "<http_url>", cmd_updatewififw },
        { "pp", "print partition info", cmd_print_partition },
        { "pv", "print version", cmd_print_versions },
};

int miio_chip_rpc_init(void)
{
    return cli_register_commands(miio_chip_rpc_commands, sizeof(miio_chip_rpc_commands) / sizeof(struct cli_command));
}



#ifndef RELEASE
/*
 * {"method":"miIO.psm_set","params":{"module":"ot_config","key":"psm_model","value":"xiaomi.dev.v1"}}
 */
int JSON_DELEGATE(psm_set)(jsmn_node_t* pjn, fp_json_delegate_ack ack, void* ctx)
{
    int ret = STATE_OK;
    char module[32];
    char key[32];
    char value[128];
    psm_handle_t handle;

    memset(module, 0, sizeof(module));
    memset(key, 0, sizeof(key));
    memset(value, 0, sizeof(value));

    ret = jsmn_key2val_str(pjn->js, pjn->tkn, "module", module, sizeof(module), NULL);
    if (STATE_OK != ret) {
        goto err_exit;
    }

    ret = jsmn_key2val_str(pjn->js, pjn->tkn, "key", key, sizeof(key), NULL);
    if (STATE_OK != ret) {
        goto err_exit;
    }

    ret = jsmn_key2val_str(pjn->js, pjn->tkn, "value", value, sizeof(value), NULL);
    if (STATE_OK != ret) {
        goto err_exit;
    }

    LOG_INFO("psm-set %s.%s=%s\r\n", module, key, value);

    ret = psm_open(&handle, module);
    if (WM_SUCCESS != ret) {
        LOG_ERROR("open psm module %s error\r\n", module);
        goto err_exit;
    }

    ret = psm_set(&handle, key, value);
    if (WM_SUCCESS != ret) {
        LOG_ERROR("psm set error, %s=%s\r\n", key, value);
        psm_close(&handle);
        goto err_exit;
    }

    psm_close(&handle);

    json_delegate_ack_result(ack, ctx, STATE_OK);
    return STATE_OK;

err_exit:
    json_delegate_ack_err(ack, ctx, OT_ERR_CODE_PARAM_INVALID, OT_ERR_INFO_PARAM_INVALID);
    return STATE_ERROR;
}

/*
 * {"method":"miIO.psm_get","params":{"module":"ot_config","key":"psm_model"}}
 */
int JSON_DELEGATE(psm_get)(jsmn_node_t* pjn, fp_json_delegate_ack ack, void* ctx)
{
    int ret = STATE_OK;
    char module[32];
    char key[32];
    char value[128];
    psm_handle_t handle;
    jsmn_node_t jn;
    char js[128];

    memset(module, 0, sizeof(module));
    memset(key, 0, sizeof(key));
    memset(value, 0, sizeof(value));

    ret = jsmn_key2val_str(pjn->js, pjn->tkn, "module", module, sizeof(module), NULL);
    if (STATE_OK != ret) {
        goto err_exit;
    }

    ret = jsmn_key2val_str(pjn->js, pjn->tkn, "key", key, sizeof(key), NULL);
    if (STATE_OK != ret) {
        goto err_exit;
    }

    LOG_INFO("psm-get %s.%s\r\n", module, key);

    ret = psm_open(&handle, module);
    if (WM_SUCCESS != ret) {
        LOG_ERROR("open psm module %s error\r\n", module);
        goto err_exit;
    }

    ret = psm_get(&handle, key, value, sizeof(value));
    if (WM_SUCCESS != ret) {
        LOG_ERROR("psm_get error, %s\r\n", key);
        psm_close(&handle);
        goto err_exit;
    }

    psm_close(&handle);

    jn.js = js;
    jn.js_len = snprintf_safe(js, sizeof(js), "{\"result\":[\"%s\"]}", value);
    jn.tkn = NULL;

    ack(&jn, ctx);

    return STATE_OK;

err_exit:
    json_delegate_ack_err(ack, ctx, OT_ERR_CODE_PARAM_INVALID, OT_ERR_INFO_PARAM_INVALID);
    return STATE_ERROR;
}

/*
 * {"method":"miIO.cli","params":["cmd1","cmd2 arg arg","cmd3 arg"]}
 */
int JSON_DELEGATE(cli)(jsmn_node_t* pjn, fp_json_delegate_ack ack, void* ctx)
{
    jsmntok_t *tkn = NULL;
    char *cmd = NULL;
    size_t cmd_sz = 0;
    uint32_t i = 0;

    while (NULL != (tkn = jsmn_array_value(pjn->js, pjn->tkn, i))) {

        int retry_count = 0;
        while(WM_SUCCESS != cli_get_cmd_buffer(&cmd)) {
            api_os_tick_sleep(1);
            retry_count++;
            if(retry_count > 500) {
                json_delegate_ack_err(ack, ctx, OT_ERR_CODE_PARAM_INVALID, OT_ERR_INFO_PARAM_INVALID);
                return STATE_ERROR;
            }
        }

        jsmn_tkn2val_str(pjn->js, tkn, cmd, 256, &cmd_sz);
        LOG_INFO("command #%d: %s\r\n", i, cmd);
        i++;
        cli_submit_cmd_buffer(&cmd);
        cmd = NULL;
    }

    json_delegate_ack_result(ack, ctx, STATE_OK);
    return STATE_OK;
}

FSYM_EXPORT(JSON_DELEGATE(psm_set));
FSYM_EXPORT(JSON_DELEGATE(psm_get));
FSYM_EXPORT(JSON_DELEGATE(cli));

#endif // #ifndef RELEASE

FSYM_EXPORT(JSON_DELEGATE(restore));
FSYM_EXPORT(JSON_DELEGATE(info));
FSYM_EXPORT(JSON_DELEGATE(config));

int JSON_DELEGATE(test)(jsmn_node_t* pjn, fp_json_delegate_ack ack, void* ctx)
{
    LOG_INFO("test\r\n");

    json_delegate_ack_result(ack, ctx, STATE_OK);
    return STATE_OK;
}
FSYM_EXPORT(JSON_DELEGATE(test));

