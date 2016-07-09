#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <rfget.h>
#include <crc32.h>
#include <partition.h>
#include <firmware_structure.h>
#include <pwrmgr.h>
#include <board.h>

#include <malloc.h>
#include <appln_dbg.h>
#include <ot/d0_otd.h>
#include <misc/led_indicator.h>
#include <call_on_worker_thread.h>
#include <lib/ota.h>
#include <lib/third_party_mcu_ota.h>
#include <lib/arch_os.h>
#include <lib/power_mgr_helper.h>
#include <lib/miio_up_manager.h>
#include <lib/miio_command.h>
#include <ot/lib_addon.h>

static uint32_t g_app_weight = 0;
static uint32_t g_wifi_weight = 0;
static uint32_t g_mcu_weight = 0;
static mum ota_up_manager = NULL;
static os_timer_t progress_report_timer;
static int g_ota_state;

extern uint32_t crc32_flash(mdev_t *fl_drv, uint32_t offset, uint32_t len);
const char *ota_state_string[] = {
        "\"idle\"",
        "\"downloading\"",
        "\"downloaded\"",
        "\"installing\"",
        "\"installed\"",
        "\"failed\"",
        "\"busy\""
};

static int verify_wifi_fw_and_get_size(struct partition_entry *p, int *fw_sz)
{
    int ret = WM_SUCCESS;
    struct wlan_fw_header header;
    uint32_t crc;
    mdev_t *dev = flash_drv_open(p->device);
    if (dev == NULL) {
        return -WM_FAIL;
    }

    // verify header (magic and size)
    flash_drv_read(dev, (uint8_t*) &header, sizeof(header), p->start);
    if (header.magic != WLAN_FW_MAGIC || header.length < 16 || header.length > 290 * 1024) {
        LOG_ERROR("wifi fw header invalid\r\n");
        ret = -WM_FAIL;
        goto out;
    }

    // verify crc
    flash_drv_read(dev, (uint8_t*) &crc, sizeof(crc), p->start + sizeof(header) + header.length);
    if (crc != crc32_flash(dev, p->start + sizeof(header), header.length)) {
        LOG_ERROR("wifi fw crc check failed\r\n");
        ret = -WM_FAIL;
        goto out;
    }
    *fw_sz = header.length + sizeof(struct wlan_fw_header);

    out: flash_drv_close(dev);
    return ret;
}

static int flash_copy(struct partition_entry *to, struct partition_entry *from, int size)
{
    int ret = WM_SUCCESS;
    #define FW_COPY_BUF_SIZE 256
    uint8_t buf[FW_COPY_BUF_SIZE];
    uint32_t bytes_copied = 0;
    mdev_t *dev_from = flash_drv_open(from->device);
    if (dev_from == NULL) {
        return -WM_FAIL;
    }
    mdev_t *dev_to = flash_drv_open(to->device);
    if (dev_to == NULL) {
        return -WM_FAIL;
    }

    //LOG_DEBUG("erase start...\r\n");
    flash_drv_erase(dev_to, to->start, to->size);
    //LOG_DEBUG("erase finished, start copying...\r\n");

    while (bytes_copied < size) {
        uint32_t size_to_copy;
        if (size - bytes_copied < FW_COPY_BUF_SIZE)
            size_to_copy = size - bytes_copied;
        else
            size_to_copy = FW_COPY_BUF_SIZE;
        flash_drv_read(dev_from, buf, size_to_copy, from->start + bytes_copied);
        flash_drv_write(dev_to, buf, size_to_copy, to->start + bytes_copied);
        bytes_copied += size_to_copy;
        //wmprintf("\r%d", bytes_copied * 100 / size);
    }
    //wmprintf("\r\n");
    //LOG_DEBUG("copy finished.\r\n");

    flash_drv_close(dev_from);
    flash_drv_close(dev_to);
    return ret;
}

static int copy_wifi_fw(struct partition_entry *to, struct partition_entry *from)
{
    struct wlan_fw_header wf_header;
    mdev_t *dev_from = flash_drv_open(from->device);
    if (dev_from == NULL) {
        return -WM_FAIL;
    }

    flash_drv_read(dev_from, (uint8_t*) &wf_header, sizeof(struct wlan_fw_header), from->start);
    flash_drv_close(dev_from);

    flash_copy(to, from, wf_header.length + sizeof(struct wlan_fw_header));
    return WM_SUCCESS;
}

