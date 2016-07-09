#ifndef _MAIN_EXTENSION_H_
#define _MAIN_EXTENSION_H_

#include <wmlist.h>
#include "miio_arch.h"


//FIXME: the content of the strcut will be changed after abandon mc200+8801
struct tag_cfg_struct{
    char mac_address[8];   //binary 
    char device_id[8];      //binary
    char key[20];           //binary
    char model[120];        //string
    char url[120];          //string
    char domain[64];        //string
};

#define MODEL_MAX_SIZE 23

/*
 * data config struct with fixed value
 */
struct fixed_cfg_data_t {
    uint16_t dport;         //destination port
    uint16_t lport;         //loacal port
    uint8_t  interval;      
    uint8_t  enauth;        //enable query token
};

extern char mcu_fw_version[10];
extern struct tag_cfg_struct cfg_struct;
extern uint8_t token[16];

void get_mcu_version_from_psm();

struct provisioned_t {
	int state; // FIXME: THIS IS A WORKAROUND, IF NOT STRUCT, GLOABAL VARIABLE WILL CHANGE UNEXPECTEDLY
};
extern struct provisioned_t g_provisioned;

typedef enum {
    SCAN_ROUTER = 0,
    NB_OPERATION= 1
}main_loop_message_type;

#define OT_TCP_DOMAIN   "ott.io.mi.com"
#define OT_UDP_DOMAIN   "ot.io.mi.com"
#define COUNTRY_DOMAIN_LENGTH   (2)

#define DEFAULT_MODULE  "xiaomi.dev.v"

#define OT_TOKEN          "ot_token"
#define ENAUTH_DEFAULT_VALUE    (1)

/*********************************************
* port from bm09, & will add more field in the
* near future
*********************************************/

typedef enum {
    FT_FACTORY = 0, //factory mode 
    FT_NORMAL = 1,    //boot more than one time 
    WRONG_MODE = 0xff
}factory_mode_t;



/* network connection state */
typedef enum {
    WAIT_SMT_TRIGGER = 0,
    SMT_CONNECTING,
    GOT_IP_NORMAL_WORK,
    UAP_WAIT_SMT,
    SMT_CFG_STOP,
}device_network_state_t;

typedef void (*dev_state_observer_func)(device_network_state_t state);

typedef struct {
    list_head_t list;
    dev_state_observer_func func;
}dev_work_state_obv_item_t;

void store_or_retrive_taged_config(void );

device_network_state_t get_device_network_state(void);
int register_device_network_state_observer(dev_state_observer_func func);
int deregister_device_network_state_observer(dev_state_observer_func func);
void set_device_network_state(device_network_state_t state);
void read_provision_status(void);
bool is_provisioned(void);
void led_network_state_observer(device_network_state_t state);
uint8_t *get_wlan_mac_address(void);
uint32_t get_device_did_digital(void);
int common_event_handler(int event, void *data);
void WEAK product_set_wifi_cal_data(void);
void init_addon_interface(void);

void main_loop_run();

uint8_t get_enauth_data_in_ram();
void set_enauth_data_in_ram(uint8_t enauth);
int get_enauth_data_in_psm(uint8_t *enauth);
int set_enauth_data_in_psm(uint8_t enauth);

uint64_t get_smt_cfg_uid_in_ram();
void set_smt_cfg_uid_in_ram(uint64_t data);

int psm_format_check(flash_desc_t *fl);

void set_dport_data_in_ram(uint16_t dport);

int set_country_domain_in_psm(const char * country_domain);
int get_country_domain_in_psm(char * buffer,int buffer_length);
int erase_country_domain_in_psm();

int set_model_in_psm(const char * model);
int get_model_in_psm(char *model,size_t length);

void netif_refresh(u32_t* local_if_mask, u32_t* local_if_netid, u32_t* uap_if_mask, u32_t* uap_if_netid);

char * construct_ssid_for_smt_cfg();

bool* get_read_otp_flag();
#endif //_MAIN_EXTENSION_H_

