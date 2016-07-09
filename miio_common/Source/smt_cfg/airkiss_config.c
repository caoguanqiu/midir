#include <wmstdio.h>
#include <wmsysinfo.h>
#include <wm_net.h>
#include <lwip/udp.h>
#include <psm.h>
#include <system-work-queue.h>
#include "wlan_smc.h"
#include <app_framework.h>

#include "airkiss/airkiss.h"
#include "airkiss/airkiss_config.h"

#define AIRKISS_DEVICE_TYPE "gh_27098xxxxx" 
#define AIRKISS_DEVICE_ID "BD5D7899xxxxx"
static airkiss_context_t akcontex;
//static uint8_t cur_channel = 0xff;
static bool airkiss_lock_flag = false; 
static bool airkiss_done_flag = false; 
static uint8_t airkiss_random = 0;
static uint8_t lockedBSSID[6];
#define AIRKISS_TIMEOUT_TICKS 40000
const airkiss_config_t akconf = {
	(airkiss_memset_fn)&memset,
	(airkiss_memcpy_fn)&memcpy,
	(airkiss_memcmp_fn)&memcmp,
	(airkiss_printf_fn)&wmprintf
};

static struct wlan_network uap_network;

static os_thread_stack_define(apconfig_stack, 4096);
static uint8_t localKey[20];
static uint8_t localKeyLength;
static struct wlan_network uapNetwork;

typedef struct {
	wlan_frame_t frame;
	uint16_t length;
} capture_data_t;

#define CAPTURE_MAX_MESSAGES 1024
static os_queue_pool_define(capture_queue_data, CAPTURE_MAX_MESSAGES * sizeof(capture_data_t *));
os_queue_t capture_queue;


void sniffer_cb(const wlan_frame_t *frame, const uint16_t len)
{
	int ret;
	uint8_t client[MLAN_MAC_ADDR_LENGTH];
	wlan_get_mac_address(client);

	if (frame->frame_type == PROBE_REQ &&
	    frame->frame_data.probe_req_info.ssid_len) {

		/* To stop smc mode and start uAP on receiving directed probe
		 * request for uAP */
		if (strncmp(frame->frame_data.probe_req_info.ssid, uap_network.ssid,
			    frame->frame_data.probe_req_info.ssid_len))
			return;

	} else if (frame->frame_type == AUTH &&
		   (memcmp(frame->frame_data.auth_info.dest, client, 6) == 0)) {
		/* To stop smc mode and start uAP on receiving auth request */
        capture_data_t * uap_flag = NULL;
        uap_flag = os_mem_alloc(sizeof(capture_data_t));
        if(uap_flag) {
            memset(uap_flag,0,sizeof(capture_data_t));
            uap_flag->length = 0;
            uap_flag->frame.frame_type = 0xff;

            ret = os_queue_send(&capture_queue,&uap_flag,OS_NO_WAIT);
            if(ret != WM_SUCCESS) {
                SAND_LOG("fail to stop airkiss sniffer\r\n");
                os_mem_free(uap_flag);
            }
        }
        return;

	} else if (frame->frame_type == DATA || frame->frame_type == QOS_DATA) {
		capture_data_t* pItem = NULL;
		pItem = os_mem_alloc(sizeof(capture_data_t));
		if(pItem != NULL){
			//pItem->channel = chan_num;
			pItem->length = len;
			memcpy(&pItem->frame, frame, sizeof(wlan_frame_t));
			ret = os_queue_send(&capture_queue, &pItem, OS_NO_WAIT);
			if(ret != WM_SUCCESS){
				wmprintf("Failed to add into queue\r\n");
				os_mem_free(pItem);
			}
		} else {
			wmprintf("Failed to alloc mem\r\n");	
		}
	}
}

static int PROVISION_AP_NUM;
static int scan_cb(unsigned int count)
{
	PROVISION_AP_NUM = count;
	return 0;
}

