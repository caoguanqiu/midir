
#ifndef TEXT_ON_UART_H_
#define TEXT_ON_UART_H_

#include <stdint.h>

typedef struct tou_struct *tou;

tou tou_create(int uart_id);
void tou_destroy(tou * self);
int tou_pend_til_recv(tou self, uint8_t * buf, uint32_t buf_size);
void tou_quit_pending(tou self);
int tou_send(tou self, const char * format, ...);
void tou_set_echo_on(tou self);
void tou_set_echo_off(tou self);

#endif /* TEXT_ON_UART_H_ */
