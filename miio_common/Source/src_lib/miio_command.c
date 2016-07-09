#include <stdbool.h>
#include <string.h>
#include <malloc.h>
#include <appln_dbg.h>
#include <pwrmgr.h>
#include <lib/arch_os.h>
#include <lib/miio_up_manager.h>
#include <lib/miio_down_manager.h>
#include <lib/text_on_uart.h>
#include <lib/miio_command.h>
#include <lib/miio_command_functions.h>
#include <miio_version.h>
#include <app_framework.h>

#if MIIO_BLE
#include "ble.h"
#include "mi_band.h"
#endif


#define MAX_COMMAND_LEN (1024)
#define MAX_COMMANDS (sizeof(command_table) / sizeof(struct command_entry))

struct command_entry {
    const char *name;
    const char *help;
    void (*function)(int argc, char **argv);
};

static void print_help(int argc, char **argv);
static void set_echo(int argc, char **argv);
static void mcmd_version(int argc, char **argv);
static void get_platform_string(int argc, char **argv);
static void get_firmware_version(int argc, char **argv);

static void get_up(int argc, char **argv);
static void update_properties(int argc, char **argv);
static void update_event(int argc, char **argv);
static void get_down(int argc, char **argv);
static void send_result(int argc, char **argv);
static void send_error(int argc, char **argv);
static void send_log(int argc, char **argv);
static void send_store(int argc, char **argv);
static void call_cloud_method(int argc, char **argv);
static void setwifi(int argc, char **argv);
static void getwifi(int argc, char **argv);

static void json_get_down(int argc, char **argv);
static void json_ack(int argc, char **argv);
static void json_send(int argc, char **argv);
#if MIIO_BLE
static void ble_scan(int argc, char **argv);
static void set_bands(int argc, char **argv);
static void get_bands_index(int argc, char **argv);
static void get_bands(int argc, char **argv);
#endif

int app_network_get_nw_state(void);
int app_network_get_nw(struct wlan_network *network);
int app_network_set_nw(struct wlan_network *network);


tou mcmd_tou;
mum mcmd_mum;
mdm mcmd_mdm;
static os_thread_vt* mcmd_thread;
static bool terminate_main;
static char * command_buf = NULL;
static struct command_entry command_table[] =
        {
                
                { "help",            NULL,                                      print_help },
                { "echo",            "\tARG: <on or off> RET: ok",              set_echo },
                { "version",         "\tARG: none RET: 1/2/3...",               mcmd_version },
                { "platform",        "\tARG: none RET: VENDOR_CHIPTYPE (eg. MARVELL_MC200)",     get_platform_string },
                { "fw_ver",          "\tARG: none RET: eg. 1.2.0",              get_firmware_version },

                { "net",             "\tARG: none RET: ok/error",               get_net },
                { "time",            "\tARG: <poisx or timezone(+8,-8,0)>  RET: ok/error",               get_time},
                { "mac",             "\tARG: none RET: ok/error",               get_mac1 },
                { "model",           "\tARG: <model_string> RET: <model>",      get_set_model },
                { "mcu_version",     "\tARG: <ver_string> RET: <mcu_ver>",      set_mcu_version },
                { "ota_state",       "\tARG: <ota_state_string> RET: <ota_state>",      get_set_ota_state},
                { "reboot",          "\tARG: none RET: ok/error",               reboot_miio_chip },
                { "restore",         "\tARG: none RET: ok/error",               deprovision_miio_chip },
                { "update_me",       "\tARG: none RET: ok/error",               handle_mcu_update_req },
                { "factory",         "\tARG: none RET: ok/error",               factory_mode },
                { "get_pin_level",   "\tARG: <mask1> <mask2> <mask3> RET: 0xXXXXXXXX 0xXXXXXXXX 0xXXXXXXXX",       get_pin_level },

                { "get_up",          "\tDeprecated! ARG: none RET: ok/error",                         get_up },
                { "props",           "\tARG: <prop1> <\"strvalue1\"> <prop2> <intvalue2> ...  RET: ok/error",   update_properties },
                { "event",           "\tARG: <event> <\"strvalue1\"> <intvalue2> ...          RET: ok/error",   update_event },
                { "get_down",        "\tARG: none  RET: down <method> <arg1> <arg2> ... / down none", get_down },
                { "result",          "\tARG: <\"value1\"> <value2> ... RET: ok/error",                    send_result },
                { "error",           "\tARG: <\"message\"> <error_code> RET: ok/error",                   send_error },
                { "log",             "\tARG: <log_name> <\"strarg1\"> <intarg2> ...",                           send_log },
                { "store",            "\tARG: <store_name> <\"strarg1\"> <intarg2> ...",                           send_store },
                { "call",            "\tARG: <method> <params>  RET: result ... / error ...",         call_cloud_method },

                {"setwifi",          "\tARG: <\"ssid\"> <\"passwd\">  RET: ok/ error",setwifi},
                {"getwifi",          "\tARG: none RET: <\"ssid\"> <rssi> / error",getwifi},
                // 暂时不开启这个接口
                //{ "dl",            "\tARG: uart <url>  RET: ok/error",         call_download_url },

                { "json_get_down",   "\tARG: none RET: <json_string>/down none",                      json_get_down },
                { "json_ack",        "\t\tARG: <json_string> RET: ok/error",                          json_ack },
                { "json_send",       "\t\tARG: <json_string> RET: ok/error",                          json_send },
                

#if MIIO_BLE
                { "ble_scan",             "\tARG: <on or off> RET: ok",                                                         ble_scan },
                //{ "ble_get_scan_result",  "\RET<addrType1> <addr1> <rssi1> <data1> <addrType2> <addr2> <rssi2> <data2>...",    json_get_down },

                { "set_bands",            "\tARG: <addr1> <addr2> ...     RET:ok/error",                            set_bands },
                { "get_bands_index",      "\tARG: <addr1> <addr2> ...     RET:ok/error",                            get_bands_index },
                { "get_bands",            "\tARG: <rssi or sleep or data> RET:<addr1> <rssi1 or sleep1 or data1> <addr2> <rssi2 or sleep2 or data2>...",  get_bands },
#endif
        };


