#include <lwip/lwipopts.h>
#include <lwip/arch.h>
#include <lib/uap_diagnosticate.h>
#include <lib/arch_os.h>
#include <lib/jsmn/jsmn_api.h>

#include <ot/lib_addon.h>
#include <ot/d0_otd.h>

#include <lib/third_party_mcu_ota.h>
#include <util/dump_hex_info.h>
#include "../../../../sdk/src/app_framework/app_ctrl.h"
#include <malloc.h>
#include <wlan.h>
#include <appln_dbg.h>

static volatile uint32_t trigger_uap  __attribute__((section(".nvram_uninit")));

uint32_t get_trigger_uap_info()
{
    return trigger_uap;
}

void set_trigger_uap_info(uint32_t value)
{
    __asm volatile ("dsb");
    trigger_uap = value;
    __asm volatile ("dsb");
}

extern struct connection_state conn_state;


static uint32_t construct_conn_json(char *buf,size_t buf_capacity)
{
    int n = 0;
    char * state = "unknown";

    if(conn_state.state == CONN_STATE_NOTCONNECTED) {
       state = "NOTCONNECTED"; 
    } else if(conn_state.state == CONN_STATE_CONNECTING) {
       state = "CONNECTING";
    } else if(conn_state.state == CONN_STATE_CONNECTED) {
        state = "CONNECTED";

        if(otn_is_online()) {
            state = "ONLINE";
        }
    }

    n += snprintf_safe(buf, buf_capacity, "{\"result\":{\"state\":\"%s\",\"auth_fail_count\":%d,\"conn_succes_count\":%d,\"conn_fail_count\":%d,\"dhcp_fail_count\":%d",
            state, conn_state.auth_failed,
            conn_state.conn_success,
            conn_state.conn_failed,
            conn_state.dhcp_failed);

    n += snprintf_safe(buf + n,buf_capacity - n,"}}");

    return n;
}


//{"method":"wifi_assoc_state","params":""}
int JSON_DELEGATE(wifi_assoc_state)(jsmn_node_t* pjn, fp_json_delegate_ack ack, void* ctx)
{

    int ret = 0;
    const char * err_str = "unknown";

    char  * js_buf = malloc(256);
    if(!js_buf) {
        err_str = "no mem";
        ret = STATE_NOMEM;
        goto error_handle;
    }

    uint32_t n = 0;

    n = construct_conn_json(js_buf,256);
    
    jsmn_node_t ack_node;
   
    memset(&ack_node,0,sizeof(jsmn_node_t));
    ack_node.js = js_buf;
    ack_node.js_len = n;
    ack_node.tkn= NULL;

    if(ack) {
        ack(&ack_node,ctx);

    }

    free(js_buf);

    return 0;

error_handle:
    if(ack)
	    json_delegate_ack_err(ack,ctx,ret,err_str);
    return 0;
}

//which means stop UAP
int JSON_DELEGATE(stop_diag_mode)(jsmn_node_t* pjn, fp_json_delegate_ack ack, void* ctx)
{
    if(is_uap_started()) {
        LOG_DEBUG("APP send stop diag command to stop uap \r\n");
        app_uap_stop();
    }

    json_delegate_ack_result(ack,ctx,0);

    return 0;
}

FSYM_EXPORT(JSON_DELEGATE(wifi_assoc_state));
FSYM_EXPORT(JSON_DELEGATE(stop_diag_mode));

