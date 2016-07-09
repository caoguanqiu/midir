#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <malloc.h>
#include <mdev_uart.h>
#include <appln_dbg.h>
#include <lib/arch_os.h>
#include <lib/text_on_uart.h>

#define END_CHAR '\r'
#define MSG_BUF_LEN (1024)
typedef struct tou_struct {
    mdev_t * mdev;
    uint32_t rx_byte_count;
    char msg_buf[MSG_BUF_LEN];
    os_mutex_vt* msg_buf_mutex;
    bool echo;
    bool quit_pending;
} *tou;

tou tou_create(int uart_id)
{
    tou self = malloc(sizeof(struct tou_struct));
    if(!self)
        return NULL;
    if(uart_drv_init(uart_id, UART_8BIT)) {
        free(self);
        return NULL;
    }

    if(uart_drv_blocking_read(uart_id, true) != WM_SUCCESS) {
        free(self);
        return NULL;
    }

    if(!(self->mdev = uart_drv_open(uart_id, 115200))) {
        free(self);
        return NULL;
    }

    arch_os_mutex_create(&self->msg_buf_mutex);

    self->rx_byte_count = 0;
    self->echo = false;
    self->quit_pending = false;

    return self;
}

void tou_destroy(tou * self)
{
    if(!(*self))
        return;
    uint32_t uart_id = (*self)->mdev->port_id;
    arch_os_mutex_delete((*self)->msg_buf_mutex);
    uart_drv_close((*self)->mdev);
    uart_drv_deinit(uart_id);
    free(*self);
    *self = NULL;
}

static int tou_send_char(tou self, char c)
{
    arch_os_mutex_get(self->msg_buf_mutex, OS_WAIT_FOREVER);
    uart_drv_write(self->mdev, (uint8_t*)&c, 1);
    arch_os_mutex_put(self->msg_buf_mutex);
    return 0;
}

int tou_send_without_end_char(tou self, const char * format, ...)
{
    va_list args;
    int n = 0;
    arch_os_mutex_get(self->msg_buf_mutex, OS_WAIT_FOREVER);
    va_start(args, format);
    n = vsnprintf(self->msg_buf, MSG_BUF_LEN, &format[0], args);
    va_end(args);
    uart_drv_write(self->mdev, (uint8_t*)self->msg_buf, n);
    arch_os_mutex_put(self->msg_buf_mutex);
    return n;
}

int tou_pend_til_recv(tou self, uint8_t * buf, uint32_t buf_size)
{
    uint8_t c;
    uint32_t bytes;

    if(self->echo)
        tou_send_without_end_char(self, "\n# ");

    self->rx_byte_count = 0;
    while(!self->quit_pending) {
        bytes = uart_drv_read(self->mdev, &c, 1);
        if (1 == bytes) {
            if(self->echo) {
                if(END_CHAR == c)
                    tou_send_without_end_char(self, "\r\n");
                else if(0x08 == c || 0x7f == c) {
                    if(self->rx_byte_count > 0) {
                        tou_send_without_end_char(self, "%c %c", 0x08, 0x08);
                        self->rx_byte_count--;
                    }
                } else
                    tou_send_char(self, c);
            }
            if (END_CHAR == c) {
                break;
            } else {
                if(c > 31 && c < 127 && !(self->rx_byte_count == 0 && c == ' '))
                    buf[self->rx_byte_count++] = c;
                if (self->rx_byte_count >= buf_size)
                    self->rx_byte_count = 0;
            }
        } else
            LOG_ERROR("Blocking uart read error (bytes=%u)\r\n", bytes);
    }

    buf[self->rx_byte_count] = '\0';
    self->quit_pending = false;
    return self->rx_byte_count;
}

void tou_quit_pending(tou self)
{
	if(self)
		self->quit_pending = true;
}

#ifdef MIIO_COMMANDS_DEBUG
extern void uart_wifi_debug_send(u8_t* buf, u32_t length, char kind);
#endif
int tou_send(tou self, const char * format, ...)
{
    va_list args;
    int n = 0;

    arch_os_mutex_get(self->msg_buf_mutex, OS_WAIT_FOREVER);
    va_start(args, format);
    n = vsnprintf(self->msg_buf, MSG_BUF_LEN - 1, &format[0], args);
    va_end(args);
    if(n < MSG_BUF_LEN - 1) {
        self->msg_buf[n++] = END_CHAR;
        uart_drv_write(self->mdev, (uint8_t*)self->msg_buf, n);
    #ifdef MIIO_COMMANDS_DEBUG
        uart_wifi_debug_send((uint8_t*)self->msg_buf, n,'d');
    #endif
    } else {
        n = 0;
    }
    arch_os_mutex_put(self->msg_buf_mutex);
    return n;
}

void tou_set_echo_on(tou self)
{
    self->echo = true;
}

void tou_set_echo_off(tou self)
{
    self->echo = false;
}
