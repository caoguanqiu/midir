#include <ctype.h>
#include <stdlib.h>
#include <malloc.h>

#include <psm.h>
#include <app_framework.h>
#include <pwrmgr.h>

#include <ot/d0_otd.h>
#include <main_extension.h>
#include <wmtime.h>
#include <appln_dbg.h>
#include <misc/miio_cli.h>
#include <lib/arch_io.h>
#include <lib/arch_os.h>
#include <lib/ota.h>
#include <lib/text_on_uart.h>
#include <lib/miio_command_functions.h>
#include <lib/uap_diagnosticate.h>
#include <lib/third_party_mcu_ota.h>
#include <call_on_worker_thread.h>
#include <lib/uap_diagnosticate.h>
#include <httpc.h>
#include <call_on_worker_thread.h>

extern tou mcmd_tou;

void get_net(int argc, char **argv)
{
    if (!is_provisioned()) {
        if(is_uap_started())
            tou_send(mcmd_tou, "uap");
        else
            tou_send(mcmd_tou, "unprov");
        return;
    }
    if (is_ota_ongoing()) {
        tou_send(mcmd_tou, "updating");
        return;
    }
    if (otn_is_online())
        tou_send(mcmd_tou, "cloud");
    else {
        if (GOT_IP_NORMAL_WORK == get_device_network_state())
            tou_send(mcmd_tou, "local");
        else
            tou_send(mcmd_tou, "offline");
    }
}

void get_time(int argc, char **argv)
{
    char time_str[32];
    struct tm time;
    time_t curtime;
    curtime = arch_os_utc_now();
    if(argc > 1 && strncmp(argv[1], "posix", 5) == 0) {
        tou_send(mcmd_tou, "%u", (uint32_t)curtime);
    }else {
        int tz = 8; // default +8 beijing time!!! do not change!! @zhangyanlu
        if(argc > 1){
            tz = atoi(argv[1]); //error input will return 0
        }
        curtime += tz*60*60;
        gmtime_r((const time_t *)&curtime, &time);
        snprintf_safe(time_str,sizeof(time_str),"%d-%02d-%02d %02d:%02d:%02d",time.tm_year + 1900,time.tm_mon+1,time.tm_mday,time.tm_hour,time.tm_min,time.tm_sec);
        tou_send(mcmd_tou, "%s", time_str);
    }
}

void get_mac1(int argc, char **argv)
{
    int ret;
    psm_handle_t handle;
    char mac[16];

    ret = psm_open(&handle, "ot_config");
    if (ret) {
        LOG_ERROR("Error opening PSM module\r\n");
        return;
    }

    if (WM_SUCCESS == psm_get(&handle, "psm_mac", mac, sizeof(mac))) {
        tou_send(mcmd_tou, "%s", mac);
    } else {
        tou_send(mcmd_tou, "error");
    }

    psm_close(&handle);
}

void get_set_model(int argc, char **argv)
{
    int ret;
    psm_handle_t handle;
    char model[64];

    ret = psm_open(&handle, "ot_config");
    if (ret) {
        LOG_ERROR("Error opening PSM module\r\n");
        return;
    }

    if (WM_SUCCESS != psm_get(&handle, "psm_model", model, sizeof(model))) {
        model[0] = '\0';
    }

    if (argc > 1) { // set
        bool illegal = false;
        {
            // verify model string
            int len = 0;
            int dot_idx[2] = { 0, 0 };
            int dot_cnt = 0;
            while ('\0' != argv[1][len]) {
                if ('.' == argv[1][len]) {
                    if (dot_cnt >= 2) {
                        //tou_send(mcmd_tou, "too many dots");
                        illegal = true;
                        break;
                    }
                    dot_idx[dot_cnt++] = len;
                }

                if (!isdigit(argv[1][len]) && !isalpha(argv[1][len]) && '_' != argv[1][len] && '-' != argv[1][len]
                        && '.' != argv[1][len]) {
                    //tou_send(mcmd_tou, "ill char");
                    illegal = true;
                    break;
                }

                len++;
            }

            if (dot_cnt != 2 || dot_idx[0] <= 0
            || dot_idx[1] - dot_idx[0] <= 1
            || len - dot_idx[1] <= 1
            || len > MODEL_MAX_SIZE) {
                //tou_send(mcmd_tou, "ill len");
                illegal = true;
            }
        }

        if (illegal) {
            LOG_ERROR("illegal model name: %s\r\n", argv[1]);
            tou_send(mcmd_tou, "error");
        } else {
            if (0 != strncmp(model, argv[1], sizeof(model))) {
                strcpy(cfg_struct.model, argv[1]);
                psm_set(&handle, "psm_model", argv[1]);
            } else {
                //tou_send(mcmd_tou, "same");
            }

            tou_send(mcmd_tou, "ok");
            LOG_INFO("model set by mcu. (%s)\r\n", argv[1]);
        }
    } else { // get
        tou_send(mcmd_tou, "%s", model);
        LOG_INFO("model queried by mcu. (%s)\r\n", model);
    }

    psm_close(&handle);
}