down_command_t * g_down_command = NULL;
static uint32_t down_command_timeout = 1000;
#define JSON_BUF_SIZE 1024
static char * js_msg_send_buf = NULL;
static os_mutex_vt* g_up_ack_mutex;
static bool mcmd_initialized = false;


static const struct command_entry *lookup_command(char *name)
{
    int i = 0;
    if(!name)
        return NULL;
    if(strlen(name) == 0)
        return NULL;
    while (i < MAX_COMMANDS) {
        if (command_table[i].name != NULL && strcmp(command_table[i].name, name) == 0)
            return &command_table[i];
        i++;
    }

    return NULL;
}

#ifdef MIIO_COMMANDS_DEBUG
extern void uart_wifi_debug_send(u8_t* buf, u32_t length, char kind);
#endif
static int mcmd_main(void * arg)
{
    while(!terminate_main) {
        int argc = 0;
        char *argv[32];

#ifdef MIIO_COMMANDS_DEBUG
        u32 length;
        length = tou_pend_til_recv(mcmd_tou, (uint8_t*)command_buf, MAX_COMMAND_LEN);
        uart_wifi_debug_send((u8_t*)command_buf,length,'u');
#else
        tou_pend_til_recv(mcmd_tou, (uint8_t*)command_buf, MAX_COMMAND_LEN);
#endif
        mcmd_parse(command_buf, &argc, argv);
        if (argc > 0) {
            const struct command_entry * command = lookup_command(argv[0]);
            if (command && command->function)
                command->function(argc, argv);
            else
                tou_send(mcmd_tou, "command '%s' not found\r", argv[0]);
        }
    }
    return 0;
}


int mcmd_create(int uart_id)
{
    int ret;

    if (mcmd_initialized)
        return 0;

    mcmd_tou = tou_create(uart_id);
    if(!mcmd_tou) {
        LOG_ERROR("mcmd_tou create failed\r\n");
        return -1;
    }

    mcmd_mum = mum_create();
    if(!mcmd_mum) {
        LOG_ERROR("mcmd_mum create failed\r\n");
        return -2;
    }

    mcmd_mdm = mdm_create();
    if(!mcmd_mdm) {
        LOG_ERROR("mcmd_mdm create failed\r\n");
        return -3;
    }

    command_buf = malloc(MAX_COMMAND_LEN);
    if(!command_buf) {
        LOG_ERROR("command recv buf malloc failed\r\n");
        return -4;
    }

    js_msg_send_buf = malloc(JSON_BUF_SIZE);
    if(!js_msg_send_buf) {
        LOG_ERROR("json send buf malloc failed\r\n");
        return -5;
    }

    if (STATE_OK != arch_os_mutex_create(&g_up_ack_mutex)) {
        LOG_ERROR("mcmd up ack sem create failed\r\n");
        return -6;
    }

    terminate_main = false;
    ret = arch_os_thread_create(&mcmd_thread, "mcmd", mcmd_main, 1024, NULL);
    if(ret) {
        LOG_ERROR("mcmd_main create failed\r\n");
        return -7;
    }

    mcmd_initialized = true;

    return 0;
}