int recover_wifi_fw(void)
{
    short history = 0;
    struct partition_entry *to = part_get_layout_by_id(FC_COMP_WLAN_FW, &history);
    struct partition_entry *from = rfget_get_passive_firmware();
    int wifi_fw_size = 0;

    if (WM_SUCCESS != verify_wifi_fw_and_get_size(from, &wifi_fw_size)) {
        LOG_ERROR("Bad wifi fw\r\n");
        return -WM_FAIL;
    }
    if (WM_SUCCESS != flash_copy(to, from, wifi_fw_size)) {
        LOG_ERROR("Wifi fw copy failed\r\n");
        return -WM_FAIL;
    }
    return WM_SUCCESS;
}

int download_app_fw(char *url)
{
    int ret = STATE_OK;
    struct partition_entry *p = rfget_get_passive_firmware();

    if (NULL == p) {
        //LOG_ERROR("getting passive app fw partition failed\r\n");
        return STATE_ERROR;
    }
    //LOG_DEBUG("passive partition is 0x%x, genlev=%d\r\n", p->start, p->gen_level);

    if (rfget_client_mode_update(url, p,NULL) == WM_SUCCESS) {
        LOG_DEBUG("App fw download succeeded.\r\n");
    } else {
        LOG_ERROR("App fw download failed.\r\n");
        ret = STATE_ERROR;
    }

    return ret;
}

int install_app_fw(void)
{
    struct partition_entry *p = rfget_get_passive_firmware();
    if (NULL == p) {
        return STATE_ERROR;
    }

    if (WM_SUCCESS != part_set_active_partition(p)) {
        //LOG_ERROR("setting partition 0x%x active failed\r\n", p->start);
        return STATE_ERROR;
    }
    return STATE_OK;
}

int update_app_fw(char *url)
{
    if(STATE_OK != download_app_fw(url))
        return STATE_ERROR;

    return install_app_fw();
}

int download_wifi_fw(char *url)
{
    int ret = STATE_OK;
    struct partition_entry *p = rfget_get_passive_wifi_firmware();
    bool share_download_partition = (NULL == p);

    if(share_download_partition) {
        p = rfget_get_passive_firmware();
        if(NULL == p)
            return STATE_ERROR;
        p->type = FC_COMP_WLAN_FW;
    }

    if (rfget_client_mode_update(url, p, NULL) == WM_SUCCESS) {
        LOG_DEBUG("Wifi fw download finished.\r\n");
    } else {
        LOG_ERROR("Wifi fw download failed.\r\n");
        ret = STATE_ERROR;
    }

    if(share_download_partition)
        p->type = FC_COMP_FW;
    return ret;
}

int install_wifi_fw(void)
{
    struct partition_entry *p = rfget_get_passive_wifi_firmware();

    if(NULL == p) {
        short history = 0;
        struct partition_entry *from = rfget_get_passive_firmware();
        struct partition_entry *to = part_get_layout_by_id(FC_COMP_WLAN_FW, &history);

        if(NULL == from || NULL == to)
            return STATE_ERROR;

        if (WM_SUCCESS != copy_wifi_fw(to, from)) {
            LOG_ERROR("wifi fw install failed\r\n");
            return STATE_ERROR;
        }
    } else {
        if (WM_SUCCESS != part_set_active_partition(p)) {
            return STATE_ERROR;
        }
    }

    return STATE_OK;
}

int update_wifi_fw(char *url)
{
    if(STATE_OK != download_wifi_fw(url))
        return STATE_ERROR;
    return install_wifi_fw();
}

int download_mcu_fw(char *url)
{
    int ret = STATE_OK;
    struct partition_entry *p = rfget_get_mcu_firmware();
    bool share_download_partition = (NULL == p);

    if(share_download_partition) {
        p = rfget_get_passive_firmware();
        if(NULL == p)
            return STATE_ERROR;
        p->type = FC_COMP_MCU_FW;
    }

    if (rfget_client_mode_update(url, p, NULL) == WM_SUCCESS) {
        LOG_DEBUG("MCU fw download finished.\r\n");
    } else {
        LOG_ERROR("MCU fw download failed.\r\n");
        ret = STATE_ERROR;
    }

    if(share_download_partition)
        p->type = FC_COMP_FW;
    return ret;
}