void set_mcu_version(int argc, char **argv)
{
    int ret;
    psm_handle_t handle;
    char ver[8];

    if (argc < 2) {
        tou_send(mcmd_tou, "error");
        return;
    }
    if (strlen(argv[1]) != 4) {
        tou_send(mcmd_tou, "error");
        LOG_ERROR("mcu version illegal, %s\r\n", argv[1]);
        return;
    }

    if (isdigit(argv[1][0]) && isdigit(argv[1][1]) && isdigit(argv[1][2]) && isdigit(argv[1][3])) {
        strcpy(mcu_fw_version, argv[1]);
        tou_send(mcmd_tou, "ok");
    } else {
        LOG_ERROR("mcu version illegal, %s\r\n", argv[1]);
        tou_send(mcmd_tou, "error");
        return;
    }

    ret = psm_open(&handle, "ot_config");
    if (ret) {
        LOG_ERROR("Error opening PSM module\r\n");
        return;
    }

    if (WM_SUCCESS != psm_get(&handle, "mcu_version", ver, sizeof(ver))) {
        ver[0] = '\0';
    }

    if (0 != strcmp(ver, mcu_fw_version)) {
        psm_set(&handle, "mcu_version", mcu_fw_version);
    }

    psm_close(&handle);
}

void get_set_ota_state(int argc, char **argv)
{
    const char *state = get_ota_state_string();
    char ota_state[16] = {0};

    memcpy(ota_state,state + 1,strlen(state) - 2);

    if (argc > 1) { // set
        if(0 != strncmp(argv[1],"idle",4) && 0 != strncmp(argv[1],"busy",4)){//wrong state
            LOG_ERROR("illegal ota_state name: %s\r\n", argv[1]);
            tou_send(mcmd_tou, "error");
        } 
        else if (0 != strncmp(ota_state, "idle",4) && 0 != strncmp(ota_state,"busy",4)) {//busy
            LOG_ERROR("is updating,refuse request: %s\r\n", argv[1]);
            tou_send(mcmd_tou, "ok");
        }
        else{
            if(0 == strncmp(argv[1],"idle",4)){
                set_ota_state(OTA_STATE_IDLE);
            }
            else if(0 == strncmp(argv[1],"busy",4)){
                set_ota_state(OTA_STATE_BUSY);
            }
            tou_send(mcmd_tou, "ok");
        }
    }
    else { // get
        tou_send(mcmd_tou, "%s", ota_state);
        LOG_INFO("ota_state queried by mcu. (%s)\r\n", ota_state);
    }
}

void reboot_miio_chip(int argc, char **argv)
{
    tou_send(mcmd_tou, "ok");
    arch_os_tick_sleep(500);
    pm_reboot_soc();
}

void deprovision_miio_chip(int argc, char **argv)
{
    tou_send(mcmd_tou, "ok");
    arch_os_tick_sleep(500);

    if(is_provisioned()) {
        app_reset_saved_network();
    }
    else{
        pm_reboot_soc(); //reset again,just reboot
    }
}

void handle_mcu_update_req(int argc, char **argv)
{
    // todo
    tou_send(mcmd_tou, "ok");
    LOG_ERROR("MCU request update\r\n");
/*
    struct partition_entry *p = find_valid_mcu_fw();
    if(p == NULL) {
        LOG_ERROR("no valid mcu fw found\r\n");
        return;
    }

    // start restoring
    LOG_INFO("start restoring mcu fw\r\n");
    call_on_worker_thread(transfer_mcu_fw, NULL, 1, p);
    LOG_INFO("finish restoring mcu fw\r\n");
*/
}

void factory_mode(int argc, char **argv)
{
    tou_send(mcmd_tou, "ok");
    arch_os_tick_sleep(500);
    cmd_enter_factory_mode(argc, argv);
}

