#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include <lib/miio_up_manager.h>

#define APPLICATION_PRIORITY 2
#define UartRecvBufferLen   256
#define UART_BUFFER_LEN 200
#define STARTFF  0xff
#define INSERTDATA  0x55

#define STARTFRAME0 0
#define STARTFRAME1 1
#define LENHAIGH    2
#define LENLOW      3
#define CMD         4
#define SN          5
#define FLAGES0     6
#define FLAGES1     7


#define CHECKDEVICEINFO 0x01
#define UPDEVICEINFO    0x02

#define CHECKCTRSTATUS    0x03
#define UPDEVICESTATUS    0x04

#define UPAUTOSTATUS      0x05
#define REAUTOSTATUS      0x06

#define HEARTBEAT       0x07
#define UPHEARTBEAT     0x08

#define UPNETCONFIG     0x09
#define RENETCONFIG     0x0A

#define UPRESETNET      0x0B
#define RERESETNET      0x0C

#define WIFISTATUS      0x0D
#define UPWIFISTATUS    0x0E

#define MCUill          0x11
#define WIFIill         0x12

#define UPTESTMODE      0x13
#define RETESTMODE      0x14

#define UPBINDING       0x15
#define REBINDING       0x16

typedef struct _cmd_set_def
{
	u32_t attr_flags; 
    u32_t start; 
    u32_t ingredients; 
	u32_t convenient; 
	u32_t pressure_set; 
	u32_t cooking_time; 
	u32_t recipe_basic_fun; 
	u32_t date_minutes; 
	u32_t net_recipe_num; 	
}cmd_set_def_t;

typedef struct
{
  int idx;
  const char *name;
} device_func_index;

typedef struct _device_func{
  uint8_t func;
  uint8_t value;
}device_func_t;

typedef struct _device_cmd_head
{
  uint8_t startframeflag0;  //0xff
  uint8_t startframeflag1;  //0xff
  uint8_t len_h;
  uint8_t len_l;
  uint8_t cmd;
  uint8_t sn;
  uint8_t flags0;
  uint8_t flags1;
  uint8_t action;
  uint8_t start;
  uint8_t ingredients;
  uint8_t convenient;
  uint8_t pressure_set;
  uint8_t cooking_time;
  uint8_t recipe_basic_fun;
  uint8_t date_minutes_h;
  uint8_t date_minutes_l;
  uint8_t net_recipe_num0;
  uint8_t net_recipe_num1;
  uint8_t net_recipe_num2;
  uint8_t net_recipe_num3;
  uint8_t cap_status;
  uint8_t pressure_now;
  uint8_t pressure_time;
  uint8_t cooking_countdown;
  uint8_t exhaust_countdown;
  uint8_t cooking_state;
  uint8_t error;
  uint8_t checksum;
 
}device_cmd_head_t;

typedef struct _device_func_json{
  const char *key;
  const char *value;
}device_func_json_t;

typedef struct _device_func_json_extra{
  const char *key;
  const char *value;
  const char *extra;
}device_func_json_extra_t;

typedef struct _device_status{//设备状态
 // device_func_json_t devType;//设备类型，保留待用
  device_func_json_t start;
  device_func_json_t ingredients;  
  device_func_json_t convenient;
  device_func_json_t pressure_set;
  device_func_json_t cooking_time;
  device_func_json_t recipe_basic_fun;
  device_func_json_t date_minutes;
  device_func_json_t net_recipe_num;
  device_func_json_t cap_status;
  device_func_json_t pressure_now;//
  device_func_json_t pressure_time;//
  device_func_json_t cooking_countdown;//
  device_func_json_t exhaust_countdown;//
  device_func_json_t cooking_state;//
  device_func_json_t error_0;//
  device_func_json_t error_1;//
  device_func_json_t error_2;//
  device_func_json_t error_3;//
  device_func_json_t error_4;//
  device_func_json_t error_5;//
  device_func_json_t error_6;//
  device_func_json_t error_7;//
}device_status_t;


void device_cmd_process(uint8_t *buf, int inlen);
uint32_t send_check_command_to_device(uint8_t cmd,uint8_t action,cmd_set_def_t *cmd_receive);
void supor2mi_loop(void);
void module2dev_loop(mdev_t *dev);
void user_init(void);

uint8_t calculate_lrc(uint8_t *inbuf,int inlen);
int  get_original_data(uint8_t *inbuf, int inlen);
int get_final_data(uint8_t *inbuf, int inlen);
void one_data_back(uint8_t *inbuf,int len);
int check_lrc(uint8_t *inbuf,int inlen);
void insert_toback(uint8_t *pos,int move_data_len,uint8_t data);

extern mdev_t *dev;
extern cmd_set_def_t cmd_set;
extern mum g_mum;
extern xQueueHandle xQueue;
extern uint8_t sem_get_prop;
extern uint8_t semnetnotify;
extern uint8_t onlineflag;
extern device_network_state_t state_config;


#endif