int install_mcu_fw(void)
{
    struct partition_entry *p = rfget_get_mcu_firmware();

    if(NULL == p) {
        p = rfget_get_passive_firmware();
        if(NULL == p)
            return STATE_ERROR;
    }

    if (WM_SUCCESS != transfer_mcu_fw(p)) {
        LOG_ERROR("transfer_mcu_fw failed\r\n");
        return STATE_ERROR;
    } else {
        LOG_INFO("transfer_mcu_fw succeeded\r\n");
        return STATE_OK;
    }
}

int update_mcu_fw(char *url)
{
    if(STATE_OK != download_mcu_fw(url))
        return STATE_ERROR;
    return install_mcu_fw();
}

int install_all_fw(char *sequence, bool no_reboot)
{
    int err = WM_SUCCESS;
    int i;

    LOG_DEBUG("============== install all fw ==============\r\n");
    for(i = 0; i < 3; i++) {
        if(g_app_weight > 0 && sequence[i] == 'a') {
            if(STATE_OK != install_app_fw()) {
                LOG_ERROR("install app fw failed\r\n");
                set_ota_state(OTA_STATE_FAILED);
                err = -WM_FAIL;
                goto exit;
            }
        }

        if(g_wifi_weight > 0 && sequence[i] == 'w') {
            if(STATE_OK != install_wifi_fw()) {
                LOG_ERROR("install wifi fw failed\r\n");
                set_ota_state(OTA_STATE_FAILED);
                err = -WM_FAIL;
                goto exit;
            }
        }

        if (g_mcu_weight > 0 && sequence[i] == 'm') {
            if(STATE_OK != install_mcu_fw()) {
                LOG_ERROR("install mcu fw failed\r\n");
                set_ota_state(OTA_STATE_FAILED);
                err = -WM_FAIL;
                goto exit;
            }
            mcmd_enqueue_raw("MIIO_mcu_version_req");
            os_thread_sleep(os_msec_to_ticks(5000)); // extra time for mcu to report updated version
        }
    }
    LOG_DEBUG("============== install fw end ==============\r\n");
    if(!no_reboot) {
        os_thread_sleep(os_msec_to_ticks(1000)); // make sure JSON_DELEGATE(ota_install) send ack
        pm_reboot_soc();
    } else {
        set_ota_state(OTA_STATE_INSTALLED);
    }
    return err;

exit:
    LOG_DEBUG("============== install fw failed ==============\r\n");
    return err;
}