uint8_t ap_list[20][7];
static void apConfigProcess(os_thread_arg_t data)
{
	int ret;
	int i;
    bool switch_to_uap = false;
	capture_data_t *pItem;
	airkiss_lock_flag = false; 
	airkiss_done_flag = false; 
	airkiss_random = 0;
	PROVISION_AP_NUM = -1;
	memset(ap_list, 0, sizeof(ap_list));

	wlan_scan(scan_cb);
	while(PROVISION_AP_NUM <0){
		os_thread_sleep(10);
	}
	struct wlan_scan_result res;
	for (i = 0; i < PROVISION_AP_NUM && i<20; i++) {
		wlan_get_scan_result(i, &res);
		ap_list[i][6] = res.channel;
		memcpy(&ap_list[i][0], res.bssid, 6);
	}

	os_queue_create(&capture_queue, "Capture Queue", sizeof(capture_data_t *), &capture_queue_data);

	ret = airkiss_init(&akcontex, &akconf);
	if(ret == 0){
		SAND_LOG("Airkiss Init Success");
	} else {
		SAND_LOG("Airkiss Init Failure");
	}

	ret = airkiss_set_key(&akcontex, localKey, localKeyLength);

	//Lock Channel
	uint8_t smc_frame_filter[] = {0x02, 0x12, 0x01, 0x2c, 0x04, 0x02};
	smart_mode_cfg_t smart_mode_cfg;

	bzero(&smart_mode_cfg, sizeof(smart_mode_cfg_t));
	smart_mode_cfg.beacon_period = 20;
	smart_mode_cfg.country_code = COUNTRY_CN;
	smart_mode_cfg.min_scan_time = 60;
	smart_mode_cfg.max_scan_time = 120;
	smart_mode_cfg.filter_type = 0x03;
	smart_mode_cfg.smc_frame_filter = smc_frame_filter;
	smart_mode_cfg.smc_frame_filter_len = sizeof(smc_frame_filter);
	smart_mode_cfg.custom_ie_len = 0;

	ret = wlan_set_smart_mode_cfg(&uapNetwork, &smart_mode_cfg, sniffer_cb);

	if (ret != WM_SUCCESS){
		SAND_LOG("Error: wlan_set_smart_mode_cfg failed");
	}
	wlan_start_smart_mode();

	while(!airkiss_lock_flag){
		ret = os_queue_recv(&capture_queue, &pItem, 20);
		if(ret == WM_SUCCESS){
            if((pItem->length == 0) && (pItem->frame.frame_type == 0xff)) {
                if(!airkiss_lock_flag) {
                    switch_to_uap = true;
                    os_mem_free(pItem);
                    break;
                }
            } else if( pItem->frame.frame_data.data_info.dest[0] == 0xFF &&
				pItem->frame.frame_data.data_info.dest[1] == 0xFF &&
				pItem->frame.frame_data.data_info.dest[2] == 0xFF &&
				pItem->frame.frame_data.data_info.dest[3] == 0xFF &&
				pItem->frame.frame_data.data_info.dest[4] == 0xFF &&
				pItem->frame.frame_data.data_info.dest[5] == 0xFF ){
				wmprintf("%d\r\n", pItem->length);
				ret = airkiss_recv(&akcontex, &pItem->frame, pItem->length);
				if(ret == AIRKISS_STATUS_CHANNEL_LOCKED){
					wmprintf("locked\r\n");
					airkiss_lock_flag = true; 
					memcpy(lockedBSSID, pItem->frame.frame_data.data_info.bssid, 6);
				}
			} 
			os_mem_free(pItem);
		}	
	}

	wlan_stop_smart_mode();
    if(switch_to_uap) {
        app_uap_start_on_channel_with_dhcp(uap_network.ssid, NULL, uap_network.channel);
        goto EXIT_POINT;
    }

	//Capture AP Info
	for(i=0; i<PROVISION_AP_NUM && i<20; i++){
		if(!memcmp(lockedBSSID, &ap_list[i][0], 6)){
			smart_mode_cfg.channel = ap_list[i][6];
			SAND_LOG("LOCK Channel[%d]", ap_list[i][6]);
			break;
		}
	}
	smart_mode_cfg.min_scan_time = 2000;
	smart_mode_cfg.max_scan_time = 2000;

	ret = wlan_set_smart_mode_cfg(&uapNetwork, &smart_mode_cfg, sniffer_cb);
	if (ret != WM_SUCCESS){
		SAND_LOG("Error: wlan_set_smart_mode_cfg failed");
	}
	wlan_start_smart_mode();
	while(!airkiss_done_flag){
		ret = os_queue_recv(&capture_queue, &pItem, 20);
		if(ret == WM_SUCCESS){
            
            if((pItem->length == 0) && (pItem->frame.frame_type == 0xff)) {
                if(!airkiss_done_flag) {
                    switch_to_uap = true;
                    os_mem_free(pItem);
                    break;
                }
            } else if( pItem->frame.frame_data.data_info.dest[0] == 0xFF &&
				pItem->frame.frame_data.data_info.dest[1] == 0xFF &&
				pItem->frame.frame_data.data_info.dest[2] == 0xFF &&
				pItem->frame.frame_data.data_info.dest[3] == 0xFF &&
				pItem->frame.frame_data.data_info.dest[4] == 0xFF &&
				pItem->frame.frame_data.data_info.dest[5] == 0xFF ){
				wmprintf("%d\r\n", pItem->length);
				ret = airkiss_recv(&akcontex, &pItem->frame, pItem->length);
				if(ret == AIRKISS_STATUS_COMPLETE){
					airkiss_done_flag = true;
				}
			} else if( pItem->frame.frame_data.data_info.dest[0] == 0x01 &&
				pItem->frame.frame_data.data_info.dest[1] == 0x00 &&
				pItem->frame.frame_data.data_info.dest[2] == 0x5E){
			}
			os_mem_free(pItem);
		}
	}
	wlan_stop_smart_mode();

    if(switch_to_uap) {
        app_uap_start_on_channel_with_dhcp(uap_network.ssid, NULL, uap_network.channel);
        goto EXIT_POINT;
    }

	//Done
	if(airkiss_done_flag){
		airkiss_result_t result;
		ret = airkiss_get_result(&akcontex, &result);
		if(ret == 0){
			SAND_LOG("%s %s %d %d", result.ssid, result.pwd, result.ssid_length, result.pwd_length);
			airkiss_random = result.random;
			struct wlan_network network;
			memset(&network, 0, sizeof(network));
			strncpy(network.name, "airkiss", sizeof(network.name));
			strncpy(network.ssid, result.ssid, sizeof(network.ssid));
			strncpy(network.security.psk, result.pwd, sizeof(network.security.psk));
			network.security.psk_len = strlen(network.security.psk);
			network.security.type = (result.pwd_length == 0)?WLAN_SECURITY_NONE : WLAN_SECURITY_WILDCARD;
			network.ip.ipv4.addr_type = ADDR_TYPE_DHCP;
			network.type = WLAN_BSS_TYPE_STA;
			network.role = WLAN_BSS_ROLE_STA;

			app_sta_save_network(&network);
			app_sta_start();
		}	
	}

EXIT_POINT:

	os_queue_delete(&capture_queue);
	os_thread_delete(NULL);
}