void mcmd_destroy(void)
{
    if (mcmd_initialized) {
        terminate_main = true;
        tou_quit_pending(mcmd_tou);
        arch_os_thread_delete(mcmd_thread);
        arch_os_mutex_delete(g_up_ack_mutex);
        tou_destroy(&mcmd_tou);
        mum_destroy(&mcmd_mum);
        mdm_destroy(&mcmd_mdm);
        if(command_buf)
            free(command_buf);
        command_buf = NULL;
        if(js_msg_send_buf)
            free(js_msg_send_buf);
        js_msg_send_buf = NULL;
        mcmd_initialized = false;
    }
}

int mcmd_parse(char * inbuf, int *argc, char **argv)
{
    struct {
        unsigned inArg :1;
        unsigned inQuote :1;
        unsigned done :1;
        unsigned brace :1;
    } stat;
    int i = 0;

    *argc = 0;
    memset(&stat, 0, sizeof(stat));

    do {
        switch (inbuf[i]) {
        case '\0':
            if (stat.inQuote)
                return 2;
            stat.done = 1;
            break;
        case ' ':
        case ',':
            if(stat.brace > 0)
                break;
            if (i > 0 && inbuf[i - 1] == '\\' && stat.inArg) {
                memcpy(&inbuf[i - 1], &inbuf[i], strlen(&inbuf[i]) + 1);
                --i;
                break;
            }
            if (!stat.inQuote && stat.inArg) {
                stat.inArg = 0;
                inbuf[i] = '\0';
            }
            break;
        case '"':
            if(stat.brace > 0)
                break;
            if (i > 0 && inbuf[i - 1] == '\\' && stat.inArg) {
                memcpy(&inbuf[i - 1], &inbuf[i], strlen(&inbuf[i]) + 1);
                --i;
                break;
            }
            if (!stat.inQuote && stat.inArg)
                break;
            if (stat.inQuote && !stat.inArg)
                return 2;

            if (!stat.inQuote && !stat.inArg) {
                stat.inArg = 1;
                stat.inQuote = 1;
                (*argc)++;
                argv[*argc - 1] = &inbuf[i];
            } else if (stat.inQuote && stat.inArg) {
                stat.inQuote = 0;
                if(inbuf[i + 1] == '\0')
                    stat.done = 1;
            }
            break;
        case '{':
            if(stat.brace == 0 && !stat.inQuote && !stat.inArg) {
                stat.inArg = 1;
                (*argc)++;
                argv[*argc - 1] = &inbuf[i];
            }
            stat.brace++;
            break;
        case '}':
            if(!stat.inQuote) {
                stat.brace--;
            }
            break;

        default:
            if (!stat.inArg) {
                stat.inArg = 1;
                (*argc)++;
                argv[*argc - 1] = &inbuf[i];
            }
            break;
        }
    } while (!stat.done && ++i < MAX_COMMAND_LEN);

    if (stat.inQuote)
        return 2;

    if (*argc < 1)
        return 0;
    return 0;
}

int mcmd_enqueue_json(const char *buf, char* sid, fp_json_delegate_ack ack, void* ctx)
{
    return mdm_enq_json(mcmd_mdm, buf, sid, ack, ctx);
}

int mcmd_enqueue_raw(const char *buf)
{
    return mdm_enq(mcmd_mdm, buf);
}

/******************************************
 *
 * Functions in command table
 *
 * ****************************************
 */

static void print_help(int argc, char **argv)
{
    int i = 0;

    while (i < MAX_COMMANDS) {
        if (command_table[i].name)
            tou_send(mcmd_tou, "%s %s\r\n", command_table[i].name, command_table[i].help ? command_table[i].help : "");
        i++;
    }
}

static void set_echo(int argc, char **argv)
{
    if(argc < 2)
    {
        tou_send(mcmd_tou, "error");
        return;
    }

    if(strcmp(argv[1], "on") == 0) {
        tou_set_echo_on(mcmd_tou);
        down_command_timeout = 60000;
    } else if(strcmp(argv[1],"off") == 0) {
        tou_set_echo_off(mcmd_tou);
        down_command_timeout = 1000;
    } else{
        tou_send(mcmd_tou, "error");
        return;
    }

    tou_send(mcmd_tou, "ok");
}

static void mcmd_version(int argc, char **argv)
{
    tou_send(mcmd_tou, "2");
}

static void get_platform_string(int argc, char **argv)
{
#if defined(CONFIG_CPU_MC200) && defined (CONFIG_WiFi_8801)
    tou_send(mcmd_tou, "MARVELL_MC200_8801");
#elif defined(CONFIG_CPU_MW300)
    tou_send(mcmd_tou, "MARVELL_MW3XX"); // todo: distinguish mw302, mw300, mw310
#else
    tou_send(mcmd_tou, "UNKNOWN");
#endif
}