int update_all_fw(char *app_url, char *wifi_url, char *mcu_url, char *seq, bool no_reboot)
{
    enum {
        IGNORED = 0,
        SUCCESS,
        FAIL,
    };
    int ota_result = IGNORED;
    char sequence[8];
    struct partition_entry *p1, *p2;
    bool share_download_partition = false;
    int ret, i;

    if (is_ota_ongoing()) {
        LOG_ERROR("OTA ongoing\r\n");
        ota_result = IGNORED;
        goto err_exit;
    }

    g_app_weight = g_wifi_weight = g_mcu_weight = 0;
    if (app_url && strncmp(app_url, "http", 4) == 0)
        g_app_weight = 32; // 320kB for app fw size
    if (wifi_url && strncmp(wifi_url, "http", 4) == 0)
        g_wifi_weight = 16; // 160kB for wifi fw size
    if (mcu_url && strncmp(mcu_url, "http", 4) == 0)
        g_mcu_weight = 16; // usually much smaller than wifi fw

    if(g_app_weight + g_wifi_weight + g_mcu_weight > 0) {
        ota_result = SUCCESS;
    } else {
        ota_result = IGNORED;
        LOG_ERROR("no valid URL, quit OTA.\r\n");
        goto err_exit;
    }

    if(seq == NULL || (seq[0] != 'a' && seq[0] != 'w' && seq[0] != 'm'))
        strncpy(sequence, "wam", sizeof(sequence));
    else
        strncpy(sequence, seq, sizeof(sequence));

    LOG_DEBUG("============== download all fw ==============\r\n");

    led_enter_ota_mode();

    p1 = rfget_get_passive_wifi_firmware();
    p2 = rfget_get_mcu_firmware();

    if(p1 == NULL || p2 == NULL)
        share_download_partition = true;
    else
        share_download_partition = false;

    for(i = 0; i < 3; i++) {
        if (g_app_weight > 0 && sequence[i] == 'a') {
            if(g_ota_state != OTA_STATE_DOWNLOADING)
                set_ota_state(OTA_STATE_DOWNLOADING);
            LOG_DEBUG("app: %s\r\n", app_url);

            if(share_download_partition)
                ret = update_app_fw(app_url);
            else
                ret = download_app_fw(app_url);

            if(WM_SUCCESS != ret) {
                ota_result = FAIL;
                goto finish;
            }

            g_updatefw_progress = 100;
        }

        if (g_wifi_weight > 0 && sequence[i] == 'w') {
            if(g_ota_state != OTA_STATE_DOWNLOADING)
                set_ota_state(OTA_STATE_DOWNLOADING);
            LOG_DEBUG("wifi: %s\r\n", wifi_url);

            if(share_download_partition)
                ret = update_wifi_fw(wifi_url);
            else
                ret = download_wifi_fw(wifi_url);

            if(WM_SUCCESS != ret) {
                ota_result = FAIL;
                goto finish;
            }

            g_updatewififw_progress = 100;
        }

        if (g_mcu_weight > 0 && sequence[i] == 'm') {
            if(g_ota_state != OTA_STATE_DOWNLOADING)
                set_ota_state(OTA_STATE_DOWNLOADING);
            LOG_DEBUG("mcu: %s\r\n", mcu_url);

            if(share_download_partition)
                ret = update_mcu_fw(mcu_url);
            else
                ret = download_mcu_fw(mcu_url);

            if(WM_SUCCESS != ret) {
                ota_result = FAIL;
                goto finish;
            }

            g_updateuserfw_progress = 100;
        }
    }

finish:
    if (ota_result == FAIL)
        LOG_ERROR("OTA fail!!!\r\n");
    else if(ota_result == SUCCESS)
        LOG_DEBUG("OTA success.\r\n");
    else if(ota_result == IGNORED)
        LOG_DEBUG("OTA ignored.\r\n");

    LOG_DEBUG("============== download fw end ==============\r\n");


err_exit:

    if (app_url)
        free(app_url);
    if (wifi_url)
        free(wifi_url);
    if (mcu_url)
        free(mcu_url);
    if (seq)
        free(seq);

    if (ota_result == FAIL) {
        set_ota_state(OTA_STATE_FAILED);
        led_exit_ota_mode();
        return -WM_FAIL;
    } else if(ota_result == SUCCESS) {
        if(share_download_partition) {
            if(!no_reboot) {
                set_ota_state(OTA_STATE_INSTALLING);
                os_thread_sleep(os_msec_to_ticks(6000));
                led_exit_ota_mode();
                pm_reboot_soc();
            } else {
                set_ota_state(OTA_STATE_INSTALLED);
            }
            return WM_SUCCESS;
        } else {
            set_ota_state(OTA_STATE_INSTALLING);
            ret = install_all_fw(sequence, no_reboot);
            led_exit_ota_mode();
            return ret;
        }
    } else {
        return WM_SUCCESS;
    }
}

int ota_progress(void)
{
    int prog;
    if (is_ota_ongoing() && 0 != g_app_weight + g_wifi_weight + g_mcu_weight) {
        prog = (g_app_weight * g_updatefw_progress + g_wifi_weight * g_updatewififw_progress
                + g_mcu_weight * g_updateuserfw_progress) / (g_app_weight + g_wifi_weight + g_mcu_weight);
    } else
        prog = 101; // 101 indicates not-in-updating-status
    return prog;
}

