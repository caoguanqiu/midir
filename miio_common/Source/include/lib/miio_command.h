#ifndef MIIO_COMMAND_H_
#define MIIO_COMMAND_H_

#include <lib/jsmn/jsmn_api.h>

typedef struct mcmd_struct *mcmd;

int mcmd_create(int uart_id);
void mcmd_destroy(void);
int mcmd_parse(char * inbuf, int *argc, char **argv);
int mcmd_enqueue_json(const char *buf, char* sid, fp_json_delegate_ack ack, void* ctx);
int mcmd_enqueue_raw(const char *buf);

#endif /* MIIO_COMMAND_H_ */