static void get_firmware_version(int argc, char **argv)
{
    char ver[16];
    get_miio_sdk_ver(ver, sizeof(ver));
    tou_send(mcmd_tou, "%s", ver);
}

static void get_up(int argc, char **argv)
{
    tou_send(mcmd_tou, "idle");
}

static void update_properties(int argc, char **argv)
{
    int i;
    if (argc < 3 || (argc & 0x1) != 0x1) {
        LOG_ERROR("wrong number of prop arg\r\n");
        goto err_exit;
    }

    for(i = 1; i + 1 < argc; i += 2) {
        if(MUM_OK != mum_set_property(mcmd_mum, argv[i], argv[i + 1])) {
            LOG_ERROR("prop invalid\r\n");
            goto err_exit;
        }
    }
    tou_send(mcmd_tou, "ok");
    return;

err_exit:
    tou_send(mcmd_tou, "error");
}

static void update_event(int argc, char **argv)
{
    size_t buf_size = 32;
    uint32_t buf_usage = 0;
    char *buf = calloc(buf_size, sizeof(char));
    int i;

    if (argc < 2) {
        LOG_ERROR("wrong number of event arg\r\n");
        tou_send(mcmd_tou, "error");
        goto exit;
    }

    for(i = 2; i < argc; i++) {
        size_t argv_len = strlen(argv[i]);
        if(buf_usage + argv_len + 1 >= buf_size) {
            buf_size = buf_usage + argv_len + 2;
            if(buf_size < 768)
                buf = realloc(buf, buf_size);
            else {
                LOG_ERROR("event string too long\r\n");
                tou_send(mcmd_tou, "error");
                goto exit;
            }
        }
        if(i > 2)
            buf_usage += snprintf_safe(buf + buf_usage, buf_size - buf_usage, ",");
        buf_usage += snprintf_safe(buf + buf_usage, buf_size - buf_usage, "%s", argv[i]);
    }

    if(MUM_OK == mum_set_event(mcmd_mum, argv[1], buf))
        tou_send(mcmd_tou, "ok");
    else {
        LOG_ERROR("event invalid\r\n");
        tou_send(mcmd_tou, "error");
    }

exit:
    if(buf)
        free(buf);
}

static void discard_down_command(int err_code, const char* err)
{
    if(g_down_command->ack)
        json_delegate_ack_err(g_down_command->ack, g_down_command->ctx, err_code, err);
    LOG_INFO("discard:%s (err=%s,t=%d)\r\n", g_down_command->cmd_str, err,
            arch_os_tick_elapsed(g_down_command->time));
    free(g_down_command);
    g_down_command = NULL;
}

static void get_down(int argc, char **argv)
{
    // if already processing a down command
    if (g_down_command) {
        if (arch_os_tick_elapsed(g_down_command->time) < down_command_timeout) {
            tou_send(mcmd_tou, "error");
            goto normal_exit;
        } else {
            // last cmd timeout
            LOG_ERROR("get_down1 time out\r\n");
            discard_down_command(OT_ERR_CODE_UART_TIMEOUT, OT_ERR_INFO_UART_TIMEOUT);
        }
    }

    // need to dequeue a new command
    // dequeue until find a valid command
    while (NULL != (g_down_command = mdm_deq(mcmd_mdm))) {
        if(arch_os_tick_elapsed(g_down_command->time) < down_command_timeout) {
            LOG_INFO("deQ:%s\r\n", g_down_command->cmd_str);
            break;
        } else {
            LOG_ERROR("get_down2 time out\r\n");
            discard_down_command(OT_ERR_CODE_UART_TIMEOUT, OT_ERR_INFO_UART_TIMEOUT);
        }
    }

    if (!g_down_command) {
        tou_send(mcmd_tou, "down none");
    } else if(g_down_command->type == MDM_TYPE_JSON) {
        char *method = NULL;
        char *params = NULL;
        json2cmd(g_down_command->cmd_str, &method, &params);
        if(method == NULL || strlen(method) == 0) {
            goto err_exit;
        }
        if(params == NULL || strlen(params) == 0) {
            tou_send(mcmd_tou, "down %s", method);
            LOG_INFO("tou_send:down %s\r\n", method);
        } else {
            tou_send(mcmd_tou, "down %s %s", method, params);
            LOG_INFO("tou_send:down %s %s\r\n", method, params);
        }
    } else if(g_down_command->type == MDM_TYPE_RAW) {
        tou_send(mcmd_tou, "down %s", g_down_command->cmd_str);
        LOG_INFO("tou_send:down %s\r\n", g_down_command->cmd_str);
    } else {
        goto err_exit;
    }

    if(g_down_command != NULL && g_down_command->ack == NULL) {
        free(g_down_command);
        g_down_command = NULL;
    }

normal_exit:
    return;

err_exit:
    if(g_down_command)
        discard_down_command(OT_ERR_CODE_PARSE, OT_ERR_INFO_PARSE);
    tou_send(mcmd_tou, "down none");
    return;
}

