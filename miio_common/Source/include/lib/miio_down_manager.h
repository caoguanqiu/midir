#ifndef MIIO_DOWN_MANAGER_H_
#define MIIO_DOWN_MANAGER_H_

#include <lib/jsmn/jsmn_api.h>
#include <ot/d0_otd.h>

typedef enum {
    MDM_TYPE_JSON = 0,
    MDM_TYPE_RAW
} mdm_type_t;

typedef struct {
    mdm_type_t type;
    uint32_t time;
    fp_json_delegate_ack ack;
    void *ctx;
    char sid[OT_GATEWAY_SID_SIZE_MAX];
    int cmd_len;
    char cmd_str[0]; // must be the last of struct members
} down_command_t;

typedef struct mdm_struct *mdm;

mdm mdm_create(void);
void mdm_destroy(mdm *self);
down_command_t * mdm_deq(mdm self);
int mdm_process_cmd(down_command_t * cmd);
int mdm_enq(mdm self, const char *buf);
int mdm_enq_json(mdm self, const char *json, char* sid, fp_json_delegate_ack ack, void* ctx);
uint32_t mdm_get_q_len(mdm self);
int json2cmd(char * json, char ** method, char ** params);

#endif /* MIIO_DOWN_MANAGER_H_ */
