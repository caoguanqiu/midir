#include <string.h>
#include <wmstdio.h>
#include <boot_flags.h>
#include <lib/crash_trace.h>
#include <lib/arch_os.h>
#include <wm_os.h>
#include <appln_dbg.h>
#include <util/dump_hex_info.h>
#include <ot/d0_otd.h>

static crash_trace_t ct __attribute__((section(".nvram_uninit")));
static bool crash_report_sent_success;

void save_crash_trace(unsigned long *pmsp, unsigned long *ppsp, SCB_Type *scb,const char *task_name)
{
    uint32_t dump_size = sizeof(ct.stack_dump);

    memset(&ct, 0, sizeof(ct));
    ct.pmsp = pmsp;
    memcpy(&ct.msp, pmsp, sizeof(stframe));
    ct.ppsp = ppsp;
    memcpy(&ct.psp, ppsp, sizeof(stframe));
    memcpy(&ct.scb, scb, sizeof(SCB_Type));
    memcpy(&ct.task_name[0], task_name, sizeof(ct.task_name));

    wmprintf("psp addr= 0x%x dump_size = %u\r\n", ct.ppsp, dump_size);
    xiaomi_dump_hex((uint8_t *) ct.ppsp, (uint16_t) dump_size);
    wmprintf("=================================\r\n");

    memcpy(ct.stack_dump, ct.ppsp, dump_size);
    ct.stack_dump_size = dump_size;

    ct.valid = CRASH_TRACE_VALID_MAGIC;
}


static void clear_crash_trace(void)
{
    ct.valid = 0;
}

static const unsigned char  base64Encode[] = { 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
        'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l',
        'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7',
        '8', '9', '+', '/' };

enum {
    BAD = 0xFF, /* invalid encoding */
    PAD = '=', PEM_LINE_SZ = 64
};

/* porting assistance from yaSSL by Raphael HUCK */
static int Base64_Encode(const unsigned char * in, uint32_t inLen, uint8_t* out, uint32_t* outLen)
{
    uint32_t i = 0, j = 0;

    uint32_t outSz = (inLen + 3 - 1) / 3 * 4;
    //outSz += (outSz + PEM_LINE_SZ - 1) / PEM_LINE_SZ;  /* new lines */

    if (outSz > *outLen)
        return -1;

    while (inLen > 2) {
        uint8_t b1 = in[j++];
        uint8_t b2 = in[j++];
        uint8_t b3 = in[j++];

        /* encoded idx */
        uint8_t e1 = b1 >> 2;
        uint8_t e2 = ((b1 & 0x3) << 4) | (b2 >> 4);
        uint8_t e3 = ((b2 & 0xF) << 2) | (b3 >> 6);
        uint8_t e4 = b3 & 0x3F;

        /* store */
        out[i++] = base64Encode[e1];
        out[i++] = base64Encode[e2];
        out[i++] = base64Encode[e3];
        out[i++] = base64Encode[e4];

        inLen -= 3;

        //if ((++n % (PEM_LINE_SZ / 4)) == 0 && inLen)
        //    out[i++] = '\n';
    }

    /* last integral */
    if (inLen) {
        int twoBytes = (inLen == 2);

        uint8_t b1 = in[j++];
        uint8_t b2 = (twoBytes) ? in[j++] : 0;

        uint8_t e1 = b1 >> 2;
        uint8_t e2 = ((b1 & 0x3) << 4) | (b2 >> 4);
        uint8_t e3 = (b2 & 0xF) << 2;

        out[i++] = base64Encode[e1];
        out[i++] = base64Encode[e2];
        out[i++] = (twoBytes) ? base64Encode[e3] : PAD;
        out[i++] = PAD;
    }

    //out[i++] = '\n';
    if (i != outSz)
        return -2;
    *outLen = outSz;

    return 0;
}

static int crash_report_ack(jsmn_node_t* pjn, void* ctx)
{
    jsmntok_t *jt;

    if (pjn && pjn->js && NULL == pjn->tkn) {
        LOG_WARN("crash trace send err:%s. Retry.\r\n", pjn->js);
    } else if (NULL != (jt = jsmn_key_value(pjn->js, pjn->tkn, "result"))) {
        crash_report_sent_success = true;
        LOG_INFO("crash trace sent success.\r\n");
    }
    return STATE_OK;
}

/*
 * this function could possibly occupy worker thread for up to 20 seconds
 */