static void send_result(int argc, char **argv)
{
    jsmn_node_t jn;
    char* js = js_msg_send_buf;
    int i, n = 0;

    if (argc < 2) {
        tou_send(mcmd_tou, "error");
        return;
    }

    if (!g_down_command) {
        tou_send(mcmd_tou, "error");
        return;
    }

    if (api_os_tick_elapsed(g_down_command->time) >= down_command_timeout) {
        LOG_ERROR("sending result time out\r\n");
        discard_down_command(OT_ERR_CODE_UART_TIMEOUT, OT_ERR_INFO_UART_TIMEOUT);
        tou_send(mcmd_tou, "error");
        return;
    }

    n += snprintf_safe(js + n, JSON_BUF_SIZE - n, "{\"result\":[%s", argv[1]);
    for (i = 2; i < argc; i++) {
        n += snprintf_safe(js + n, JSON_BUF_SIZE - n, ",%s", argv[i]);
    }
    n += snprintf_safe(js + n, JSON_BUF_SIZE - n, "]}");

    LOG_INFO("%s\r\n", js);

    jn.js = js;
    jn.js_len = n;
    jn.tkn = NULL;

    g_down_command->ack(&jn, g_down_command->ctx);

    tou_send(mcmd_tou, "ok");
    free(g_down_command);
    g_down_command = NULL;

    return;
}

static void send_error(int argc, char **argv)
{
    jsmn_node_t jn;
    char* js = js_msg_send_buf;
    int n = 0;
    int err_code = OT_ERR_CODE_UART_UNDEF_ERROR;

    if (!g_down_command) {
        tou_send(mcmd_tou, "error");
        return;
    }

    if (argc < 2) {
        tou_send(mcmd_tou, "error");
        return;
    }

    if (argc >= 3)
        err_code = atoi(argv[2]);

    if (err_code > OT_ERR_CODE_USR_MAX)
        err_code = OT_ERR_CODE_UART_UNDEF_ERROR;

    if (err_code < OT_ERR_CODE_USR_MIN)
        err_code = OT_ERR_CODE_UART_UNDEF_ERROR;

    if (api_os_tick_elapsed(g_down_command->time) >= down_command_timeout) {
        LOG_ERROR("sending error time out\r\n");
        discard_down_command(OT_ERR_CODE_UART_TIMEOUT, OT_ERR_INFO_UART_TIMEOUT);
        tou_send(mcmd_tou, "error");
        return;
    }

    n += snprintf_safe(js + n, JSON_BUF_SIZE - n, "{\"error\":{\"code\":%d,\"message\":%s}}", err_code, argv[1]);

    LOG_INFO("%s\r\n", js);

    jn.js = js;
    jn.js_len = n;
    jn.tkn = NULL;

    g_down_command->ack(&jn, g_down_command->ctx);

    tou_send(mcmd_tou, "ok");
    free(g_down_command);
    g_down_command = NULL;

    return;
}