void get_pin_level(int argc, char **argv)
{
    uint32_t pin, r1 = 0, r2 = 0, r3 = 0;
    uint32_t m1 = 0, m2 = 0, m3 = 0;

    if (argc != 4 && argc != 1) {
        tou_send(mcmd_tou, "error");
        return;
    }
    m1 = strtoul(argv[1], NULL, 16);
    m2 = strtoul(argv[2], NULL, 16);
    m3 = strtoul(argv[3], NULL, 16);

    for (pin = GPIO_0; pin <= GPIO_31; pin++) {
        int level = api_io_get(pin);
        if (1 == level)
            r1 |= 1 << pin;
    }

#if defined(CONFIG_CPU_MW300)
    for (pin = GPIO_32; pin <= GPIO_49; pin++) {
        int level = api_io_get(pin);
        if (1 == level)
            r2 |= 1 << pin;
    }
#elif defined(CONFIG_CPU_MC200)
    for (pin = GPIO_32; pin <= GPIO_63; pin++) {
        int level = api_io_get(pin);
        if (1 == level)
            r2 |= 1 << pin;
    }
    for (pin = GPIO_64; pin <= GPIO_79; pin++) {
        int level = api_io_get(pin);
        if (1 == level)
            r3 |= 1 << pin;
    }
#else 
#error not supported platform
#endif
    if (argc == 4)
        tou_send(mcmd_tou, "0x%.8x 0x%.8x 0x%.8x", r1 & m1, r2 & m2, r3 & m3);
    else if (argc == 1)
        tou_send(mcmd_tou, "0x%.8x 0x%.8x 0x%.8x", r1, r2, r3);
}

//-----dl uart command

#define DL_BUFF_SIZE 2048
#define DL_UART_BUFF_SIZE XMODEM_MTU
static int _do_dl2uart(char *url) {
    int status,dsize,crc,ret;
    ret = WM_FAIL;
    http_session_t handle;
    http_resp_t *resp = NULL;
    unsigned char* buf = malloc(DL_BUFF_SIZE);
    unsigned char* uart_buf = malloc(DL_UART_BUFF_SIZE);
    if (!buf) {
        LOG_ERROR("dl uart mem full");
        return ret;
    }
    if(!uart_buf){
        LOG_ERROR("dl uart mem full");
        goto err_dl_3;
    }
    status = dl_uart_init();
    if(status < 0){
        goto err_dl_3;
    }
    status = xmodem_get_crc_config(&crc);
    if (status < 0) {
        goto err_dl_2;
    }

    status = httpc_get(url, &handle, &resp, NULL );
    if (status != 0) {
        LOG_ERROR("dl uart error open url");
        goto err_dl_1;
    }

    if (resp->status_code != 200){
        LOG_ERROR("dl uart error http code:%d",resp->status_code);
        goto err_dl_1;
    }

    int uart_buflen = 0;

    while (1) {
        dsize = http_read_content(handle, buf, DL_BUFF_SIZE);
        if(dsize > 0){
            LOG_INFO("dl:%d\r\n",dsize);
            int offset = 0,size_left = dsize;

            while(1){

                int bytes_need = DL_UART_BUFF_SIZE - uart_buflen;

                if(size_left > bytes_need){

                    memcpy(uart_buf+uart_buflen,buf + offset,bytes_need);
                    uart_buflen =  uart_buflen + bytes_need;

                    status = xmodem_transfer_data(crc,uart_buf ,uart_buflen,XMODEM_MTU);
                    uart_buflen = 0;
                    offset += bytes_need;
                    size_left = size_left - bytes_need;
                    if (status != 0) {
                        goto err_dl_1;
                    }

                }else{

                    memcpy(uart_buf+uart_buflen,buf + offset,size_left);
                    uart_buflen =  uart_buflen + size_left;

                    break;
                }
            }



        }else if(dsize == 0){
            LOG_ERROR("********* All data read **********");

            xmodem_transfer_end(crc,uart_buf,uart_buflen,XMODEM_MTU);
            break;
        }else {//dsize < 0
            LOG_ERROR("dl uart error read url: %d", dsize);
            cancel_xmodem_transfer();
            break;
        }
    }
    ret = WM_SUCCESS;
err_dl_1:
    dl_uart_deinit();
err_dl_2:
    if(handle)
        http_close_session(&handle);
err_dl_3:
    if(buf)
        free(buf);
    if(uart_buf)
        free(uart_buf);

    return ret;
}

void call_download_url(int argc,char **argv){

    if(argc == 3 && strcmp(argv[1],"uart") == 0){
        if(WM_SUCCESS == call_on_worker_thread(_do_dl2uart,NULL,1,argv[2]) ){
            tou_send(mcmd_tou, "ok");
            return;
        }
    }
    tou_send(mcmd_tou, "error");
}


