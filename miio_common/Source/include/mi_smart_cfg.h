#ifndef _MI_SMART_CFG_H_
#define _MI_SMART_CFG_H_

void stop_smart_config();

#define DEFAULT_AP_CHANNEL  (6)

int smt_cfg_monitor_init();
int smt_cfg_monitor_start();
int smt_cfg_monitor_stop();
int smt_cfg_monitor_destory();
void set_smart_config_wait_time(uint32_t timeout);

#endif // _MI_SMART_CFG_H_