static void json_get_down(int argc, char **argv)
{
    // if already processing a down command
    if (g_down_command) {
        if (api_os_tick_elapsed(g_down_command->time) < down_command_timeout) {
            tou_send(mcmd_tou, "error");
            goto normal_exit;
        } else {
            // last cmd timeout
            LOG_ERROR("json_get_down1 time out\r\n");
            discard_down_command(OT_ERR_CODE_UART_TIMEOUT, OT_ERR_INFO_UART_TIMEOUT);
        }
    }

    // need to dequeue a new command
    // dequeue until find a valid command
    while (NULL != (g_down_command = mdm_deq(mcmd_mdm))
            && api_os_tick_elapsed(g_down_command->time) >= down_command_timeout) {
        LOG_ERROR("json_get_down2 time out\r\n");
        discard_down_command(OT_ERR_CODE_UART_TIMEOUT, OT_ERR_INFO_UART_TIMEOUT);
    }

    if (!g_down_command) {
        tou_send(mcmd_tou, "json_down none");
        goto normal_exit;
    } else if(g_down_command->type == MDM_TYPE_JSON) {
        tou_send(mcmd_tou, "json_down %s", g_down_command->cmd_str);
    } else if(g_down_command->type == MDM_TYPE_RAW) {
        tou_send(mcmd_tou, "json_down %s", g_down_command->cmd_str);
    } else {
        LOG_ERROR("unknown down command type, %d\r\n", g_down_command->type);
        tou_send(mcmd_tou, "json_down none");
    }

    if(g_down_command != NULL && g_down_command->ack == NULL) {
        free(g_down_command);
        g_down_command = NULL;
    }

normal_exit:
    return;
}
static void json_ack(int argc, char **argv)
{
    jsmn_node_t jn;
    char *js;

    if (!g_down_command)
        goto err_exit;

    if (argc < 2)
        goto err_exit;

    js = argv[1];

    if (arch_os_tick_elapsed(g_down_command->time) >= down_command_timeout) {
        LOG_ERROR("sending result time out\r\n");
        discard_down_command(OT_ERR_CODE_UART_TIMEOUT, OT_ERR_INFO_UART_TIMEOUT);
        goto err_exit;
    }

    LOG_INFO("%s\r\n", js);

    jn.js = js;
    jn.js_len = strlen(js);
    jn.tkn = NULL;

    g_down_command->ack(&jn, g_down_command->ctx);

    tou_send(mcmd_tou, "ok");
    free(g_down_command);
    g_down_command = NULL;
    return;

err_exit:
    tou_send(mcmd_tou, "error");
    return;
}

static int up_ack(jsmn_node_t* pjn, void* ctx)
{
    if (pjn && pjn->js && NULL == pjn->tkn) {
        LOG_WARN("cloud no response.\r\n");
    }

    if (ctx) {
        arch_os_mutex_put(*((os_mutex_vt **)ctx));
        tou_send(mcmd_tou, "ok");
    }

    return STATE_OK;
}

static void json_send(int argc, char **argv)
{
    if (STATE_OK != arch_os_mutex_get(g_up_ack_mutex, OS_NO_WAIT)) {
        tou_send(mcmd_tou, "busy");
        LOG_INFO("prop busy\r\n");
        return;
    }

    if (argc < 2) {
        tou_send(mcmd_tou, "error");
        goto err_exit;
    }

    wmprintf("%s\r\n", argv[1]);
    if (STATE_OK != ot_api_method(argv[1], strlen(argv[1]), up_ack, &g_up_ack_mutex)) {
        // OT not found, this happens usually at startup
        tou_send(mcmd_tou, "error");
        goto err_exit;
    }
    return;

err_exit:
    arch_os_mutex_put(g_up_ack_mutex);
    return;
}

static int up_log_ack(jsmn_node_t* pjn, void* ctx)
{
    return 0;
}

static void send_log(int argc, char **argv)
{
    char* js = js_msg_send_buf;
    int n = 0, i;

    if (argc < 3) {
        tou_send(mcmd_tou, "error");
        return;
    }
    for( i = 2; i < argc; i++) {
        if(i >= 3)
            n += snprintf_safe(js + n, JSON_BUF_SIZE - n, ",%s", argv[i]);
        else
            n += snprintf_safe(js + n, JSON_BUF_SIZE - n, "[%s", argv[i]);
    }
    n += snprintf_safe(js + n, JSON_BUF_SIZE - n, "]");

    LOG_INFO("up log:%s\r\n", js);
    if (MUM_OK != mum_set_log(mcmd_mum,argv[1],js))
        tou_send(mcmd_tou, "error");
    else
        tou_send(mcmd_tou, "ok");

    return;
}

static void send_store(int argc, char **argv){
    char* js = js_msg_send_buf;
    int n = 0, i;
    if (argc < 3) {
        tou_send(mcmd_tou, "error");
        return;
    }
    for (i = 2; i < argc; i++) {
        if (i >= 3)
            n += snprintf_safe(js + n, JSON_BUF_SIZE - n, ",%s", argv[i]);
        else
            n += snprintf_safe(js + n, JSON_BUF_SIZE - n, "[%s", argv[i]);
    }
    n += snprintf_safe(js + n, JSON_BUF_SIZE - n, "]");

    LOG_INFO("up store:%s\r\n", js);
    if (MUM_OK != mum_set_store(mcmd_mum,argv[1],js))
        tou_send(mcmd_tou, "error");
    else
        tou_send(mcmd_tou, "ok");

    return;
}


