#ifndef OTA_H_
#define OTA_H_

#include <stdbool.h>
#include <wm_os.h>
#include <ot/d0_otd.h>

enum {
    OTA_STATE_IDLE = 0,
    OTA_STATE_DOWNLOADING,
    OTA_STATE_DOWNLOADED,
    OTA_STATE_INSTALLING,
    OTA_STATE_INSTALLED,
    OTA_STATE_FAILED,
    OTA_STATE_BUSY
};

#define URL_BUF_LEN 256

int update_app_fw(char *url);
int update_wifi_fw(char *url);
int recover_wifi_fw(void);
int update_all_fw(char *app_url, char *wifi_url, char *mcu_url, char *seq, bool no_reboot);
void update_all_fw_finish(int ret,int para_count, ...);
int download_all_fw(char *app_url, char *wifi_url, char *mcu_url);
int install_all_fw(char *sequence, bool no_reboot);
int ota_progress(void);
bool is_ota_ongoing(void);
void led_enter_ota_mode(void);
void led_exit_ota_mode(void);
int ota_init(void);

int set_ota_state(int state);
const char* get_ota_state_string(void);
void report_ota_state(void);
#endif /* OTA_H_ */
