#ifndef _THIRD_PARTY_MCU_OTA_H_
#define _THIRD_PARTY_MCU_OTA_H_

#include <partition.h>
#define XMODEM_MTU (128)

int xmodem_get_crc_config(int *crc_ret);
int xmodem_transfer_data(int crc,unsigned char *src, int srcsz,int xmodem_mtu);
int xmodem_transfer_end(int crc ,unsigned char *src, int srcsz,int xmodem_mtu);
void cancel_xmodem_transfer();

int mcu_ota_from_flash(struct partition_entry *from);

int ota_uart_byte_read(unsigned char * byte);
void ota_uart_type_write(unsigned char trychar);
int ota_begin(void);

int transfer_mcu_fw(struct partition_entry *p);
struct partition_entry * find_valid_mcu_fw(void);

int dl_uart_init();
void dl_uart_deinit();

#endif //_THIRD_PARTY_MCU_OTA_H_