static int up_call_ack(jsmn_node_t* pjn, void* ctx)
{
    char result[128] = "";
    size_t res_len = 0;
    char ret_str[128];

    if(pjn && pjn->js)
        LOG_INFO("up call ack: %s\r\n", pjn->js);

    if (pjn && pjn->js && NULL == pjn->tkn) {
        snprintf_safe(ret_str, sizeof(ret_str), "error \"call timeout\"");
    } else {
        jsmntok_t *tk;
        char *ack;
        if(STATE_OK != jsmn_key2val_str(pjn->js, pjn->tkn, "result", result, sizeof(result), &res_len)) {
            if(NULL == (tk = jsmn_key_value(pjn->js, pjn->tkn, "error"))) {
                LOG_ERROR("up call ack no result nor error\r\n");
                goto err_exit;
            } else {
                if(STATE_OK != jsmn_key2val_str(pjn->js, tk, "message", result, sizeof(result), &res_len)) {
                    LOG_ERROR("up call ack no message\r\n");
                    goto err_exit;
                }
            }
            ack = "error";
        } else
            ack = "result";

        snprintf_safe(ret_str, sizeof(ret_str), "%s \"%s\"", ack, result);
    }
    mcmd_enqueue_raw(ret_str);
    return 0;

err_exit:
    mcmd_enqueue_raw("error \"invalid ack\"");
    return -1;
}

static void call_cloud_method(int argc, char **argv)
{
    char* js = js_msg_send_buf;
    int n = 0, i;

    if (argc < 2) {
        tou_send(mcmd_tou, "error");
        return;
    }

    n += snprintf_safe(js + n, JSON_BUF_SIZE - n, "{\"method\":\"_sync.%s\",\"params\":{", argv[1]);

    for (i = 2; i < argc; i++) {
        if(i >= 3)
            n += snprintf_safe(js + n, JSON_BUF_SIZE - n, ",%s", argv[i]);
        else
            n += snprintf_safe(js + n, JSON_BUF_SIZE - n, "%s", argv[i]);
    }
    n += snprintf_safe(js + n, JSON_BUF_SIZE - n, "}}");

    LOG_INFO("up call:%s\r\n", js);
    if (STATE_OK != ot_api_method(js, strlen(js), up_call_ack, NULL))
        tou_send(mcmd_tou, "error");
    else
        tou_send(mcmd_tou, "ok");
    return;
}

static void setwifi(int argc,char **argv){
    if(argc < 3){
        tou_send(mcmd_tou,"error");
    }

    char *ssid = NULL, *passwd = NULL;
    size_t ssid_len = strlen(argv[1]) - 2 , passwd_len = strlen(argv[2]) - 2 ; // skip \"

    ssid = malloc(IEEEtypes_SSID_SIZE + 1);
    passwd = malloc(WLAN_PSK_MAX_LENGTH + 1); //64+1
    if (!ssid || !passwd || ssid_len <= 0 || passwd_len <= 0 || ssid_len > IEEEtypes_SSID_SIZE || passwd_len > WLAN_PSK_MAX_LENGTH ) {
      LOG_ERROR("setwifi no mem");
      tou_send(mcmd_tou,"error");
      return;
    }

    memset(ssid,0,IEEEtypes_SSID_SIZE + 1);
    memset(passwd,0,WLAN_PSK_MAX_LENGTH + 1);

    memcpy(ssid,  argv[1] + 1, ssid_len ); //skip \"
    memcpy(passwd, argv[2] + 1, passwd_len);

    LOG_DEBUG("setwifi %s %s\r\n",ssid,passwd);

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
    if(APP_NETWORK_PROVISIONED == app_network_get_nw_state()) {
        app_network_set_nw(&new_network);
        should_reboot = true;
    }else{
        app_sta_save_network_and_start(&new_network);
    }

    if (ssid)
        free(ssid);
    if (passwd)
        free(passwd);

    tou_send(mcmd_tou, "ok");

    if (should_reboot) {
        arch_os_tick_sleep(500);
        pm_reboot_soc();
    }
}

static void getwifi(int argc, char **argv) {

    if (APP_NETWORK_PROVISIONED == app_network_get_nw_state()) {
        struct wlan_network network;
        if (WM_SUCCESS == app_network_get_nw(&network)) {
            short rssi;
            wlan_get_current_rssi(&rssi);
            tou_send(mcmd_tou,"\"%s\" %hd",network.ssid,rssi);
            return;
        }
    }
    tou_send(mcmd_tou, "error");
}

#ifdef MIIO_COMMANDS // this is to override the weak function in otd.c

int delegate_consume(jsmn_node_t* pjn_param, u32_t method_id, char* sid, fp_json_delegate_ack ack, void* ctx)
{
    mcmd_enqueue_json(pjn_param->js, sid, ack, ctx);
    LOG_INFO("enQ:%s\r\n", pjn_param->js);
    return STATE_OK;
}

