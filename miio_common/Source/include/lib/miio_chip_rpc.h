#ifndef _PERIOD_TASK_UTIL_H_
#define _PERIOD_TASK_UTIL_H_

int net_info(char*buf, int size, char* key, char* post);
int mac_info(char*buf, int size, char* key, char* post);
int model_info(char*buf, int size, char* key, char* post);
int ver_info(char*buf, int size, char* key, char* post);
int ap_info(char*buf, int size, char* key, char* post);
int mcu_ver_info(char*buf, int size, char* key, char* post);
int wifi_ver_info(char*buf, int size, char* key, char* post);
int miio_chip_rpc_init(void);
uint32_t miio_dev_info(char * buf, uint32_t buf_size, const char * post);

#endif //_PERIOD_TASK_UTIL_H_