static void report_ota_progress(os_timer_arg_t arg)
{
    char last_progress[4] = "0", progress[4];
    uint32_t cur_progress;
    mum_get_property(ota_up_manager, "ota_progress", last_progress);
    cur_progress = ota_progress();
    if(atoi(last_progress) < cur_progress) {
        snprintf_safe(progress, sizeof(progress), "%lu", cur_progress);
        mum_set_property(ota_up_manager, "ota_progress", progress);
        mum_send_property(ota_up_manager);
    }
}

void report_ota_state(void)
{
    mum_set_property(ota_up_manager, "ota_state", ota_state_string[g_ota_state]);
    mum_send_property_with_retry(ota_up_manager, 5);
}

int set_ota_state(int new_state)
{
    if(new_state < OTA_STATE_IDLE || new_state > OTA_STATE_BUSY)
        return -1;

    int old_state = g_ota_state;

    if(old_state != OTA_STATE_DOWNLOADING && new_state == OTA_STATE_DOWNLOADING) {
        if(os_timer_create(&progress_report_timer, "ota_progress", os_msec_to_ticks(2500), report_ota_progress, NULL,
                OS_TIMER_PERIODIC, OS_TIMER_AUTO_ACTIVATE) != WM_SUCCESS) {
            //LOG_ERROR("ota report timer error\r\n");
        }
    } else if(old_state == OTA_STATE_DOWNLOADING && new_state != OTA_STATE_DOWNLOADING) {
        os_timer_delete(&progress_report_timer);
        report_ota_progress(NULL);
    }

    if(new_state != old_state) {
        g_ota_state = new_state;
        LOG_INFO("ota state change: %s -> %s\r\n", ota_state_string[old_state], ota_state_string[g_ota_state]);
    }

    report_ota_state();

    return 0;
}

const char* get_ota_state_string(void)
{
    return ota_state_string[g_ota_state];
}

bool is_ota_ongoing(void)
{
    return (g_ota_state == OTA_STATE_DOWNLOADING || g_ota_state == OTA_STATE_INSTALLING);
}

void led_enter_ota_mode(void)
{
    led_unlock(board_led_1().gpio);
    led_unlock(board_led_2().gpio);
    led_unlock(board_led_3().gpio);
    led_updating_fw();
    led_super_lock(board_led_1().gpio);
    led_super_lock(board_led_2().gpio);
    led_super_lock(board_led_3().gpio);
}

void led_exit_ota_mode(void)
{
    led_super_unlock(board_led_1().gpio);
    led_super_unlock(board_led_2().gpio);
    led_super_unlock(board_led_3().gpio);
    miio_led_on();
}

int ota_init(void)
{
    if(NULL == (ota_up_manager = mum_create2())) {
        return -2;
    }

#ifdef MIIO_COMMANDS
    struct partition_entry *p = find_valid_mcu_fw();
    if(p == NULL) {
        set_ota_state(OTA_STATE_IDLE);
        goto exit;
    }

    // start restoring
    LOG_INFO("start restoring mcu fw\r\n");
    if(WM_SUCCESS != transfer_mcu_fw(p)) {
        LOG_ERROR("restoring mcu fw error\r\n");
        set_ota_state(OTA_STATE_FAILED);
        goto exit;
    } else {
        set_ota_state(OTA_STATE_IDLE);
    }
    LOG_INFO("finish restoring mcu fw\r\n");

#else
    set_ota_state(OTA_STATE_IDLE);
#endif

#ifdef MIIO_COMMANDS
exit:
#endif
    return WM_SUCCESS;
}



static d0_ctx_call_ret_t ot_reboot(struct d0_loop*l, struct d0_invoke* invk)
{
    if(!is_ota_ongoing())
        pm_reboot_soc();
    return D0_INVOKE_RET_VAL_RELEASE;
}

int JSON_DELEGATE(reboot)(jsmn_node_t* pjn, fp_json_delegate_ack ack, void* ctx)
{
    json_delegate_ack_result(ack, ctx, STATE_OK);
    d0_invoke_start(d0_invoke_loop_alloc(d0_api_find(OTN_XOBJ_NAME_DEFAULT), ot_reboot, 0), NULL);

    return STATE_OK;
}