void start_airkiss(uint8_t *keyBuf, uint8_t keyLen, struct wlan_network* uap_network)
{
	memcpy(localKey, keyBuf, keyLen);
	localKeyLength = keyLen;
	memcpy(&uapNetwork, uap_network, sizeof(struct wlan_network));
	os_thread_create(NULL,
			 "ap config process",	
			 apConfigProcess,	
			 0,
			 &apconfig_stack,
			 OS_PRIO_3);
}

static void airkiss_lan_cb(void *data)
{
	struct sockaddr_in client_addr;
	int sock_fd;
	uint8_t sendBuf[256];
	uint16_t sendBufLen = 256;

	sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock_fd == -1) {
		wmprintf("failed to create sock_fd!\n\r");
	} else {	
		memset(&client_addr, 0, sizeof(client_addr));
		client_addr.sin_family = AF_INET;
		client_addr.sin_addr.s_addr = INADDR_BROADCAST;		
		client_addr.sin_port = htons(12476);

		sendBufLen = sizeof(sendBuf);
		int packret = airkiss_lan_pack(AIRKISS_LAN_SSDP_RESP_CMD, AIRKISS_DEVICE_TYPE, AIRKISS_DEVICE_ID, 0, 0, sendBuf, &sendBufLen, &akconf); 
		if (packret != AIRKISS_LAN_PAKE_READY) {
			SAND_LOG("Pack lan packet error!");
		} else {
			int err = sendto(sock_fd, &airkiss_random, 1, 0, (struct sockaddr *)&client_addr, sizeof(client_addr));
			close(sock_fd);

            //for warning 
            err = err;
		}
	}

}
static wq_handle_t airkiss_wq_handle;
void airkiss_lan_broadcast_start(uint32_t period_ms)
{
	if (sys_work_queue_init() != WM_SUCCESS){
		SAND_LOG("Failed to init sys work queue");
		return;
	}

	wq_job_t job = {
		.job_func = airkiss_lan_cb,
		.owner = "ak",  //short for airkiss
		.param = NULL,
		.periodic_ms = period_ms,
		.initial_delay_ms = 0,
	};

	airkiss_wq_handle = sys_work_queue_get_handle();
	if (airkiss_wq_handle){
		work_enqueue(airkiss_wq_handle, &job, NULL);
	}
}