static int crash_report(crash_trace_t * ct)
{
    uint32_t n = 0;
    uint32_t base64_len = 0;
    int ret;
    char *js_buf;
    int try_count;

    LOG_INFO("Reporting crash...\r\n");

/*	delete me  <--------------------
    try_count = 3;
    while (try_count-- > 0 && NULL == (ot = (ot_info_t*) api_nb_find("otd"))) {
        api_os_tick_sleep(1000);
    }
    if (NULL == ot)
        return -1;
*/


    try_count = 8;
    while (try_count-- > 0 && NULL == otn_is_online()) {
        api_os_tick_sleep(1000);
    }

    if (NULL == otn_is_online())
        return -1;

    if (NULL == (js_buf = (char*) os_mem_alloc(OT_PACKET_SIZE_MAX))) {
        LOG_ERROR("Crash report malloc failed\r\n");
        return -1;
    }

    n += snprintf_safe(js_buf + n, OT_PACKET_SIZE_MAX - n, "{\"method\":\"_otc.crash\",\"params\":[\"");

    base64_len = OT_PACKET_SIZE_MAX - n - 4;
    ret = Base64_Encode((const uint8_t*) ct, (sizeof(*ct) - sizeof(ct->stack_dump) + ct->stack_dump_size),
            (uint8_t*) js_buf + n, &base64_len);
    if (0 != ret) {
        LOG_ERROR("base64 error %d\r\n", ret);
        os_mem_free(js_buf);
        clear_crash_trace();
        return -1;
    }
    n += base64_len;
    n += snprintf_safe(js_buf + n, OT_PACKET_SIZE_MAX - n, "\"]}");

    crash_report_sent_success = false;
    try_count = 3;
    while (try_count-- > 0 && !crash_report_sent_success) {
        LOG_INFO("Report crash try #%d\r\n", 3 - try_count);
        ot_api_method(js_buf, n, crash_report_ack, NULL);
        api_os_tick_sleep(5000);
    }
    os_mem_free(js_buf);
    clear_crash_trace();

    LOG_INFO("Report crash finish\r\n");

    return 0;
}

static int assert_report(assert_trace *at)
{
    uint32_t n = 0;
    char *js_buf;
    int try_count;

    LOG_INFO("Reporting assert...\r\n");

    try_count = 8;
    while (try_count-- > 0 && NULL == otn_is_online()) {
        api_os_tick_sleep(1000);
    }

    if (NULL == otn_is_online())
        return -1;

    if (NULL == (js_buf = (char*) os_mem_alloc(OT_PACKET_SIZE_MAX))) {
        LOG_ERROR("assert report malloc failed\r\n");
        return -1;
    }

    n += snprintf_safe(js_buf + n, OT_PACKET_SIZE_MAX - n, "{\"method\":\"_otc.crash\",\"params\":[\"%s\"]}",(const char *)at);

    crash_report_sent_success = false;
    try_count = 3;
    while (try_count-- > 0 && !crash_report_sent_success) {
        LOG_INFO("Report assert try #%d\r\n", 3 - try_count);
        ot_api_method(js_buf, n, crash_report_ack, NULL);
        api_os_tick_sleep(5000);
    }
    os_mem_free(js_buf);
    clear_crash_trace();

    LOG_INFO("Report assert finish\r\n");

    return 0;

}

int crash_assert_check(void)
{
    if (PMU_CM3_SYSRESETREQ == boot_reset_cause()) {
        if(CRASH_TRACE_VALID_MAGIC == ct.valid) {
            return crash_report(&ct);
        } else if(ASSERT_TRACE_VALID_MAGIC == ct.valid){
            return assert_report((assert_trace *)&ct);
        }
    }

    ct.valid = 0;
    return -1;
}

void content_4aligned(char * buffer,int *length)
{
    if(*length % 4 == 0) {
        return;
    }

    int i = 4 - (*length % 4);
    buffer[*length + i] = 0; //0 ended c string

    while(i > 0) {
        buffer[*length + i - 1] = ' ';
        --i;
    }

    *length = ((*length - 1) / 4 + 1) * 4;
}

void save_assert_trace(const char * file_name,const char * function,int line)
{
    int n = 0,temp,str_len = 0, i;

    assert_trace * at = (assert_trace *) &ct;
    at->valid = ASSERT_TRACE_VALID_MAGIC;

    char buffer[64];
    temp = snprintf_safe(buffer,64," assert fail  : ");
    memcpy(at->data + n,buffer,temp);
    n += temp;

    str_len = strlen(file_name);
    i = str_len -1;

    while((i >= 0) && (file_name[i] != '/')) { //get basename
        i--;
    }
    
    ++i;

    temp = snprintf_safe(buffer,64,"file:%s,",file_name + i);
    content_4aligned(buffer,&temp);
    memcpy(at->data + n,buffer,temp);
    n += temp;

    temp = snprintf_safe(buffer,64,"func:%s,",function);
    content_4aligned(buffer, &temp);
    memcpy(at->data + n,buffer,temp);
    n += temp;

    temp = snprintf_safe(buffer,64,"line:%d",line);
    content_4aligned(buffer,&temp);
    memcpy(at->data + n,buffer,temp);
    n += temp;

    os_thread_sleep(10);

    pm_reboot_soc();
}


#if (defined EASY_BACKTRACE && EASY_BACKTRACE)

#define R7VALUE_MINU_ADDRESS (8)
#define MAX_DUMP_DEPTH       (3)

void easy_back_trace()
{
    unsigned long in_r7,in_sp;

    __asm volatile ("mov %0, r7" : "=r" (in_r7) : : );
    __asm volatile ("mov %0, sp" : "=r" (in_sp) : : );

    portSTACK_TYPE iter;

    int dump_depth = MAX_DUMP_DEPTH;
    iter = in_sp;

    while(dump_depth >0) {
        if((iter + R7VALUE_MINU_ADDRESS) == *((portSTACK_TYPE *)iter)) {
            wmprintf("r7 value is 0x%x,lr is 0x%x \r\n",*((portSTACK_TYPE *)iter),*((portSTACK_TYPE *)(iter+sizeof(portSTACK_TYPE))));
            dump_depth--;
        }
        iter += sizeof(portSTACK_TYPE);
    }
}

#endif

