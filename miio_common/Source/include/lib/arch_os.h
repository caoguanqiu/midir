#ifndef __ARCH_OS_H__
#define __ARCH_OS_H__

#include <lib/lib_generic.h>
#include <wm_os.h>

typedef struct
{
	char year[4];
	char dash1;
	char month[2];
	char dash2;
	char day[2];
	char T;
	char hour[2];
	char colon1;
	char minute[2];
	char colon2;
	char second[2];
	char decimal;
	char sub_second[6];
	char Z; // UTC timezone
	char end;
} iso8601_time_t;


#define OS_NEVER_TIMEOUT OS_WAIT_FOREVER

#define BEIJING_UTC_SECOND_OFFSET 8*3600


typedef int (*fp_thread_code_t)(void* arg);

typedef void* os_mbox_pt;

typedef void os_mutex_vt;

typedef void* os_mbox_pvt;
typedef void os_mbox_vt;
typedef void os_thread_vt;




#define ARCH_OS_PRIORITY_DEFAULT (1) //WICED_APPLICATION_PRIORITY

#define WICED_PRIORITY_TO_NATIVE_PRIORITY(priority) (uint8_t)(configMAX_PRIORITIES - priority)


uint32_t arch_os_time_now(void);
void arch_os_time_tune(uint32_t std_time);

uint32_t arch_os_utc_now(void);
int arch_os_utc_set(uint32_t utc);
int arch_os_utc_parse(uint32_t ts, uint16_t* year, uint8_t* month, uint8_t* day, uint8_t* hour, uint8_t* min, uint8_t* sec);

int arch_os_iso8601_time(iso8601_time_t* iso8601_time);

uint32_t arch_os_tick_now( void );
void  arch_os_tick_sleep( uint32_t tick );
uint32_t arch_os_tick_elapsed(uint32_t last_tick);

os_thread_vt* arch_os_thread_current(void);
int arch_os_thread_create(os_thread_vt** thread, const char* name, fp_thread_code_t function, uint32_t stack_size, void* arg );
int arch_os_thread_delete(os_thread_vt* thread);
void arch_os_thread_finish(void);


int arch_os_mbox_new(os_mbox_vt **mbox, int size );
int arch_os_mbox_free(os_mbox_vt *mbox );
int arch_os_mbox_post( os_mbox_vt *mbox, void *msg, uint8_t from_isr);
int arch_os_mbox_fetch(os_mbox_vt *mbox, void **msg, uint32_t wait);


void arch_os_enter_critical(void);
void arch_os_exit_critical(void);

int arch_os_mutex_create(os_mutex_vt** handle);
int arch_os_mutex_delete(os_mutex_vt* handle);
int arch_os_mutex_put(os_mutex_vt* handle);
int arch_os_mutex_get(os_mutex_vt* handle, uint32_t wait);

int arch_os_queue_create(os_queue_t *qhandle, uint32_t q_len, uint32_t item_size);
int arch_os_queue_delete(os_queue_t *qhandle);
int arch_os_queue_send(os_queue_t *qhandle, const void *msg, uint32_t wait);
int arch_os_queue_recv(os_queue_t *qhandle, void *msg, uint32_t wait);
int arch_os_queue_get_msgs_waiting(os_queue_t *qhandle);

/* interface macro defination */
#define	api_os_thread_current   arch_os_thread_current
#define api_os_thread_create    arch_os_thread_create
#define api_os_thread_delete    arch_os_thread_delete
#define api_os_thread_finish    arch_os_thread_finish

#define api_os_mbox_new         arch_os_mbox_new
#define api_os_mbox_free        arch_os_mbox_free
#define api_os_mbox_post        arch_os_mbox_post
#define api_os_mbox_fetch       arch_os_mbox_fetch

#define api_os_tick_sleep       arch_os_tick_sleep
#define api_os_tick_now         arch_os_tick_now
#define api_os_tick_elapsed     arch_os_tick_elapsed

#define api_os_ms_elapsed     arch_os_tick_elapsed

#define api_os_time_now         arch_os_time_now
#define api_os_time_tune        arch_os_time_tune
#define api_os_utc_now          arch_os_utc_now
#define api_os_utc_set          arch_os_utc_set
#define api_os_utc_parse        arch_os_utc_parse

#define api_os_iso8601_time     arch_os_iso8601_time

#define api_os_enter_critical   arch_os_enter_critical
#define api_os_exit_critical    arch_os_exit_critical

#define api_os_schedule_stop    arch_os_schedule_stop
#define api_os_schedule_resume  arch_os_schedule_resume

#define api_os_mutex_create     arch_os_mutex_create
#define api_os_mutex_delete     arch_os_mutex_delete
#define api_os_mutex_put        arch_os_mutex_put
#define api_os_mutex_get        arch_os_mutex_get

#endif /* __ARCH_OS_H__ */