void airkiss_lan_broadcast_stop()
{
	if(airkiss_wq_handle){
		work_dequeue_owner_all(airkiss_wq_handle, "airkiss");
	}
}

static os_thread_stack_define(airkiss_lan_stack, 4096);
static void airkiss_lan_sta_process(os_thread_arg_t data)
{
	uint8_t recvBuf[256];
	uint8_t sendBuf[256];
	uint16_t sendBufLen = 256;
	struct sockaddr_in server_addr;
	struct sockaddr_in client_addr;
	int len = sizeof(client_addr);
	int server_sock_fd = -1;
	int err;
	netif_add_udp_broadcast_filter(12476);

	airkiss_lan_broadcast_start(5000);

	server_sock_fd = -1;
	server_sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (server_sock_fd == -1) {
		wmprintf("failed to create server_sock_fd!\n\r");
	} else {	
		memset(&server_addr, 0, sizeof(server_addr));
		server_addr.sin_family = AF_INET;
		server_addr.sin_addr.s_addr = IPADDR_ANY;		
		server_addr.sin_port = htons(12476);

		err = bind(server_sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
		if(err == -1){
			SAND_LOG("Failed to bind UDP socket");
		} else {
			SAND_LOG("Succeed in binding UDP socket");
		}

		while(is_sta_connected()){
			memset(recvBuf, 0, 256);
			err = recvfrom(server_sock_fd, recvBuf, 256, 0, (struct sockaddr*)&client_addr, (socklen_t *)&len);
			if(err > 0){
				airkiss_lan_ret_t ret = airkiss_lan_recv(recvBuf, err, &akconf); 
				airkiss_lan_ret_t packret;
				switch (ret){
					case AIRKISS_LAN_SSDP_REQ:
						sendBufLen = sizeof(sendBuf);
						packret = airkiss_lan_pack(AIRKISS_LAN_SSDP_RESP_CMD, AIRKISS_DEVICE_TYPE, AIRKISS_DEVICE_ID, 0, 0, sendBuf, &sendBufLen, &akconf); 
						if (packret != AIRKISS_LAN_PAKE_READY) {
							SAND_LOG("Pack lan packet error!");
							continue; 
						}
						err = sendto(server_sock_fd, sendBuf, sendBufLen, 0, (struct sockaddr *)&client_addr, sizeof(client_addr));
						if (err < 0) {
							SAND_LOG("LAN UDP Send err!");
						}
					break;
					default:
						;
				}
			}
		}
	}
	airkiss_lan_broadcast_stop();

	os_thread_delete(NULL);
}

void airkiss_sta_init()
{
	struct sockaddr_in client_addr;
	int sock_fd;
	int i;
	int err;
	int ret;

	if(airkiss_random != 0){
		sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
		if (sock_fd == -1) {
			wmprintf("failed to create sock_fd!\n\r");
		} else {	
			memset(&client_addr, 0, sizeof(client_addr));
			client_addr.sin_family = AF_INET;
			client_addr.sin_addr.s_addr = INADDR_BROADCAST;		
			client_addr.sin_port = htons(10000);

			for(i=0; i<20; i++){
				err = sendto(sock_fd, &airkiss_random, 1, 0, (struct sockaddr *)&client_addr, sizeof(client_addr));

                //for warning
                err = err;

				os_thread_sleep(10);
			}
			close(sock_fd);
		}
	} else {
		ret = airkiss_init(&akcontex, &akconf);
		if(ret == 0){
			SAND_LOG("Airkiss Init Success");
		} else {
			SAND_LOG("Airkiss Init Failure");
		}
	}
	os_thread_create(NULL,
			 "airkiss lan process",	
			 airkiss_lan_sta_process,	
			 0,
			 &airkiss_lan_stack,
			 OS_PRIO_3);
}

void airkiss_config_with_sniffer(char * ssid)
{
	bzero(&uap_network, sizeof(struct wlan_network));
	memcpy(uap_network.ssid, ssid, strlen(ssid));
	uap_network.channel = 6;
	start_airkiss((uint8_t *)"[marvellairkiss]", 16, &uap_network);
}


