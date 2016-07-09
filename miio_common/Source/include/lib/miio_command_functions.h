#ifndef MIIO_COMMAND_FUNCTIONS_H_
#define MIIO_COMMAND_FUNCTIONS_H_

void get_net(int argc, char **argv);
void get_time(int argc, char **argv);
void get_mac1(int argc, char **argv);
void get_set_model(int argc, char **argv);
void set_mcu_version(int argc, char **argv);
void get_set_ota_state(int argc, char **argv);
void reboot_miio_chip(int argc, char **argv);
void deprovision_miio_chip(int argc, char **argv);
void handle_mcu_update_req(int argc, char **argv);
void factory_mode(int argc, char **argv);
void get_pin_level(int argc, char **argv);
void call_download_url(int argc, char **argv);
#endif /* MIIO_COMMAND_FUNCTIONS_H_ */