#endif

#if MIIO_BLE
void ble_scan(int argc, char **argv)
{
    if (argc > 2) {
        LOG_ERROR("wrong number of prop arg. argc = %d\r\n", argc);
        goto err_exit;
    }

    if (0 == strcmp(argv[1], "on")) {
        miio_ble_discovery(1);
    } else if (0 == strcmp(argv[1], "off")){
        miio_ble_discovery(0);
    } else {
        goto err_exit;
    }

    tou_send(mcmd_tou, "ok");
    return;

err_exit:
    tou_send(mcmd_tou, "error");
}


static void set_bands(int argc, char **argv)
{
    int i;
    mi_band_sts_t status = MI_BAND_SUCC;
    if (argc < 2 ) {
        LOG_ERROR("wrong number of prop arg. argc = %d\r\n", argc);
        goto err_exit;
    }

    mi_band_reset();

    for(i = 1; i < argc; i++) {
        uint8_t mac[BLE_ADDR_LEN];
        LOG_DEBUG("%s\r\n", argv[i]);
        strtomac(argv[i], mac);
        if( MI_BAND_SUCC != (status = mi_band_add(mac))) {
            LOG_ERROR("prop invalid\r\n");
            goto err_exit;
        }
    }
    tou_send(mcmd_tou, "ok");
    return;

err_exit:
    switch (status) {
        case MI_BAND_ERR_TABLE_FULL:
            tou_send(mcmd_tou, "error: band table full");
            break;

        case MI_BAND_ERR_EXISTED:
            tou_send(mcmd_tou, "error: band already existed");
            break;

        default:
            tou_send(mcmd_tou, "error");
            break;

    }
    

}

//"\tARG: <rssi or sleep or data> RET:<addr1> <rssi1 or status> <addr2> <rssi2 or sleep2>...

static void get_bands(int argc, char **argv)
{
    uint8_t i;
    char *str = malloc(300);
    int fRssi = 0;
    int fStatus = 0;
    int8_t rssi;
    uint8_t bandSts;
    uint8_t curNum;
    uint8_t mac[BLE_ADDR_LEN];
    if (argc > 2 ) {
        LOG_ERROR("wrong number of prop arg. argc = %d\r\n", argc);
        goto err_exit;
    }
    //strcchar paraStr[50] 

    if (0 == strcmp(argv[1], "rssi")) {
        fRssi = 1;
    } else if (0 == strcmp(argv[1], "status")){
        fStatus = 1;
    } else {
        goto err_exit;
    }

    if (str) {
        *str = '\0';
    } else {
        goto err_exit;
    }

    curNum = mi_band_get_curNum();
    if (curNum == 0|| curNum == 0xff) {
        goto err_exit;
    }

    for (i = 0; i < curNum; i++) {
        if (MI_BAND_SUCC != mi_band_get_entry(i, mac, &rssi, &bandSts)) {
            goto err_exit;
        }
        
        if (fRssi) {
            sprintf(str, "%s%02x%02x%02x%02x%02x%02x %d ", str, mac[0], mac[1],mac[2],mac[3],mac[4],mac[5], (int8_t)rssi);
        } else if (fStatus) {
            if (bandSts == MI_BAND_USER_STATUS_SLEEPING) {
                sprintf(str, "%s%02x%02x%02x%02x%02x%02x sleep ", str, mac[0], mac[1],mac[2],mac[3],mac[4],mac[5]);
            } else {
                sprintf(str, "%s%02x%02x%02x%02x%02x%02x reserved ", str, mac[0], mac[1],mac[2],mac[3],mac[4],mac[5]);
            }
            
        } else {
            goto err_exit;
        }
        
    }

    sprintf(str, "%s\r\n\0", str);
    LOG_DEBUG("%s", str);
    tou_send(mcmd_tou, str);
    free(str);
    return;

err_exit:
    free(str);
    tou_send(mcmd_tou, "error");
    return;


}



static void get_bands_index(int argc, char **argv)
{
    char str[100] = "";
    uint8_t mac[6];
    int8_t rssi;
    uint8_t status;

    uint8_t index = atoi(argv[1]);

    if (MI_BAND_SUCC != mi_band_get_entry(index, mac, &rssi, &status)) {
        tou_send(mcmd_tou, "error");
        return;
    }
    
    sprintf(str, "%smac:%02x%02x%02x%02x%02x%02x, rssi: %d, status: %d\0", str, mac[0],
        mac[1],mac[2],mac[3],mac[4],mac[5], rssi, status);
    wmprintf("%s\r\n", str);
    tou_send(mcmd_tou, str);
}



#endif