/************************************************************************************
 *
 * OTA INTERFACE 2
 * {"method":"miIO.ota","params":{"app_url":"...","wifi_url":"...","mcu_url":"..."}}
 * {"method":"miIO.get_ota_progress","params":{}}
 * {"method":"miIO.get_ota_state","params":{}}
 *
 ************************************************************************************/
int JSON_DELEGATE(ota)(jsmn_node_t* pjn, fp_json_delegate_ack ack, void* ctx)
{
    int ret = STATE_OK;
    const char *err;
    char *app_url = NULL, *wifi_url = NULL, *mcu_url = NULL, *seq = NULL;
    bool no_reboot;

    if (NULL == (app_url = malloc(URL_BUF_LEN))) {
        err = "no mem";
        ret = STATE_NOMEM;
        goto err_exit;
    }
    if (NULL == (wifi_url = malloc(URL_BUF_LEN))) {
        err = "no mem";
        ret = STATE_NOMEM;
        goto err_exit;
    }
    if (NULL == (mcu_url = malloc(URL_BUF_LEN))) {
        err = "no mem";
        ret = STATE_NOMEM;
        goto err_exit;
    }
    if (NULL == (seq = malloc(4))) {
        err = "no mem";
        ret = STATE_NOMEM;
        goto err_exit;
    }

    if (STATE_OK != jsmn_key2val_str(pjn->js, pjn->tkn, "app_url", app_url, URL_BUF_LEN, NULL)) {
        free(app_url);
        app_url = NULL;
    }

    if (STATE_OK != jsmn_key2val_str(pjn->js, pjn->tkn, "wifi_url", wifi_url, URL_BUF_LEN, NULL)) {
        free(wifi_url);
        wifi_url = NULL;
    }

    if (STATE_OK != jsmn_key2val_str(pjn->js, pjn->tkn, "mcu_url", mcu_url, URL_BUF_LEN, NULL)) {
        free(mcu_url);
        mcu_url = NULL;
    }

    if (STATE_OK != jsmn_key2val_str(pjn->js, pjn->tkn, "sequence", seq, 4, NULL)) {
        free(seq);
        seq = NULL;
    }

    if (STATE_OK != jsmn_key2val_bool(pjn->js, pjn->tkn, "no_reboot", (uint8_t*)&no_reboot)) {
        no_reboot = false;
    }

    g_updatefw_progress = g_updatewififw_progress = g_updateuserfw_progress = 0;
    json_delegate_ack_result(ack, ctx, STATE_OK);
    call_on_worker_thread((general_cross_thread_task)update_all_fw, NULL, 5, app_url, wifi_url, mcu_url, seq, no_reboot);
    return ret;

err_exit:
    if (app_url)
        free(app_url);
    if (wifi_url)
        free(wifi_url);
    if (mcu_url)
        free(mcu_url);
    if (seq)
        free(seq);
    json_delegate_ack_err(ack, ctx, ret, err);
    return ret;
}

int JSON_DELEGATE(get_ota_progress)(jsmn_node_t* pjn, fp_json_delegate_ack ack, void* ctx)
{
	char js_ret[64];
	jsmn_node_t jn;

	if(NULL == ack)
		return STATE_OK;

    int prog = ota_progress();
    LOG_DEBUG("ota progress:%u%\r\n", prog);


	jn.js_len = snprintf_safe(js_ret, sizeof(js_ret), "{\"result\":[%d]}", prog);

	jn.js = js_ret;
	jn.tkn = NULL;
	return ack(&jn, ctx);
}

int JSON_DELEGATE(get_ota_state)(jsmn_node_t* pjn, fp_json_delegate_ack ack, void* ctx)
{
    const char *state = get_ota_state_string();
    LOG_DEBUG("ota state:%s\r\n", state);

    char state_string[32];
    jsmn_node_t jn;
    jn.js = state_string;
    jn.js_len = snprintf_safe(state_string, sizeof(state_string), "{\"result\":[%s]}", state);
    jn.tkn = NULL;

    return ack(&jn, ctx);
}

FSYM_EXPORT(JSON_DELEGATE(reboot));
FSYM_EXPORT(JSON_DELEGATE(ota));
FSYM_EXPORT(JSON_DELEGATE(get_ota_progress));
FSYM_EXPORT(JSON_DELEGATE(get_ota_state));












