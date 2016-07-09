#include <wmtypes.h>
#include <pwrmgr.h>
#include <wmerrno.h>
#include <flash.h>
#include <stdint.h>
#include <wifi_events.h>
#include <wifi.h>
#include <wlan_11d.h>
#include <wlan.h>

/*********************************************************
* refer to g_wifi_scan_params defined in file wlan.c 
**********************************************************/
static struct wifi_scan_params_t wifi_scan_params = {
	NULL, NULL, {0,}, BSS_ANY, 60, 153
};

int set_unicast_scan_params(struct wifi_scan_params_t *wifi_scan_params);

int scan_special_router(uint8_t *bssid, int (*cb) (unsigned int count)) 
{
    int ret;

    if(!bssid || !cb) {
        return -WM_FAIL;
    }

    //init params 
    wifi_scan_params.bssid = bssid;

    ret = set_unicast_scan_params(&wifi_scan_params);
    if(WM_SUCCESS != ret) {
        return ret;
    }

    return wlan_scan(cb);
}
