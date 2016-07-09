#include <stdbool.h>
#include <stdint.h>
#include <malloc.h>
#include <appln_dbg.h>
#include <lib/lib_generic.h>
#include <lib/arch_os.h>
#include <lib/miio_down_manager.h>

#define MDM_Q_LEN 8

typedef struct mdm_struct {
    os_queue_t q;
} *mdm;

mdm mdm_create(void)
{
    mdm h = calloc(1, sizeof(struct mdm_struct));
    if(!h)
        return NULL;

    arch_os_queue_create(&h->q, MDM_Q_LEN, sizeof(void*));
    return h;
}

void mdm_destroy(mdm *self)
{
	if(!(*self))
		return;

    down_command_t * cmd = NULL;
    while(NULL != (cmd = mdm_deq(*self))) {
        free(cmd);
    }

    arch_os_queue_delete(&(*self)->q);
    if(*self)
        free(*self);
    (*self) = NULL;
}

down_command_t * mdm_deq(mdm self)
{
    if(!self)
        return NULL;

    down_command_t * cmd = NULL;
    if(STATE_OK != arch_os_queue_recv(&self->q, (void*)&cmd, OS_NO_WAIT)) {
        return NULL;
    }
    return cmd;
}

int mdm_process_cmd(down_command_t * cmd)
{
    if(cmd)
        free(cmd);
    return 0;
}

int mdm_enq(mdm self, const char *buf)
{
    if(!self)
        return STATE_ERROR;

    if(mdm_get_q_len(self) >= MDM_Q_LEN) {
        LOG_ERROR("miio down manager queue full\r\n");
        return STATE_ERROR;
    }

    size_t len = strlen(buf);
    down_command_t * cmd = calloc(sizeof(down_command_t) + len + 1, 1);
    if (!cmd)
        return STATE_NOMEM;
    cmd->ack = NULL;
    cmd->ctx = NULL;
    cmd->time = arch_os_tick_now();
    cmd->type = MDM_TYPE_RAW;
    cmd->cmd_len = len;
    strncpy(cmd->cmd_str, buf, len);

    return arch_os_queue_send(&self->q, &cmd, OS_NO_WAIT);
}

int mdm_enq_json(mdm self, const char *json, char* sid, fp_json_delegate_ack ack, void* ctx)
{
    if(!self)
        return STATE_ERROR;

    if(mdm_get_q_len(self) >= MDM_Q_LEN) {
        LOG_ERROR("miio down manager queue full\r\n");
        return STATE_ERROR;
    }

    size_t len = strlen(json);
    int ret;
    down_command_t * cmd = calloc(sizeof(down_command_t) + len + 1, 1);
    if (!cmd) {
        json_delegate_ack_err(ack, ctx, OT_ERR_CODE_RESP_ERROR, OT_ERR_INFO_RESP_ERROR);
        return STATE_NOMEM;
    }
    cmd->ack = ack;
    cmd->ctx = ctx;
    cmd->time = arch_os_tick_now();
    cmd->type = MDM_TYPE_JSON;
    cmd->cmd_len = len;
    strncpy(cmd->cmd_str, json, len);
    strncpy(cmd->sid, sid, sizeof(cmd->sid));

    ret = arch_os_queue_send(&self->q, &cmd, OS_NO_WAIT);
    if(ret != STATE_OK) {
        if(cmd)
            free(cmd);
        json_delegate_ack_err(ack, ctx, OT_ERR_CODE_RESP_ERROR, OT_ERR_INFO_RESP_ERROR);
        return STATE_ERROR;
    }
    return STATE_OK;
}

uint32_t mdm_get_q_len(mdm self)
{
    if(!self)
        return 0;

    return arch_os_queue_get_msgs_waiting(&self->q);
}

int json2cmd(char * json, char ** method, char ** params)
{
    jsmntok_t *method_tk = NULL;
    jsmntok_t *params_tk = NULL;
    jsmntok_t tk_buf[32];
    jsmn_parser psr;
    int ret = 0;

    *method = NULL;
    *params = NULL;

    jsmn_init(&psr);
    if (JSMN_SUCCESS != (ret = jsmn_parse(&psr, json, strlen(json), tk_buf, NELEMENTS(tk_buf)))) {
        LOG_ERROR("json parse err(%d).\r\n", ret);
        goto err_exit;
    }
    tk_buf[psr.toknext].type = JSMN_INVALID;
    if (NULL == (method_tk = jsmn_key_value(json, tk_buf, "method"))) {
        LOG_ERROR("error, no method found\r\n");
        goto err_exit;
    }
    *method = json + method_tk->start;
    json[method_tk->end] = '\0';

    if (NULL == (params_tk = jsmn_key_value(json, tk_buf, "params"))) {
        LOG_ERROR("error, no params found\r\n");
        goto err_exit;
    }
    *params = json + params_tk->start + 1; // skip '['
    json[params_tk->end - 1] = '\0'; // skip ']'

    if(strlen(*method) == 0) {
        LOG_ERROR("error, no method found\r\n");
        goto err_exit;
    }

    return STATE_OK;

err_exit:
    return STATE_ERROR;
}


